/*
 * Constants for Renesas V850ES/Jx3-L
 *
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef V850J_H
#define V850J_H


#include <stdint.h>


#define USB_VID_NEC 0x0409
#define USB_PID_NEC_UART 0x0063

enum V850ESJx3LFrameBytes {
    V850ESJx3L_SOH = 0x01,
    V850ESJx3L_STX = 0x02,
    V850ESJx3L_ETB = 0x17,
    V850ESJx3L_ETX = 0x03,
};

enum V850ESJx3LCommands {
    V850ESJx3L_RESET                = 0x00,
    V850ESJx3L_VERIFY               = 0x13,
    V850ESJx3L_CHIP_ERASE           = 0x20,
    V850ESJx3L_BLOCK_ERASE          = 0x22,
    V850ESJx3L_BLOCK_BLANK_CHECK    = 0x32,
    V850ESJx3L_PROGRAMMING          = 0x40,
    V850ESJx3L_READ                 = 0x50,
    V850ESJx3L_STATUS               = 0x70,
    V850ESJx3L_OSC_FREQUENCY_SET    = 0x90,
    V850ESJx3L_BAUD_RATE_SET        = 0x9a,
    V850ESJx3L_SECURITY_GET         = 0xa0,
    V850ESJx3L_CHECKSUM             = 0xb0,
    V850ESJx3L_SILICON_SIGNATURE    = 0xc0,
    V850ESJx3L_VERSION_GET          = 0xc5,
};

enum V850ESJx3LStatus {
    V850ESJx3L_STATUS_COMMAND_ERROR     = 0x04,
    V850ESJx3L_STATUS_PARAM_ERROR       = 0x05,
    V850ESJx3L_STATUS_ACK               = 0x06,
    V850ESJx3L_STATUS_CHECKSUM_ERROR    = 0x07,
    V850ESJx3L_STATUS_VERIFY_ERROR      = 0x0f,
    V850ESJx3L_STATUS_PROTECT_ERROR     = 0x10,
    V850ESJx3L_STATUS_NACK              = 0x15,
    V850ESJx3L_STATUS_MRG10_ERROR       = 0x1a,
    V850ESJx3L_STATUS_MRG11_ERROR       = 0x1b,
    V850ESJx3L_STATUS_WRITE_ERROR       = 0x1c,
    V850ESJx3L_STATUS_READ_ERROR        = 0x20,
    V850ESJx3L_STATUS_BUSY              = 0xff,
};


int v850j_reset(libusb_device_handle *handle);
int v850j_get_silicon_signature(libusb_device_handle *handle);
int v850j_osc_frequency_set(libusb_device_handle *handle, uint32_t frequency);
int v850j_baud_rate_set(libusb_device_handle *handle, uint32_t baud_rate);


#endif
