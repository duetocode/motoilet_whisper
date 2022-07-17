/* MIT License
 * 
 * Copyright (c) 2022 Ningbo Peakhonor Technology Co., Limited
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.:0
 */ 
#include "data_layer.h"
#include <assert.h>
#include <string.h>

#include "crc.h"
#include "ring_buffer.h"

// receive buffer for protocol handling
ring_buffer_t receive_buf;

struct whisper_data_layer__packet
{
    unsigned char flags;
    unsigned char payload_len;
    unsigned short int checksum;
} packet;

// flags of the packet flags byte
#define FLAGS_ACK 0b00000001
#define FLAGS_DATA 0b00000010

// all valid packets begin with this
static const unsigned char PACKET_PREFIX[] = {0x0A, 0x0D};

#define LEN_PREFIX sizeof(PACKET_PREFIX)
#define LEN_HEADER 1 + 1
#define LEN_CHECKSUM 2

// state of the the finite state machine
#define STATE_PREFIX 0x00
#define STATE_FLAGS 0x01
#define STATE_PAYLOAD_LEN 0x02
#define STATE_PAYLOAD 0x03
#define STATE_CHECKSUM 0x04

static unsigned char state;
static unsigned char next_state;
struct whisper_data_layer__config cfg;

static void transite(unsigned char new_state) { next_state = new_state; }

static void reset(void)
{
    memset(&packet, 0, sizeof(packet));
    transite(STATE_PREFIX);
}

void whisper_data_layer__init(struct whisper_data_layer__config *config)
{
    memcpy(&cfg, config, sizeof(struct whisper_data_layer__config));
    receive_buf = ring_buffer_create(config->buf, config->buf_len);
    memset(&packet, 0, sizeof(packet));
    state = STATE_PREFIX;
}

static char handle_prefix(void);
static char handle_flags(void);
static char handle_payload_len(void);
static char handle_payload(void);
static char handle_checksum(void);

static void process_buffered_data(void)
{
    char ret = 1;
    while (ret == 1)
    {
        next_state = state;
        switch (state)
        {
        case STATE_PREFIX:
            ret = handle_prefix();
            break;
        case STATE_FLAGS:
            ret = handle_flags();
            break;
        case STATE_PAYLOAD_LEN:
            ret = handle_payload_len();
            break;
        case STATE_PAYLOAD:
            ret = handle_payload();
            break;
        case STATE_CHECKSUM:
            ret = handle_checksum();
            break;
        default:
            // fatal, as the state is unknown
            transite(STATE_PREFIX);
            ring_buffer_clear(receive_buf);
            ret = -1;
        }

        if (next_state != state)
            state = next_state;
    }
}

char whisper_data_layer__data_received(const unsigned char *data,
                                       unsigned char data_length)
{
    // The function is a finite state machine driven by the data received event.

    // push data to the tail of the receive buffer
    while (data_length > 0)
    {
        unsigned short bytes_to_copy =
            ring_buffer_capacity(receive_buf) - ring_buffer_size(receive_buf);
        if (bytes_to_copy > data_length)
            bytes_to_copy = data_length;

        ring_buffer_push(receive_buf, data, bytes_to_copy);
        data_length -= bytes_to_copy;

        // process the buffered data
        process_buffered_data();
    }

    return 0;
}

static char handle_prefix(void)
{
    if (ring_buffer_size(receive_buf) < LEN_PREFIX)
        // stop processing if the prefix is not yet received
        return 0;

    unsigned char num_of_matches = 0;

    while (ring_buffer_size(receive_buf) >= LEN_PREFIX &&
           num_of_matches < LEN_PREFIX)
    {
        if (*ring_buffer_at(receive_buf, num_of_matches) ==
            PACKET_PREFIX[num_of_matches])
        {
            // found matches, increase the counter
            ++num_of_matches;
        }
        else
        {
            // does not match, reset the counter and pop the buffer
            // drop the first byte and search from the beginning of the buffer
            num_of_matches = 0;
            ring_buffer_pop(receive_buf);
        }
    }

    if (num_of_matches == LEN_PREFIX)
    {
        // we found the whole prefix, move to the next state
        transite(STATE_FLAGS);
        // continue process the buffer
        return 1;
    }
    else
    {
        // match not found, stop processing
        return 0;
    }
}

static char handle_flags(void)
{
    assert(ring_buffer_size(receive_buf) >= LEN_PREFIX);

    if (ring_buffer_size(receive_buf) == LEN_PREFIX)
        // do not have enought data yet, stop processing
        return 0;

    packet.flags = *ring_buffer_at(receive_buf, 2);
    if (packet.flags < FLAGS_ACK || packet.flags > FLAGS_DATA)
    {
        // invalid flags, reset the state and pop
        reset();
        // track back and go over again from the second byte
        ring_buffer_pop(receive_buf);
        return 1;
    }

    // move to the next state
    transite(STATE_PAYLOAD_LEN);

    // continue processing the remaining data in the buffer
    return 1;
}

static char handle_payload_len(void)
{
    assert(ring_buffer_size(receive_buf) >= LEN_PREFIX + 1);

    if (ring_buffer_size(receive_buf) == LEN_PREFIX + 1)
        // do not have enought data yet, stop processing
        return 0;

    packet.payload_len = *ring_buffer_at(receive_buf, 3);

    // check if the data if too big for the buffer
    if (packet.payload_len > ring_buffer_capacity(receive_buf) - LEN_PREFIX -
                                 LEN_HEADER - LEN_CHECKSUM)
    {
        reset();
        ring_buffer_pop(receive_buf);
        // continue processing the buffer
        return 1;
    }

    transite(STATE_PAYLOAD);
    // continue to process the buffer
    return 1;
}

static char handle_payload(void)
{
    unsigned short data_size = ring_buffer_size(receive_buf);
    assert(data_size >= LEN_PREFIX + LEN_HEADER);

    unsigned short expected_size = LEN_PREFIX + LEN_HEADER + packet.payload_len;

    if (data_size < LEN_PREFIX + LEN_HEADER + packet.payload_len)
        // do not have enought data yet, stop processing
        return 0;

    // accumulated enough data, calculate the actual checksum and move to the next
    // state calculate the checksum
    unsigned short crc = CRC_INIT;
    for (int i = 0; i < LEN_PREFIX + LEN_HEADER + packet.payload_len; ++i)
    {
        crc = update_crc(*ring_buffer_at(receive_buf, i), crc);
    }
    packet.checksum = crc;

    // transite to checksum state
    transite(STATE_CHECKSUM);

    // continue processing the buffer
    return 1;
}

static char handle_checksum(void)
{
    unsigned short frame_length =
        LEN_PREFIX + LEN_HEADER + packet.payload_len + LEN_CHECKSUM;
    if (ring_buffer_size(receive_buf) < frame_length)
        // do not have enought data yet, stop processing
        return 0;

    // read the crc and check against the calculated one
    unsigned short checksum =
        *ring_buffer_at(receive_buf, frame_length - LEN_CHECKSUM);
    checksum =
        checksum | *ring_buffer_at(receive_buf, frame_length - LEN_CHECKSUM + 1)
                       << 8;

    if (checksum != packet.checksum)
    {
        // checksum mismatch, reset the state and pop
        reset();
        ring_buffer_pop(receive_buf);
        // continue processing the buffer
        return 1;
    }

    // the frame is valid, report if this is a data frame
    if (cfg.packet_received_cb && packet.flags & 0x02)
        cfg.packet_received_cb(packet.payload_len);

    // packet handling finished, reset the frame
    // pop the entire frame from the buffer
    ring_buffer_batch_pop(receive_buf, frame_length);
    // set the state back to the prefix state
    reset();

    // continue processing the buffer
    return 1;
}