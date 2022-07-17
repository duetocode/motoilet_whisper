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
#include <stdlib.h>
#include "ring_buffer.c"

static ring_buffer_t rb;
#define _BUF_LEN 128
static uint8_t _buf[_BUF_LEN];

void test_init(void)
{
    TEST_ASSERT_EQUAL(_BUF_LEN, ring_buffer_capacity(rb));
    TEST_ASSERT_EQUAL(0, ring_buffer_size(rb));
}

void test_push(void)
{
    uint8_t data_1[] = {0x02, 0x01};
    ring_buffer_push(rb, data_1, sizeof(data_1));
    TEST_ASSERT_EQUAL(_BUF_LEN, ring_buffer_capacity(rb));
    TEST_ASSERT_EQUAL(2, ring_buffer_size(rb));
    TEST_ASSERT_EQUAL(data_1[0], *ring_buffer_at(rb, 0));
    TEST_ASSERT_EQUAL(data_1[1], *ring_buffer_at(rb, 1));

    uint8_t data_2[] = {0x03, 0x04};
    ring_buffer_push(rb, data_2, sizeof(data_2));
    TEST_ASSERT_EQUAL(4, ring_buffer_size(rb));
    TEST_ASSERT_EQUAL(data_2[0], *ring_buffer_at(rb, 2));
    TEST_ASSERT_EQUAL(data_2[1], *ring_buffer_at(rb, 3));
}

void test_pop(void)
{
    uint8_t data[] = {0x02, 0x01};
    ring_buffer_push(rb, data, sizeof(data));
    TEST_ASSERT_EQUAL(2, ring_buffer_size(rb));
    ring_buffer_pop(rb);
    TEST_ASSERT_EQUAL(data[1], *ring_buffer_at(rb, 0));
    TEST_ASSERT_EQUAL(1, ring_buffer_size(rb));
}

void test_clear(void)
{
    uint8_t data[] = {0x02, 0x01};
    ring_buffer_push(rb, data, sizeof(data));
    TEST_ASSERT_EQUAL(2, ring_buffer_size(rb));
    ring_buffer_clear(rb);
    TEST_ASSERT_EQUAL(0, ring_buffer_size(rb));
}

void setUp(void)
{
    rb = ring_buffer_create(_buf, _BUF_LEN);
}

void tearDown(void) {}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_push);
    RUN_TEST(test_pop);
    RUN_TEST(test_clear);
    return UNITY_END();
}