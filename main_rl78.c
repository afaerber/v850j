/*
 * Test code for communicating with Renesas RL78/G13 Promotion Board
 *
 * Copyright (c) 2011-2012 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU GPL version 2 or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include "rl78.h"
#include "78k0_usb_uart.h"

static libusb_device_handle *rl78_open(libusb_context *usb_context)
{
    libusb_device_handle *handle;
    handle = libusb_open_device_with_vid_pid(usb_context, USB_VID_HITACHI, USB_PID_HITACHI_UART);
    if (handle == NULL)
        return NULL;

    int ret;

    ret = libusb_kernel_driver_active(handle, 0);
    if (ret == 1) {
        printf("kernel driver active\n");
    } else if (ret == 0) {
        //printf("kernel driver not active\n");
    } else {
        fprintf(stderr, "libusb_kernel_driver_active = %d\n", ret);
    }
    ret = libusb_claim_interface(handle, 0);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "claiming interface failed: %d\n", ret);
        libusb_close(handle);
        return NULL;
    }

    return handle;
}

static void rl78_close(libusb_device_handle *handle)
{
    libusb_release_interface(handle, 0);
    libusb_close(handle);
}

static void test(libusb_device_handle *handle)
{
    int ret;

    libusb_clear_halt(handle, 0x02);
    libusb_clear_halt(handle, 0x81);
    (void)ret;
}

static void connect(libusb_context *usb_context)
{
    printf("Opening RL78/G13 device...\n");
    libusb_device_handle *handle = rl78_open(usb_context);
    if (handle == NULL) {
        fprintf(stderr, "Opening the device failed.\n");
        return;
    }
    test(handle);
    rl78_close(handle);
}

int main(void)
{
    int ret;
    libusb_context *usb_context;
    ret = libusb_init(&usb_context);
    if (ret != 0) {
        fprintf(stderr, "USB init failed.\n");
        return -1;
    }

    connect(usb_context);

    libusb_exit(usb_context);
    return 0;
}
