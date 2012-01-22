/*
 * Constants for Renesas μPD78F0730
 *
 * Copyright (c) 2011-2012 Andreas Färber <andreas.faerber@web.de>
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
#ifndef _78K0_USB_UART_H
#define _78K0_USB_UART_H


#include <stdbool.h>
#include <stdint.h>

#ifdef UART_ASYNC_READ
#include <pthread.h>
#endif


struct UART78K0 {
    libusb_device_handle *handle;
#ifdef UART_ASYNC_READ
    uint8_t read_buffer[4096];
    size_t read_buffer_size;
    pthread_mutex_t read_mutex;
    pthread_cond_t read_cond;
#endif
};

enum USB78K0Requests {
    USB_78K0_REQUEST_LINE_CONTROL       = 0x00,
    USB_78K0_REQUEST_SET_DTR_RTS        = 0x01,
    USB_78K0_REQUEST_SET_XON_XOFF_CHR   = 0x02,
    USB_78K0_REQUEST_OPEN_CLOSE         = 0x03,
    USB_78K0_REQUEST_SET_ERR_CHR        = 0x04,
};

struct USB78K0RequestLineControl {
    uint8_t bRequest;
    uint32_t bBaud;
    uint8_t bParams;
} __attribute__((packed));

enum USB78K0LineControlParams {
    USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE     = 0 << 4,
    USB_78K0_LINE_CONTROL_FLOW_CONTROL_HARDWARE = 1 << 4,
    USB_78K0_LINE_CONTROL_FLOW_CONTROL_SOFTWARE = 2 << 4,

    USB_78K0_LINE_CONTROL_PARITY_NONE           = 0 << 2,
    USB_78K0_LINE_CONTROL_PARITY_EVEN           = 1 << 2,
    USB_78K0_LINE_CONTROL_PARITY_ODD            = 2 << 2,

    USB_78K0_LINE_CONTROL_STOP_BITS_1           = 0 << 1,
    USB_78K0_LINE_CONTROL_STOP_BITS_2           = 1 << 1,

    USB_78K0_LINE_CONTROL_DATA_SIZE_7           = 0 << 0,
    USB_78K0_LINE_CONTROL_DATA_SIZE_8           = 1 << 0,
};

struct USB78K0RequestSetDTRRTS {
    uint8_t bRequest;
    uint8_t bParams;
} __attribute__((packed));

enum USB78K0SetDTRRTSParams {
    USB_78K0_SET_DTR_RTS_DTR_OFF    = 0 << 1,
    USB_78K0_SET_DTR_RTS_DTR_ON     = 1 << 1,

    USB_78K0_SET_DTR_RTS_RTS_OFF    = 0 << 0,
    USB_78K0_SET_DTR_RTS_RTS_ON     = 1 << 0,
};

struct USB78K0RequestSetXonXoffChr {
    uint8_t bRequest;
    char XonChr;
    char XoffChr;
} __attribute__((packed));

struct USB78K0RequestOpenClose {
    uint8_t bRequest;
    uint8_t bOpen;
} __attribute__((packed));

enum USB78K0OpenCloseParams {
    USB_78K0_OPEN_CLOSE_CLOSED  = 0x00,
    USB_78K0_OPEN_CLOSE_OPENED  = 0x01,
};

struct USB78K0RequestSetErrChr {
    uint8_t bRequest;
    uint8_t bOpen;
    char ErrChr;
} __attribute__((packed));

enum USB78K0SetErrChrParams {
    USB_78K0_SET_ERR_CHR_DISABLED   = 0x00,
    USB_78K0_SET_ERR_CHR_ENABLED    = 0x01,
};


int usb_78k0_init(struct UART78K0 *uart);

int v850j_78k0_line_control(struct UART78K0 *uart, uint32_t baud_rate, uint8_t params);
int v850j_78k0_set_dtr_rts_bits(struct UART78K0 *uart, uint8_t bits);
int v850j_78k0_set_dtr_rts(struct UART78K0 *uart, bool dtr, bool rts);
int v850j_78k0_set_xon_xoff_chr(struct UART78K0 *uart, char xon, char xoff);
int v850j_78k0_open_close(struct UART78K0 *uart, bool open);
int v850j_78k0_set_err_chr(struct UART78K0 *uart, bool open, char err);

int usb_78k0_write(struct UART78K0 *uart, uint8_t *data, int length, int *transferred, int timeout);
int usb_78k0_read(struct UART78K0 *uart, uint8_t *data, int length, int *transferred, int timeout);


#endif
