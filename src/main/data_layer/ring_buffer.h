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
#ifndef RING_BUFFER_H
#define RING_BUFFER_H

typedef struct ring_buffer *ring_buffer_t;

/**
 * @brief Create a ring buffer instance with the given buffer and length.
 *
 * @param ALLOCATOR A function that allocates memory.
 * @param buf data buffer to use as the backend of the ring buffer
 * @param buf_len length of the data buffer, which will be the capacity of the ring buffer
 */
ring_buffer_t ring_buffer_create(unsigned char *buf, unsigned short buf_len);

/** remove all elements in the ring buffer */
ring_buffer_t ring_buffer_clear(ring_buffer_t rb);

unsigned char *ring_buffer_at(ring_buffer_t rb, unsigned short index);
unsigned short ring_buffer_read(unsigned char *dest, ring_buffer_t src, unsigned short offset, unsigned short count);
unsigned short ring_buffer_push(ring_buffer_t rb, const unsigned char *src, unsigned short count);

/** remove the left element from the ring buffer, if the buffer is not empty */
void ring_buffer_pop(ring_buffer_t rb);

/** return the capacity of the ring buffer, which is the length of the underlying array */
unsigned short ring_buffer_capacity(ring_buffer_t rb);
/** return the length of data in the buffer */
unsigned short ring_buffer_size(ring_buffer_t rb);

/** pop the speicified number of elements from the ring buffer and return the actual number of element removed. */
unsigned short ring_buffer_batch_pop(ring_buffer_t rb, unsigned short count);

#endif // RING_BUFFER_H