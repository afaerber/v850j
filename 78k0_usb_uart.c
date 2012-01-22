/*
 * Helpers for communicating with Renesas μPD78F0730
 *
 * Copyright (c) 2011-2012 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU LGPL version 2.1 or (at your option) any later version.
 */
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include "v850j.h"
//#define UART_ASYNC_READ
#include "78k0_usb_uart.h"
#include "bswap.h"

#define TIMEOUT_MS 1000

#define RETRY_MAX 5
#define ENDPOINT_OUT 0x02
#define ENDPOINT_IN  0x81


static void dump_request(uint8_t *req, int len)
{
    printf("78K0 request:");
    for (int i = 0; i < len; i++) {
        printf(" %02" PRIX8, req[i]);
    }
    printf("\n");
}

#ifdef UART_ASYNC_READ
static void usb_78k0_read_callback(struct libusb_transfer *transfer)
{
    struct UART78K0 *uart = transfer->user_data;
    if (transfer->status == LIBUSB_TRANSFER_COMPLETED) {
        pthread_mutex_lock(&uart->read_mutex);
        if (uart->read_buffer_size < sizeof(uart->read_buffer)) {
            int size = (sizeof(uart->read_buffer) - uart->read_buffer_size > transfer->actual_length)
                       ? transfer->actual_length : (sizeof(uart->read_buffer) - uart->read_buffer_size);
            memcpy(uart->read_buffer + uart->read_buffer_size, transfer->buffer, size);
            uart->read_buffer_size += size;
        }
        pthread_cond_signal(&uart->read_cond);
        pthread_mutex_unlock(&uart->read_mutex);
        int ret = libusb_submit_transfer(transfer);
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "Resubmitting transfer failed: %d\n", ret);
        }
    } else {
        fprintf(stderr, "Transfer failed: %d\n", transfer->status);
    }
}

static void *usb_78k0_read_loop(void *opaque)
{
    struct UART78K0 *uart = opaque;
    struct libusb_transfer *transfers[2];
    uint8_t buffers[2][64];
    int ret;

    for (int i = 0; i < 2; i++) {
        transfers[i] = libusb_alloc_transfer(0);
        libusb_fill_bulk_transfer(transfers[i], uart->handle, ENDPOINT_IN,
                                  buffers[i], 64, usb_78k0_read_callback,
                                  uart, 0);
        ret = libusb_submit_transfer(transfers[i]);
    }

    while (true) {
        ret = libusb_handle_events(NULL);
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "Handling events failed: %d\n", ret);
        }
    }
    return NULL;
}
#endif

int usb_78k0_init(struct UART78K0 *uart)
{
#ifdef UART_ASYNC_READ
    pthread_mutex_init(&uart->read_mutex, NULL);
    pthread_cond_init(&uart->read_cond, NULL);
    uart->read_buffer_size = 0;

    pthread_t thread;
    int ret = pthread_create(&thread, NULL, usb_78k0_read_loop, uart);
    return ret;
#else
    return 0;
#endif
}

int v850j_78k0_line_control(struct UART78K0 *uart, uint32_t baud_rate, uint8_t params)
{
    struct USB78K0RequestLineControl req;
    req.bRequest = USB_78K0_REQUEST_LINE_CONTROL;
    req.bBaud = cpu_to_le32(baud_rate);
    req.bParams = params;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(uart->handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_dtr_rts_bits(struct UART78K0 *uart, uint8_t bits)
{
    struct USB78K0RequestSetDTRRTS req;
    req.bRequest = USB_78K0_REQUEST_SET_DTR_RTS;
    req.bParams = bits;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(uart->handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_dtr_rts(struct UART78K0 *uart, bool dtr, bool rts)
{
    return v850j_78k0_set_dtr_rts_bits(uart,
        (dtr ? USB_78K0_SET_DTR_RTS_DTR_ON : 0) |
        (rts ? USB_78K0_SET_DTR_RTS_RTS_ON : 0));
}

int v850j_78k0_set_xon_xoff_chr(struct UART78K0 *uart, char xon, char xoff)
{
    struct USB78K0RequestSetXonXoffChr req;
    req.bRequest = USB_78K0_REQUEST_SET_XON_XOFF_CHR;
    req.XonChr = xon;
    req.XoffChr = xoff;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(uart->handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_open_close(struct UART78K0 *uart, bool open)
{
    struct USB78K0RequestOpenClose req;
    req.bRequest = USB_78K0_REQUEST_OPEN_CLOSE;
    req.bOpen = open ? USB_78K0_OPEN_CLOSE_OPENED : USB_78K0_OPEN_CLOSE_CLOSED;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(uart->handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_err_chr(struct UART78K0 *uart, bool open, char err)
{
    struct USB78K0RequestSetErrChr req;
    req.bRequest = USB_78K0_REQUEST_SET_ERR_CHR;
    req.bOpen = open ? USB_78K0_SET_ERR_CHR_ENABLED : USB_78K0_SET_ERR_CHR_DISABLED;
    req.ErrChr = err;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(uart->handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}


int usb_78k0_write(struct UART78K0 *uart, uint8_t *data, int length, int *transferred, int timeout)
{
    uint8_t endpoint = ENDPOINT_OUT;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(uart->handle, endpoint, data, length,
                                   transferred, timeout);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(uart->handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    return ret;
}

int usb_78k0_read(struct UART78K0 *uart, uint8_t *buf, int length, int *transferred, int timeout_ms)
{
#ifndef UART_ASYNC_READ
    uint8_t endpoint = ENDPOINT_IN;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(uart->handle, endpoint, buf, length,
                                   transferred, timeout_ms);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(uart->handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    return ret;
#else
    struct timeval start;
    gettimeofday(&start, NULL);
    struct timespec timeout;
    timeout.tv_sec = start.tv_sec + timeout_ms / 1000;
    timeout.tv_nsec = start.tv_usec * 1000 + (timeout_ms % 1000) * 1000000;

    int ret = 0;
    *transferred = 0;
    while (*transferred < length && ret == 0) {
        pthread_mutex_lock(&uart->read_mutex);
        while (uart->read_buffer_size == 0) {
            ret = pthread_cond_timedwait(&uart->read_cond, &uart->read_mutex, &timeout);
            if (ret == ETIMEDOUT) {
                break;
            }
        }
        int size = (uart->read_buffer_size > length - *transferred) ? (length - *transferred) : uart->read_buffer_size;
        memcpy(buf + *transferred, uart->read_buffer, size);
        *transferred += size;
        uart->read_buffer_size -= size;
        if (uart->read_buffer_size > 0) {
            memmove(uart->read_buffer, uart->read_buffer + size, uart->read_buffer_size);
        }
        pthread_mutex_unlock(&uart->read_mutex);
    }
    return ret;
#endif
}
