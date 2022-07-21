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
#ifndef DATA_LAYER_H
#define DATA_LAYER_H

#include <basic_data_type.h>

/**
 * @brief configuration for the data layer
 *
 */
struct whisper_data_layer__config
{
    /** receive buffer */
    uint8_t *buf;
    /** length of the receive buffer */
    uint8_t buf_len;
    /** callback for parsed packet */
    void (*packet_received_cb)(uint8_t *payload, uint8_t payload_len);
    /** function pointer for sending data out*/
    void (*data_write)(const uint8_t *payload, uint8_t payload_len);
    /** callback for data acknowledgement */
    void (*data_ack_cb)(unsigned int seq_no, uint8_t sent);
    void (*set_delay)(uint16_t delay_in_ms, void (*delay_cb)(void));
    void (*cancel_delay)(void);
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
char whisper_data_layer__data_received(const uint8_t *data, uint8_t data_length);

/**
 * @brief send data out
 *
 * @param data data to send
 * @param data_length the length of the data
 * @param ack_required whether the data is ack required
 * @return the sequence no of the sent packet
 */
uint16_t whisper_data_layer__data_sent(uint8_t *data, uint8_t data_length, uint8_t ack_required);

#endif // DATA_LAYER_H