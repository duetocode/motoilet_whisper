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
#include "ring_buffer.h"
#include <stdlib.h>

struct ring_buffer
{
    unsigned char *buf;
    unsigned short int buf_len;
    unsigned short int head;
    unsigned short int size;
};

ring_buffer_t ring_buffer_create(unsigned char *buf, unsigned short buf_len)
{
    ring_buffer_t rb = (ring_buffer_t)malloc(sizeof(struct ring_buffer));
    rb->buf = buf;
    rb->buf_len = buf_len;
    rb->head = 0;
    rb->size = 0;
    return rb;
}

unsigned short ring_buffer_capacity(ring_buffer_t rb) { return rb->buf_len; }

unsigned short ring_buffer_size(ring_buffer_t rb) { return rb->size; }

unsigned char *ring_buffer_at(ring_buffer_t rb, unsigned short index)
{
    return &rb->buf[(rb->head + index) % rb->buf_len];
}

unsigned short ring_buffer_push(ring_buffer_t rb, const unsigned char *src,
                                unsigned short count)
{
    unsigned short bytes_to_copy = rb->buf_len - rb->size;
    if (bytes_to_copy > count)
        bytes_to_copy = count;

    for (unsigned short i = 0; i < bytes_to_copy; ++i)
    {
        *ring_buffer_at(rb, rb->size) = src[i];
        rb->size += 1;
    }

    return bytes_to_copy;
}

void ring_buffer_pop(ring_buffer_t rb)
{
    if (rb->size > 0)
    {
        rb->size -= 1;
        rb->head = (rb->head + 1) % rb->buf_len;
    }
}

ring_buffer_t ring_buffer_clear(ring_buffer_t rb)
{
    rb->size = 0;
    return rb;
}

unsigned short ring_buffer_batch_pop(ring_buffer_t rb, unsigned short count)
{
    if (count > rb->size)
        count = rb->size;

    rb->head = (rb->head + count) % rb->buf_len;
    rb->size -= count;

    return count;
}

unsigned char ring_buffer_read(unsigned char *dest, ring_buffer_t src, unsigned short offset, unsigned short count)
{
    if (count > src->size)
        count = src->size;

    for (unsigned short i = 0; i < count; ++i)
    {
        dest[i] = src->buf[(src->head + offset + i) % src->buf_len];
    }

    return count;
}