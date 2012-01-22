/*
 * Helpers for flash programming Renesas V850ES/Jx3-L devices
 *
 * Copyright (c) 2011-2012 Andreas Färber <andreas.faerber@web.de>
 *
 * Licensed under the GNU LGPL version 2.1 or (at your option) any later version.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include "v850j.h"
#include "78k0_usb_uart.h"

#define V850J_TIMEOUT_MS (3000 + 1000)

static uint8_t checksum(uint8_t *data, size_t data_length)
{
    uint8_t checksum = 0x00;
    for (size_t i = 0; i < data_length; i++) {
        checksum -= data[i];
    }
    return checksum;
}

static int send_command_frame(struct V850Device *dev, uint8_t command,
                              const uint8_t *buffer, uint8_t buffer_length)
{
    uint8_t buf[3 + 255 + 2];
    buf[0] = V850ESJx3L_SOH;
    buf[1] = (buffer_length == 255) ? 0 : (buffer_length + 1);
    buf[2] = command;
    if (buffer_length > 0)
        memcpy(&buf[3], buffer, buffer_length);
    buf[3 + buffer_length] = checksum(&buf[1], buffer_length + 2);
    buf[3 + buffer_length + 1] = V850ESJx3L_ETX;

    printf("Sending command frame:");
    for(int i = 0; i < buffer_length + 5; i++) {
        printf(" %02" PRIX8, buf[i]);
    }
    printf("\n");

    int transferred;
    int ret = usb_78k0_write(&dev->uart, buf, buffer_length + 5, &transferred, V850J_TIMEOUT_MS);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending failed: %d\n", __func__, ret);
        return -1;
    }
    if (transferred != buffer_length + 5) {
        fprintf(stderr, "%s: transferred unexpected amount: %d (%d)\n", __func__, transferred, buffer_length + 5);
        return -1;
    }

    return 0;
}

static int receive_data_frame(struct V850Device *dev, uint8_t *buffer, size_t *length)
{
    uint8_t buf[2 + 256 + 2];
    int transferred = 0;
    int ret = usb_78k0_read(&dev->uart, buf, 2, &transferred, V850J_TIMEOUT_MS);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: receiving header failed: %d (transferred %d)\n", __func__, ret, transferred);
        return -1;
    }
    if (buf[0] != V850ESJx3L_STX) {
        fprintf(stderr, "%s: no data frame: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    if (transferred < 2) {
        ret = usb_78k0_read(&dev->uart, buf + 1, 1, &transferred, V850J_TIMEOUT_MS);
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "%s: receiving length failed: %d\n", __func__, ret);
            return -1;
        }
    }
    size_t len = (buf[1] == 0) ? 256 : buf[1];

    int received = 0;
    do {
        ret = usb_78k0_read(&dev->uart, buf + 2 + received, len + 2 - received, &transferred, V850J_TIMEOUT_MS);
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "%s: receiving data failed: %d\n", __func__, ret);
            return -1;
        }
        if (ret == 0)
            received += transferred;
    } while (received < len + 2);

    if (buf[2 + len] != checksum(buf + 1, len + 1)) {
        fprintf(stderr, "%s: checksum mismatch\n", __func__);
    }

    printf("Received data frame:");
    for(int i = 0; i < len + 4; i++) {
        printf(" %02" PRIX8, buf[i]);
    }
    printf("\n");

    memcpy(buffer, buf + 2, len);
    *length = len;
    return 0;
}

static uint32_t fxx(void)
{
    uint32_t fx = 5000000;
    return fx * 4;
}

static void wait_tCOM(void)
{
    useconds_t tCOM = (620.0 / fxx()) * 1000000 + 15;
    printf("tCOM = %u\n", tCOM);
    usleep(tCOM);
}

int v850j_reset(struct V850Device *dev)
{
    int ret;
    useconds_t t12 = (30000.0 / fxx()) * 1000000;
    printf("t12 = %u\n", t12);
    useconds_t t2C = (30000.0 / fxx()) * 1000000;
    printf("t2C = %u\n", t2C);

    wait_tCOM();
    ret = v850j_78k0_line_control(&dev->uart,
                                  9600,
                                  USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                  USB_78K0_LINE_CONTROL_PARITY_NONE |
                                  USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                  USB_78K0_LINE_CONTROL_DATA_SIZE_8);

    uint8_t x = 0x00;
    int transferred;
    ret = usb_78k0_write(&dev->uart, &x, 1, &transferred, V850J_TIMEOUT_MS);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending (i) failed: %d\n", __func__, ret);
        return -1;
    }
    usleep(t12);

    x = 0x00;
    ret = usb_78k0_write(&dev->uart, &x, 1, &transferred, V850J_TIMEOUT_MS);
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending (ii) failed: %d\n", __func__, ret);
        return -1;
    }
    usleep(t2C);

    ret = send_command_frame(dev, V850ESJx3L_RESET, NULL, 0);
    if (ret != 0)
        return ret;
    uint8_t buf[256];
    size_t len;
    ret = receive_data_frame(dev, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    return 0;
}

int v850j_get_silicon_signature(struct V850Device *dev)
{
    wait_tCOM();

    int ret;
    ret = send_command_frame(dev, V850ESJx3L_SILICON_SIGNATURE, NULL, 0);
    if (ret != 0)
        return ret;
    uint8_t buf[256];
    size_t len;
    ret = receive_data_frame(dev, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    ret = receive_data_frame(dev, buf, &len);
    if (ret != 0)
        return ret;
    char device[11];
    for (int i = 0; i < 10; i++) {
        device[i] = buf[5 + 3 * 4 + i] & 0x7f;
    }
    device[10] = '\0';
    printf("Device: '%s'\n", device);
    return 0;
}

int v850j_osc_frequency_set(struct V850Device *dev, uint32_t frequency)
{
    int ret;
    uint8_t buf[256];
    sprintf((char*)buf, "%" PRIu32, frequency);
    for (int i = 0; i < 256; i++) {
        if (i < 3) {
            buf[i] = buf[i] - '0';
        } else if (buf[i] == '\0') {
            buf[3] = i - 3;
            break;
        } else if (buf[i] != '0') {
            fprintf(stderr, "%s: invalid frequency %" PRId32 "\n", __func__, frequency);
            return -1;
        }
    }

    wait_tCOM();

    ret = send_command_frame(dev, V850ESJx3L_OSC_FREQUENCY_SET, buf, 4);
    if (ret != 0)
        return ret;
    size_t len;
    ret = receive_data_frame(dev, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    return 0;
}

int v850j_baud_rate_set(struct V850Device *dev, uint32_t baud_rate)
{
    wait_tCOM();

    int ret;
    uint8_t buf[256];
    switch (baud_rate) {
    case 9600:
    default:
        buf[0] = 0x03;
        break;
    case 19200:
        buf[0] = 0x04;
        break;
    case 31250:
        buf[0] = 0x05;
        break;
    case 38400:
        buf[0] = 0x06;
        break;
    case 76800:
        buf[0] = 0x07;
        break;
    case 153600:
        buf[0] = 0x08;
        break;
    case 57600:
        buf[0] = 0x09;
        break;
    case 115200:
        buf[0] = 0x0a;
        break;
    case 128000:
        buf[0] = 0x0b;
        break;
    }
    ret = send_command_frame(dev, V850ESJx3L_BAUD_RATE_SET, buf, 1);
    if (ret != 0)
        return ret;

    uint8_t line_settings = USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                            USB_78K0_LINE_CONTROL_PARITY_NONE |
                            USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                            USB_78K0_LINE_CONTROL_DATA_SIZE_8;
    ret = v850j_78k0_line_control(&dev->uart, baud_rate, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, true);
    ret = v850j_78k0_set_dtr_rts(&dev->uart, false, true);
    ret = v850j_78k0_line_control(&dev->uart, baud_rate, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(&dev->uart, 0x09, 0x00);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');
    ret = v850j_78k0_line_control(&dev->uart, baud_rate, line_settings);
    ret = v850j_78k0_set_err_chr(&dev->uart, false, '\0');

    useconds_t tWT10 = (2384.0 / fxx()) * 1000000;
    int try = 0;
    do {
        usleep(tWT10);

        ret = send_command_frame(dev, V850ESJx3L_RESET, NULL, 0);
        if (ret != 0) {
            return -1;
        }
        size_t len;
        ret = receive_data_frame(dev, buf, &len);
        if (ret == 0) {
            if (buf[0] == V850ESJx3L_STATUS_ACK) {
                return 0;
            }
            fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        }
        try++;
    } while (try < 16);
    return -1;
}
