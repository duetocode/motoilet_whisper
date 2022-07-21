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
#include "crc.h"

unsigned short update_crc(unsigned char val, unsigned short crc)
{
    unsigned short CRCLSB;
    unsigned char j;
    crc = crc ^ val;
    for (j = 0; j < 8; j++)
    {
        CRCLSB = (unsigned short)(crc & 0x0001);
        crc = (crc >> 1) & 0x7FFF;
        if (CRCLSB == 1)
        {
            crc = crc ^ 0xA001;
        }
    }
    return crc;
}

unsigned short update_crc_buf(const unsigned char *buf, unsigned char len, unsigned short crc)
{
    unsigned short i;
    for (i = 0; i < len; i++)
    {
        crc = update_crc(buf[i], crc);
    }
    return crc;
}