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
#include "array_buffer.h"
#include <assert.h>
#include <string.h>
#include "basic_data_type.h"

struct array_buffer
{
    uint8_t capacity;
    uint8_t size;
    uint8_t *buf;
};

const uint8_t SIZEOF_ARRAY_BUFFER_T = sizeof(struct array_buffer);

void array_buffer__init(array_buffer_t ab, uint8_t *buf, uint8_t buf_len)
{
    ab->buf = buf;
    ab->capacity = buf_len;
    ab->size = 0;
}

uint8_t array_buffer__clear(array_buffer_t ab)
{
    uint8_t limit = ab->size;
    ab->size = 0;
    return limit;
}

uint8_t *array_buffer__at(array_buffer_t ab, uint8_t index)
{
    return &ab->buf[index];
}

uint8_t array_buffer__push(array_buffer_t ab, const uint8_t *src, uint8_t count)
{
    uint8_t bytes_to_copy = ab->capacity - ab->size;
    if (bytes_to_copy > count)
        bytes_to_copy = count;

    memcpy(&ab->buf[ab->size], src, bytes_to_copy);
    ab->size += bytes_to_copy;

    return bytes_to_copy;
}

uint8_t array_buffer__pop(array_buffer_t ab, uint8_t count)
{
    if (count == 0)
        return 0;

    if (count >= ab->size)
        return array_buffer__clear(ab);

    memmove(ab->buf, &ab->buf[count], ab->size - count);
    ab->size -= count;

    return count;
}

uint8_t array_buffer__capacity(array_buffer_t ab)
{
    return ab->capacity;
}

uint8_t array_buffer__size(array_buffer_t ab)
{
    return ab->size;
}