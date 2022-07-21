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
#ifndef ARRAY_BUFFER_H
#define ARRAY_BUFFER_H

#include "basic_data_type.h"

typedef struct array_buffer *array_buffer_t;

extern const uint8_t SIZEOF_ARRAY_BUFFER_T;

/** Query the size of the array_buffer_t type */
uint8_t sizeof_array_buffer_t(void);

/**
 * @brief Initialize an array buffer with the given buffer and buffer length.
 *
 * @param buf data buffer to use as the backend of the buffer
 * @param buf_len length of the data buffer, which will be the capacity of the buffer
 */
void array_buffer__init(array_buffer_t ab, uint8_t *buf, uint8_t buf_len);

/** remove all elements in the buffer */
uint8_t array_buffer__clear(array_buffer_t ab);

uint8_t *array_buffer__at(array_buffer_t ab, uint8_t index);
uint8_t array_buffer__push(array_buffer_t ab, const uint8_t *src, uint8_t count);

/**
 * @brief Remove elements from the head of the buffer
 *
 * @param ab the operation buffer
 * @param count number of elements to remove
 * @return uint8_t actual number of elements removed
 */
uint8_t array_buffer__pop(array_buffer_t ab, uint8_t count);

/** return the capacity of the buffer, which is the length of the underlying array */
uint8_t array_buffer__capacity(array_buffer_t ab);
/** return the length of data in the buffer */
uint8_t array_buffer__size(array_buffer_t ab);

#endif // ARRAY_BUFFER_H