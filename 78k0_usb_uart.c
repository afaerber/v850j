/*
 * Helpers for communicating with Renesas μPD78F0730
 *
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU LGPL version 2.1 or (at your option) any later version.
 */
#include <inttypes.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include "v850j.h"
#include "78k0_usb_uart.h"
#include "bswap.h"

#define TIMEOUT_MS 1000

static void dump_request(uint8_t *req, int len)
{
    printf("78K0 request:");
    for (int i = 0; i < len; i++) {
        printf(" %02" PRIX8, req[i]);
    }
    printf("\n");
}

int v850j_78k0_line_control(libusb_device_handle *handle, uint32_t baud_rate, uint8_t params)
{
    struct USB78K0RequestLineControl req;
    req.bRequest = USB_78K0_REQUEST_LINE_CONTROL;
    req.bBaud = cpu_to_le32(baud_rate);
    req.bParams = params;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_dtr_rts(libusb_device_handle *handle, bool dtr, bool rts)
{
    struct USB78K0RequestSetDTRRTS req;
    req.bRequest = USB_78K0_REQUEST_SET_DTR_RTS;
    req.bParams = (dtr ? USB_78K0_SET_DTR_RTS_DTR_ON : 0) |
                  (rts ? USB_78K0_SET_DTR_RTS_RTS_ON : 0);
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_xon_xoff_chr(libusb_device_handle *handle, char xon, char xoff)
{
    struct USB78K0RequestSetXonXoffChr req;
    req.bRequest = USB_78K0_REQUEST_SET_XON_XOFF_CHR;
    req.XonChr = xon;
    req.XoffChr = xoff;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_open_close(libusb_device_handle *handle, bool open)
{
    struct USB78K0RequestOpenClose req;
    req.bRequest = USB_78K0_REQUEST_OPEN_CLOSE;
    req.bOpen = open ? USB_78K0_OPEN_CLOSE_OPENED : USB_78K0_OPEN_CLOSE_CLOSED;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}

int v850j_78k0_set_err_chr(libusb_device_handle *handle, bool open, char err)
{
    struct USB78K0RequestSetErrChr req;
    req.bRequest = USB_78K0_REQUEST_SET_ERR_CHR;
    req.bOpen = open ? USB_78K0_SET_ERR_CHR_ENABLED : USB_78K0_SET_ERR_CHR_DISABLED;
    req.ErrChr = err;
    dump_request((uint8_t *)&req, sizeof(req));
    return libusb_control_transfer(handle, 0x40, 0x00, 0, 0, (unsigned char *)&req, sizeof(req), TIMEOUT_MS);
}