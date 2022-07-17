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
#include <unity.h>
#include <stdint.h>
#include <string.h>
#include "crc.h"
#include "data_layer.c"

#define _BUF_LEN 128
static uint8_t _buf[_BUF_LEN];

static void test_init(void)
{
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
}

static void test_basic_prefix_handling(void)
{
    // First trunk
    state = STATE_PREFIX;
    memset(_buf, 0, _BUF_LEN);
    uint8_t data[] = {0x00, 0x01, 0x02, 0x0A};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
    TEST_ASSERT_EQUAL(1, ring_buffer_size(receive_buf));

    // Another trunk
    data[0] = 0x0D;
    actual = whisper_data_layer__data_received(data, 1);
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_FLAGS, state);
}

static void test_prefix_handling_with_incomplete_data(void)
{
    state = STATE_PREFIX;
    memset(_buf, 0, _BUF_LEN);
    uint8_t data[] = {0x00, 0x0A, 0x02, 0x0D};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
    TEST_ASSERT_EQUAL(1, ring_buffer_size(receive_buf));
}

static void test_prefix_handling_with_repeated_data(void)
{
    state = STATE_PREFIX;
    memset(_buf, 0, _BUF_LEN);
    uint8_t data[] = {0x0A, 0x0A, 0x0D};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_FLAGS, state);
    TEST_ASSERT_EQUAL(2, ring_buffer_size(receive_buf));
}

static void test_flags_handling(void)
{
    // ack
    state = STATE_FLAGS;
    ring_buffer_clear(receive_buf);
    ring_buffer_push(receive_buf, (uint8_t[]){0x0A, 0x0D}, 2);
    uint8_t data[] = {0x01};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PAYLOAD_LEN, state);

    // data
    state = STATE_FLAGS;
    ring_buffer_clear(receive_buf);
    ring_buffer_push(receive_buf, (uint8_t[]){0x0A, 0x0D}, 2);
    data[0] = 0x02;
    actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PAYLOAD_LEN, state);
}

static void test_flags_handling_with_invalid_data(void)
{
    state = STATE_FLAGS;
    ring_buffer_clear(receive_buf);
    ring_buffer_push(receive_buf, (uint8_t[]){0x0A, 0x0D}, 2);
    uint8_t data[] = {0x00};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
}

static void test_payload_len_handling(void)
{
    state = STATE_PAYLOAD_LEN;
    ring_buffer_clear(receive_buf);
    ring_buffer_push(receive_buf, (uint8_t[]){0x0A, 0x0D, 0x02}, 3);
    uint8_t data[] = {0x08};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(0x08, packet.payload_len);
    TEST_ASSERT_EQUAL(STATE_PAYLOAD, state);
}

static void test_payload_len_handling_with_exccesive_payload_length(void)
{
    state = STATE_PAYLOAD_LEN;
    ring_buffer_clear(receive_buf);
    ring_buffer_push(receive_buf, (uint8_t[]){0x0A, 0x0D, 0x02}, 3);
    uint8_t data[] = {0xFF};
    char actual = whisper_data_layer__data_received(data, sizeof(data));
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(0x00, packet.payload_len);
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
}

static void test_payload_handling(void)
{
    state = STATE_PAYLOAD;
    uint8_t data[] = {0x0A, 0x0D, 0x02, 0x04, 0x01, 0x02, 0x04, 0x03};
    ring_buffer_push(receive_buf, data, 4);
    packet.payload_len = 4;
    char actual = whisper_data_layer__data_received(&data[4], 2);
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PAYLOAD, state);
    TEST_ASSERT_EQUAL(0, packet.checksum);

    // Second trunk
    actual = whisper_data_layer__data_received(&data[6], 2);
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_CHECKSUM, state);
    TEST_ASSERT_EQUAL(calculate_crc(data, sizeof(data)), packet.checksum);
}

static unsigned short data_received_length = 0;
static void test_checksum_handling(void)
{
    state = STATE_CHECKSUM;
    data_received_length = 0;
    ring_buffer_clear(receive_buf);
    uint8_t data[] = {0x0A, 0x0D, 0x02, 0x02, 0x02, 0x03};
    ring_buffer_push(receive_buf, data, sizeof(data));
    packet.payload_len = 2;
    packet.flags = 0x02;
    TEST_ASSERT_EQUAL(0, data_received_length);

    uint16_t checksum = calculate_crc(data, sizeof(data));
    packet.checksum = checksum;
    uint8_t *checksum_data = (uint8_t *)&checksum;

    char actual = whisper_data_layer__data_received(checksum_data, 2);
    TEST_ASSERT_EQUAL(0, actual);
    TEST_ASSERT_EQUAL(STATE_PREFIX, state);
    TEST_ASSERT_EQUAL(2, data_received_length);
}

static void on_packet_received(unsigned char data_len)
{
    data_received_length = data_len;
}

void setUp()
{
    memset(_buf, 0, _BUF_LEN);
    struct whisper_data_layer__config cfg = {
        .buf = _buf,
        .buf_len = _BUF_LEN,
        .packet_received_cb = on_packet_received,
    };
    whisper_data_layer__init(&cfg);
}
void tearDown() {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_basic_prefix_handling);
    RUN_TEST(test_prefix_handling_with_incomplete_data);
    RUN_TEST(test_prefix_handling_with_repeated_data);

    RUN_TEST(test_flags_handling);
    RUN_TEST(test_flags_handling_with_invalid_data);

    RUN_TEST(test_payload_len_handling);
    RUN_TEST(test_payload_len_handling_with_exccesive_payload_length);

    RUN_TEST(test_payload_handling);
    RUN_TEST(test_checksum_handling);
    return UNITY_END();
}