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
#ifndef DATA_LAYER_H
#define DATA_LAYER_H

/**
 * @brief configuration for the data layer
 *
 */
struct whisper_data_layer__config
{
    /** receive buffer */
    unsigned char *buf;
    /** length of the receive buffer */
    unsigned char buf_len;
    /** write data to the serial port */
    void (*write)(unsigned char *buf, unsigned char buf_len);
    /** callback for parsed packet */
    void (*packet_received_cb)(unsigned char payload_len);
    /** function pointer for sending data out*/
    void (*data_write)(unsigned char *buf, unsigned char buf_len);
};

/** intialize the data layer with provided backend buffer */
void whisper_data_layer__init(struct whisper_data_layer__config *config);

/**
 * @brief feed data, which from the serial port, to the data layer
 *
 * @param data data from serial port
 * @param data_length the length of the passed data
 * @return char 0 success, otherwise error
 */
char whisper_data_layer__data_received(const unsigned char *data, unsigned char data_length);

/**
 * @brief copy the payload to the given buffer
 *
 * @param dest buffer to receive the payload
 * @param max_len maximum length of the buffer
 * @return unsigned short bytes copied to the buffer, zero means no data available
 */
short whisper_data_layer__copy_payload(unsigned char *dest, unsigned short max_len);

#endif // DATA_LAYER_H