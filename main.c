/*
 * Test code for communicating with Renesas V850ES/Jx3-L Starter Kit
 *
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU GPL version 2 or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include "v850j.h"
#include "78k0_usb_uart.h"

static libusb_device_handle *v850j_open(libusb_context *usb_context)
{
    libusb_device_handle *handle;
    handle = libusb_open_device_with_vid_pid(usb_context, USB_VID_NEC, USB_PID_NEC_UART);
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

static void v850j_close(libusb_device_handle *handle)
{
    libusb_release_interface(handle, 0);
    libusb_close(handle);
}

static void test(libusb_device_handle *handle)
{
    int ret;

    libusb_clear_halt(handle, 0x02);
    libusb_clear_halt(handle, 0x81);
    printf("Doing control transfers...\n");
    ret = v850j_78k0_open_close(handle, true);
    ret = v850j_78k0_set_dtr_rts(handle, true, true);

    int transferred;
    uint8_t b[0x200];
    ret = libusb_bulk_transfer(handle, 0x81, b, 0x200, &transferred, 1);
    if (ret != 0) {
        fprintf(stderr, "%s: reading failed: %d\n", __func__, ret);
    }

    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_dtr_rts(handle, true, false);
    ret = v850j_78k0_set_dtr_rts(handle, false, false);
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(handle, 0xfa, 0xcf);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_dtr_rts(handle, false, false);
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(handle, 0xfd, 0xff);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_dtr_rts(handle, false, false);
    ret = v850j_78k0_set_dtr_rts(handle, false, false);
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(handle, 0x00, 0x00);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_dtr_rts(handle, false, true);
    ret = v850j_78k0_set_dtr_rts(handle, false, true);
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(handle, 0x01, 0x00);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, 9600, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');

    printf("Resetting...\n");
    ret = v850j_reset(handle);
    if (ret != 0)
        return;
    ret = v850j_get_silicon_signature(handle);
    if (ret != 0)
        return;
}

static void connect(libusb_context *usb_context)
{
    printf("Opening V850ES/Jx3-L device...\n");
    libusb_device_handle *handle = v850j_open(usb_context);
    if (handle == NULL) {
        fprintf(stderr, "Opening the device failed.\n");
        return;
    }
    test(handle);
    v850j_close(handle);
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
