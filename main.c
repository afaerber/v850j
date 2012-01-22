/*
 * Test code for communicating with Renesas V850ES/Jx3-L Starter Kit
 *
 * Copyright (c) 2011-2012 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU GPL version 2 or (at your option) any later version.
 */

#include <stdlib.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include "v850j.h"

static struct V850Device *v850j_open(libusb_context *usb_context)
{
    struct V850Device *dev = malloc(sizeof(struct V850Device));
    dev->uart.handle = libusb_open_device_with_vid_pid(usb_context, USB_VID_NEC, USB_PID_NEC_UART);
    if (dev->uart.handle == NULL)
        return NULL;

    int ret;

    ret = libusb_kernel_driver_active(dev->uart.handle, 0);
    if (ret == 1) {
        printf("kernel driver active\n");
    } else if (ret == 0) {
        //printf("kernel driver not active\n");
    } else {
        fprintf(stderr, "libusb_kernel_driver_active = %d\n", ret);
    }
    ret = libusb_claim_interface(dev->uart.handle, 0);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "claiming interface failed: %d\n", ret);
        libusb_close(dev->uart.handle);
        return NULL;
    }

    return dev;
}

static void v850j_close(struct V850Device *dev)
{
    libusb_release_interface(dev->uart.handle, 0);
    libusb_close(dev->uart.handle);
}

static void test(struct V850Device *dev)
{
    int ret;

    libusb_clear_halt(dev->uart.handle, 0x02);
    libusb_clear_halt(dev->uart.handle, 0x81);
    ret = usb_78k0_init(&dev->uart);
    printf("Doing control transfers...\n");
    ret = v850j_78k0_open_close(&dev->uart, true);
    ret = v850j_78k0_set_dtr_rts(&dev->uart, true, true);

    uint8_t line_settings = USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                            USB_78K0_LINE_CONTROL_PARITY_NONE |
                            USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                            USB_78K0_LINE_CONTROL_DATA_SIZE_8;
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_dtr_rts(&dev->uart, true, false);
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, false);
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(&dev->uart, 0xfa, 0xcf);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, false);
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(&dev->uart, 0xfd, 0xff);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, false);
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, false);
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(&dev->uart, 0x00, 0x00);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, true);
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, true);
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(&dev->uart, 0x01, 0x00);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, 9600, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');

    printf("Resetting...\n");
    ret = v850j_reset(dev);
    if (ret != 0)
        return;
    printf("Setting oscillation frequency...\n");
    ret = v850j_osc_frequency_set(dev, 5000000);
    if (ret != 0)
        return;
    printf("Setting baud rate...\n");
    ret = v850j_baud_rate_set(dev, 9600);
    //ret = v850j_baud_rate_set(dev, 38400);
    //ret = v850j_baud_rate_set(dev, 115200);
    if (ret != 0)
        return;
    printf("Getting silicon signature...\n");
    ret = v850j_get_silicon_signature(dev);
    if (ret != 0)
        return;
}

static void connect(libusb_context *usb_context)
{
    printf("Opening V850ES/Jx3-L device...\n");
    struct V850Device *dev = v850j_open(usb_context);
    if (dev == NULL) {
        fprintf(stderr, "Opening the device failed.\n");
        return;
    }

    // Avoid having to re-plug device for reproducible results
    int ret = libusb_reset_device(dev->uart.handle);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "Resetting device failed: %d\n", ret);
        return;
    }

    test(dev);
    v850j_78k0_open_close(&dev->uart, false);
    v850j_close(dev);
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
