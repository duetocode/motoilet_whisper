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

struct whisper_data_layer__packet_header
{
    unsigned short seq_no;
    unsigned char flags;
    unsigned char payload_len;
} packet_header;

// flags of the packet flags byte
#define FLAGS_ACK 0b00000001
#define FLAGS_DATA 0b00000010

// all valid packets begin with this
static const unsigned char PACKET_PREFIX[] = {0x0A, 0x0D};

#define LEN_PREFIX sizeof(PACKET_PREFIX)
#define LEN_HEADER sizeof(struct whisper_data_layer__packet_header)
#define LEN_CHECKSUM 2

// state of the the finite state machine
#define STATE_PREFIX 0x00
#define STATE_HEADER 0x01
#define STATE_PAYLOAD 0x02
#define STATE_CHECKSUM 0x03

static unsigned char state;
static unsigned char next_state;
struct whisper_data_layer__config cfg;
static unsigned short counter;

static void transite(unsigned char new_state) { next_state = new_state; }

static void reset(void)
{
    memset(&packet_header, 0, sizeof(packet_header));
    transite(STATE_PREFIX);
}

void whisper_data_layer__init(struct whisper_data_layer__config *config)
{
    memcpy(&cfg, config, sizeof(struct whisper_data_layer__config));
    receive_buf = ring_buffer_create(config->buf, config->buf_len);
    memset(&packet_header, 0, sizeof(packet_header));
    state = STATE_PREFIX;
    counter = 0;
}

static char handle_prefix(void);
static char handle_header(void);
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
        case STATE_HEADER:
            ret = handle_header();
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
        transite(STATE_HEADER);
        // continue process the buffer
        return 1;
    }
    else
    {
        // match not found, stop processing
        return 0;
    }
}

static char handle_header(void)
{

    if (ring_buffer_size(receive_buf) < LEN_PREFIX + LEN_HEADER)
        // stop processing if the header is not yet fully received
        return 0;

    // copy the header to the packet structure
    ring_buffer_read((unsigned char *)&packet_header, receive_buf, LEN_PREFIX, LEN_HEADER);

    // check the flags field
    if (packet_header.flags < FLAGS_ACK || packet_header.flags > FLAGS_DATA)
    {
        // invalid flags, reset the state and pop
        reset();
        // track back and go over again from the second byte
        ring_buffer_pop(receive_buf);
        return 1;
    }

    // check the payload length field
    if (packet_header.payload_len >
        ring_buffer_capacity(receive_buf) - LEN_PREFIX - LEN_HEADER - LEN_CHECKSUM)
    {
        reset();
        ring_buffer_pop(receive_buf);
        // continue processing the buffer
        return 1;
    }

    // transite to payload state
    transite(STATE_PAYLOAD);
    return 1;
}

static char handle_payload(void)
{
    unsigned short data_size = ring_buffer_size(receive_buf);
    assert(data_size >= LEN_PREFIX + LEN_HEADER);

    unsigned short expected_size = LEN_PREFIX + LEN_HEADER + packet_header.payload_len;

    if (data_size < LEN_PREFIX + LEN_HEADER + packet_header.payload_len)
        // do not have enought data yet, stop processing
        return 0;

    // accumulated enough data, transite to checksum state
    transite(STATE_CHECKSUM);

    // continue processing the buffer
    return 1;
}

static void ack(void);

static char handle_checksum(void)
{
    // the expected frame length
    unsigned short expected_frame_length =
        LEN_PREFIX + LEN_HEADER + packet_header.payload_len + LEN_CHECKSUM;

    if (ring_buffer_size(receive_buf) < expected_frame_length)
        // do not have enought data yet, stop processing
        return 0;

    // calculate the checksum of the frame
    unsigned short actual_checksum = CRC_INIT;
    for (int i = 0; i < LEN_PREFIX + LEN_HEADER + packet_header.payload_len; ++i)
    {
        actual_checksum = update_crc(*ring_buffer_at(receive_buf, i), actual_checksum);
    }

    // read the crc and check against the calculated one
    unsigned short expected_checksum = 0;
    ring_buffer_read((unsigned char *)&expected_checksum, receive_buf, LEN_PREFIX + LEN_HEADER + packet_header.payload_len, LEN_CHECKSUM);

    if (expected_checksum != actual_checksum)
    {
        // checksum mismatch, reset the state and pop
        reset();
        ring_buffer_pop(receive_buf);
        // continue processing the buffer
        return 1;
    }

    if (packet_header.flags & FLAGS_DATA && cfg.packet_received_cb)
    {
        // dispatch data
        cfg.packet_received_cb(packet_header.payload_len);
        // acknowledge the packet
        ack();
    }

    // packet handling finished, reset the frame
    // pop the entire frame from the buffer
    ring_buffer_batch_pop(receive_buf, expected_frame_length);
    // set the state back to the prefix state
    reset();

    // continue processing the buffer
    return 1;
}

static void ack(void)
{
    assert((packet_header.flags & FLAGS_ACK) == 0);

    unsigned char buf[LEN_PREFIX + LEN_HEADER + sizeof(unsigned short) + LEN_CHECKSUM] = {
        PACKET_PREFIX[0],
        PACKET_PREFIX[1],
        counter & 0x00ff,
        (counter & 0xff00) >> 8,
        FLAGS_ACK,
        2,
        packet_header.seq_no & 0x00ff,
        packet_header.seq_no >> 8,
    };

    unsigned short checksum = calculate_crc(buf, LEN_PREFIX + LEN_HEADER + sizeof(unsigned short));
    buf[LEN_PREFIX + LEN_HEADER + sizeof(unsigned short)] = checksum & 0x00ff;
    buf[LEN_PREFIX + LEN_HEADER + sizeof(unsigned short) + 1] = checksum >> 8;

    cfg.data_write(buf, LEN_PREFIX + LEN_HEADER + LEN_CHECKSUM + 2);
}