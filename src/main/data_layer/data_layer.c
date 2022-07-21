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
 * SOFTWARE.
 */
#include "data_layer.h"
#include <assert.h>
#include <string.h>
#include <limits.h>

#include "crc.h"
#include "array_buffer.h"

#define RETRANSMISSION_DELAY_MS 50

// receive buffer for protocol handling
array_buffer_t _buf_recv;

struct whisper_data_layer__packet_header
{
    uint16_t seq_no;
    uint8_t flags;
    uint8_t payload_len;
} * packet_header;

static struct buffered_packet
{
    uint8_t empty;
    uint8_t ack_required;
    struct whisper_data_layer__packet_header header;
    uint8_t *payload;
    uint8_t num_transmissions;
} send_buffer;

#define MAX_RETRANSMISSIONS 3

// flags of the packet flags byte
#define FLAGS_ACK 0b00000001
#define FLAGS_DATA 0b00000010
#define FLAGS_SEQ_RESET 0b00000100

// all valid packets begin with this
static const uint8_t PACKET_PREFIX[] = {0x0A, 0x0D};

#define LEN_PREFIX sizeof(PACKET_PREFIX)
#define LEN_HEADER sizeof(struct whisper_data_layer__packet_header)
#define LEN_CHECKSUM 2

// state of the the finite state machine
#define STATE_PREFIX 0x00
#define STATE_HEADER 0x01
#define STATE_PAYLOAD 0x02
#define STATE_CHECKSUM 0x03

static uint8_t state;
static uint8_t next_state;
struct whisper_data_layer__config cfg;
static uint16_t counter;
static uint16_t receive_counter;

static void transite(uint8_t new_state) { next_state = new_state; }

static void reset(void)
{
    transite(STATE_PREFIX);
}

void whisper_data_layer__init(struct whisper_data_layer__config *config)
{
    memcpy(&cfg, config, sizeof(struct whisper_data_layer__config));
    _buf_recv = (array_buffer_t)cfg.buf;
    cfg.buf = &cfg.buf[SIZEOF_ARRAY_BUFFER_T];
    cfg.buf_len -= SIZEOF_ARRAY_BUFFER_T;
    array_buffer__init(_buf_recv, cfg.buf, cfg.buf_len);

    state = STATE_PREFIX;
    counter = 0;
    send_buffer.empty = 1;

    packet_header = (struct whisper_data_layer__packet_header *)array_buffer__at(_buf_recv, LEN_PREFIX);
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
            array_buffer__clear(_buf_recv);
            ret = -1;
        }

        if (next_state != state)
            state = next_state;
    }
}

char whisper_data_layer__data_received(const uint8_t *data,
                                       uint8_t data_length)
{
    // The function is a finite state machine driven by the data received event.

    // push data to the tail of the receive buffer
    while (data_length > 0)
    {
        uint16_t bytes_to_copy =
            array_buffer__capacity(_buf_recv) - array_buffer__size(_buf_recv);
        if (bytes_to_copy > data_length)
            bytes_to_copy = data_length;

        array_buffer__push(_buf_recv, data, bytes_to_copy);
        data_length -= bytes_to_copy;

        // process the buffered data
        process_buffered_data();
    }

    return 0;
}

static char handle_prefix(void)
{
    if (array_buffer__size(_buf_recv) < LEN_PREFIX)
        // stop processing if the prefix is not yet received
        return 0;

    uint8_t num_of_matches = 0;

    while (array_buffer__size(_buf_recv) >= LEN_PREFIX &&
           num_of_matches < LEN_PREFIX)
    {
        if (*array_buffer__at(_buf_recv, num_of_matches) ==
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
            array_buffer__pop(_buf_recv, 1);
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

    if (array_buffer__size(_buf_recv) < LEN_PREFIX + LEN_HEADER)
        // stop processing if the header is not yet fully received
        return 0;

    // check the flags field
    if (packet_header->flags < FLAGS_ACK || packet_header->flags > FLAGS_DATA)
    {
        // invalid flags, reset the state and pop
        reset();
        // track back and go over again from the second byte
        array_buffer__pop(_buf_recv, 1);
        return 1;
    }

    // check the payload length field
    if (packet_header->payload_len >
        array_buffer__capacity(_buf_recv) - LEN_PREFIX - LEN_HEADER - LEN_CHECKSUM)
    {
        reset();
        array_buffer__pop(_buf_recv, 1);
        // continue processing the buffer
        return 1;
    }

    // transite to payload state
    transite(STATE_PAYLOAD);
    return 1;
}

static char handle_payload(void)
{
    uint16_t data_size = array_buffer__size(_buf_recv);
    assert(data_size >= LEN_PREFIX + LEN_HEADER);

    uint16_t expected_size = LEN_PREFIX + LEN_HEADER + packet_header->payload_len;

    if (data_size < LEN_PREFIX + LEN_HEADER + packet_header->payload_len)
        // do not have enought data yet, stop processing
        return 0;

    // accumulated enough data, transite to checksum state
    transite(STATE_CHECKSUM);

    // continue processing the buffer
    return 1;
}

static void ack(void);
static void on_ack(void);

static void _frame_received(void)
{
    // reset the packet according to the received packet
    if (packet_header->flags & FLAGS_SEQ_RESET)
    {
        counter = packet_header->seq_no - 1;
    }
    counter = packet_header->seq_no;

    // hand the actual packet
    if (packet_header->flags & FLAGS_ACK)
    {
        on_ack();
    }
    else if (packet_header->flags & FLAGS_DATA)
    {
        if (packet_header->seq_no < receive_counter)
        {
            // The packet is a re-transmission, just ignore it.
            return;
        }

        if (cfg.packet_received_cb)
            cfg.packet_received_cb(
                array_buffer__at(_buf_recv, LEN_PREFIX + LEN_HEADER),
                packet_header->payload_len);

        ack();
    }
}

static char handle_checksum(void)
{
    // the expected frame length
    uint8_t precedent_length = LEN_PREFIX + LEN_HEADER + packet_header->payload_len;
    uint8_t expected_frame_length = precedent_length + LEN_CHECKSUM;

    if (array_buffer__size(_buf_recv) < expected_frame_length)
        // do not have enought data yet, stop processing
        return 0;

    // calculate the checksum of the frame
    uint16_t actual_checksum = update_crc_buf(array_buffer__at(_buf_recv, 0), precedent_length, CRC_INIT);

    // read the crc and check against the calculated one
    uint16_t *expected_checksum = (uint16_t *)array_buffer__at(_buf_recv, precedent_length);

    if (*expected_checksum != actual_checksum)
    {
        // checksum mismatch, reset the state and pop
        reset();
        array_buffer__pop(_buf_recv, 1);
        // continue processing the buffer
        return 1;
    }

    // checksum matched, process the frame
    _frame_received();

    // pop the entire frame from the buffer
    array_buffer__pop(_buf_recv, expected_frame_length);
    reset();

    return 1;
}

static void ack(void)
{
    assert((packet_header->flags & FLAGS_ACK) == 0);

    uint8_t buf[LEN_PREFIX + LEN_HEADER + sizeof(uint16_t) + LEN_CHECKSUM] = {
        PACKET_PREFIX[0],
        PACKET_PREFIX[1],
        counter & 0x00ff,
        (counter & 0xff00) >> 8,
        FLAGS_ACK,
        2,
        packet_header->seq_no & 0x00ff,
        packet_header->seq_no >> 8,
    };

    uint16_t checksum = calculate_crc(buf, LEN_PREFIX + LEN_HEADER + sizeof(uint16_t));
    buf[LEN_PREFIX + LEN_HEADER + sizeof(uint16_t)] = checksum & 0x00ff;
    buf[LEN_PREFIX + LEN_HEADER + sizeof(uint16_t) + 1] = checksum >> 8;

    cfg.data_write(buf, LEN_PREFIX + LEN_HEADER + LEN_CHECKSUM + 2);
}

static void _send_data(void)
{
    if (send_buffer.empty)
        return;

    if (send_buffer.num_transmissions >= MAX_RETRANSMISSIONS)
    {
        // too many retransmissions, drop the packet
        send_buffer.empty = 1;
        return;
    }

    // send out the data and calculate the checksum of the frame
    uint16_t checksum = CRC_INIT;

    // PREFIX
    checksum = update_crc_buf(PACKET_PREFIX, LEN_PREFIX, checksum);
    cfg.data_write(PACKET_PREFIX, LEN_PREFIX);

    // HEADER
    checksum = update_crc_buf((uint8_t *)&send_buffer.header, LEN_HEADER, checksum);
    cfg.data_write(PACKET_PREFIX, LEN_HEADER);

    // PAYLOAD
    checksum = update_crc_buf(send_buffer.payload, send_buffer.header.payload_len, checksum);
    cfg.data_write(send_buffer.payload, send_buffer.header.payload_len);

    // CHECKSUM
    cfg.data_write((uint8_t *)&checksum, LEN_CHECKSUM);

    // increase the number of transmissions
    ++send_buffer.num_transmissions;

    // schedule the next transmission
    if (send_buffer.num_transmissions < MAX_RETRANSMISSIONS)
    {
        cfg.set_delay(RETRANSMISSION_DELAY_MS, _send_data);
    }
}

void on_ack(void)
{
    assert(packet_header->flags | FLAGS_ACK);

    // only proceed if there are data waiting for acknowlegement
    if (send_buffer.empty)
        return;

    // get the acked sequence number for the payload
    uint16_t *ack_seq_no = (uint16_t *)array_buffer__at(_buf_recv, LEN_PREFIX + LEN_HEADER);

    // check if the acked sequence number is the same as the sending one
    if (send_buffer.header.seq_no != *ack_seq_no)
        return;

    cfg.cancel_delay();
    memset(&send_buffer, 0, sizeof(send_buffer));
    send_buffer.empty = 1;
}

uint16_t whisper_data_layer__data_sent(uint8_t *data, uint8_t data_length, uint8_t act_required)
{
    if (!send_buffer.empty)
        return 0;

    // resever 0 for buffer full error
    ++counter;
    if (counter == 0)
        ++counter;

    // try to send out the frame
    struct whisper_data_layer__packet_header header = {
        .seq_no = counter,
        .flags = FLAGS_DATA,
        .payload_len = data_length,
    };

    // buffer the data and send
    send_buffer.empty = 0;
    send_buffer.ack_required = act_required;
    send_buffer.header.seq_no = counter;
    send_buffer.header.flags = FLAGS_DATA;
    send_buffer.header.payload_len = data_length;
    send_buffer.payload = data;

    if (counter == 1)
    {
        // the counter wraps to the beginning
        header.flags |= FLAGS_SEQ_RESET;
    }

    // send the frame
    _send_data();

    return header.seq_no;
}