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
#include <unity.h>
#include <stdlib.h>
#include "array_buffer.h"

static array_buffer_t ab;
#define _BUF_LEN 128
static uint8_t _buf[_BUF_LEN];

void test_init(void)
{
    TEST_ASSERT_EQUAL(_BUF_LEN, array_buffer__capacity(ab));
    TEST_ASSERT_EQUAL(0, array_buffer__size(ab));
}

void test_push(void)
{
    uint8_t data_1[] = {0x02, 0x01};
    array_buffer__push(ab, data_1, sizeof(data_1));
    TEST_ASSERT_EQUAL(_BUF_LEN, array_buffer__capacity(ab));
    TEST_ASSERT_EQUAL(2, array_buffer__size(ab));
    TEST_ASSERT_EQUAL(data_1[0], *array_buffer__at(ab, 0));
    TEST_ASSERT_EQUAL(data_1[1], *array_buffer__at(ab, 1));

    uint8_t data_2[] = {0x03, 0x04};
    array_buffer__push(ab, data_2, sizeof(data_2));
    TEST_ASSERT_EQUAL(4, array_buffer__size(ab));
    TEST_ASSERT_EQUAL(data_2[0], *array_buffer__at(ab, 2));
    TEST_ASSERT_EQUAL(data_2[1], *array_buffer__at(ab, 3));
}

void test_pop(void)
{
    uint8_t data[] = {0x02, 0x01};
    array_buffer__push(ab, data, sizeof(data));
    TEST_ASSERT_EQUAL(2, array_buffer__size(ab));
    uint8_t poped = array_buffer__pop(ab, 1);
    TEST_ASSERT_EQUAL(1, poped);
    TEST_ASSERT_EQUAL(data[1], *array_buffer__at(ab, 0));
    TEST_ASSERT_EQUAL(1, array_buffer__size(ab));
}

void test_clear(void)
{
    uint8_t data[] = {0x02, 0x01};
    array_buffer__push(ab, data, sizeof(data));
    TEST_ASSERT_EQUAL(2, array_buffer__size(ab));
    uint8_t cleared = array_buffer__clear(ab);
    TEST_ASSERT_EQUAL(2, cleared);
    TEST_ASSERT_EQUAL(0, array_buffer__size(ab));
}

void setUp(void)
{
    ab = malloc(SIZEOF_ARRAY_BUFFER_T);
    array_buffer__init(ab, _buf, _BUF_LEN);
}

void tearDown(void)
{
    free(ab);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init);
    RUN_TEST(test_push);
    RUN_TEST(test_pop);
    RUN_TEST(test_clear);
    return UNITY_END();
}