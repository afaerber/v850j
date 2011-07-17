/*
 * Helpers for flash programming Renesas V850ES/Jx3-L devices
 *
 * Copyright (c) 2011 Andreas Färber <andreas.faerber@web.de>
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

#define RETRY_MAX 5
#define ENDPOINT_OUT 0x02
#define ENDPOINT_IN  0x81

static int send_command_frame(libusb_device_handle *handle, uint8_t command,
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

    uint8_t endpoint = ENDPOINT_OUT;
    int transferred;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, buf, buffer_length + 5,
                                   &transferred, V850J_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
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

static int receive_data_frame(libusb_device_handle *handle, uint8_t *buffer, size_t *length)
{
    uint8_t buf[2 + 256 + 2];
    uint8_t endpoint = ENDPOINT_IN;
    int transferred = 0;
    int ret;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, buf, 2,
                                   &transferred, V850J_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: receiving header failed: %d (transferred %d)\n", __func__, ret, transferred);
        return -1;
    }
    if (buf[0] != V850ESJx3L_STX) {
        fprintf(stderr, "%s: no data frame: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    if (transferred < 2) {
        try = 0;
        do {
            ret = libusb_bulk_transfer(handle, endpoint, buf + 1, 1,
                                       &transferred, V850J_TIMEOUT_MS);
            if (ret == LIBUSB_ERROR_PIPE) {
                libusb_clear_halt(handle, endpoint);
            }
            try++;
        } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
        if (ret != LIBUSB_SUCCESS) {
            fprintf(stderr, "%s: receiving length failed: %d\n", __func__, ret);
            return -1;
        }
    }
    size_t len = (buf[1] == 0) ? 256 : buf[1];

    int received = 0;
    do {
        try = 0;
        do {
            ret = libusb_bulk_transfer(handle, endpoint, buf + 2 + received, len + 2 - received,
                                       &transferred, V850J_TIMEOUT_MS);
            if (ret == LIBUSB_ERROR_PIPE) {
                libusb_clear_halt(handle, endpoint);
            }
            try++;
        } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
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
    //usleep(tCOM * 1000);
}

int v850j_reset(libusb_device_handle *handle)
{
    useconds_t t12 = (30000.0 / fxx()) * 1000000;
    printf("t12 = %u\n", t12);
    useconds_t t2C = (30000.0 / fxx()) * 1000000;
    printf("t2C = %u\n", t2C);

    wait_tCOM();

    int ret;
    uint8_t endpoint = ENDPOINT_OUT;
    uint8_t x = 0x00;
    int transferred;
    int try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, &x, 1,
                                   &transferred, V850J_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending (i) failed: %d\n", __func__, ret);
        return -1;
    }
    usleep(t12);

    x = 0x00;
    try = 0;
    do {
        ret = libusb_bulk_transfer(handle, endpoint, &x, 1,
                                   &transferred, V850J_TIMEOUT_MS);
        if (ret == LIBUSB_ERROR_PIPE) {
            libusb_clear_halt(handle, endpoint);
        }
        try++;
    } while ((ret == LIBUSB_ERROR_PIPE) && (try < RETRY_MAX));
    if (ret != LIBUSB_SUCCESS) {
        fprintf(stderr, "%s: sending (ii) failed: %d\n", __func__, ret);
        return -1;
    }
    usleep(t2C);

    ret = send_command_frame(handle, V850ESJx3L_RESET, NULL, 0);
    if (ret != 0)
        return ret;
    uint8_t buf[256];
    size_t len;
    ret = receive_data_frame(handle, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    return 0;
}

int v850j_get_silicon_signature(libusb_device_handle *handle)
{
    wait_tCOM();

    int ret;
    ret = send_command_frame(handle, V850ESJx3L_SILICON_SIGNATURE, NULL, 0);
    if (ret != 0)
        return ret;
    uint8_t buf[256];
    size_t len;
    ret = receive_data_frame(handle, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    ret = receive_data_frame(handle, buf, &len);
    if (ret != 0)
        return ret;
    return 0;
}

int v850j_osc_frequency_set(libusb_device_handle *handle, uint32_t frequency)
{
    wait_tCOM();

    int ret;
    uint8_t buf[256];
    // TODO use param
    buf[0] = 5;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 4;
    ret = send_command_frame(handle, V850ESJx3L_OSC_FREQUENCY_SET, buf, 4);
    if (ret != 0)
        return ret;
    size_t len;
    ret = receive_data_frame(handle, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    return 0;
}

/*static void my_cb(struct libusb_transfer *transfer)
{
    printf("wa-hoo! %d (%d)\n", transfer->status, transfer->actual_length);
}*/

int v850j_baud_rate_set(libusb_device_handle *handle, uint32_t baud_rate)
{
    wait_tCOM();

    /*struct libusb_transfer *transfer = libusb_alloc_transfer(0);
    libusb_fill_bulk_transfer(transfer, handle, ENDPOINT_IN, malloc(0x200), 0x200, my_cb, NULL, 3000);
    libusb_submit_transfer(transfer);*/

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
    ret = send_command_frame(handle, V850ESJx3L_BAUD_RATE_SET, buf, 1);
    if (ret != 0)
        return ret;

    ret = v850j_78k0_line_control(handle, baud_rate, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                     USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                     USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                     USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_dtr_rts(handle, false, true);
    ret = v850j_78k0_set_dtr_rts(handle, false, true);
    ret = v850j_78k0_line_control(handle, baud_rate, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                     USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                     USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                     USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_set_xon_xoff_chr(handle, 0x09, 0x00);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');
    ret = v850j_78k0_line_control(handle, baud_rate, USB_78K0_LINE_CONTROL_FLOW_CONTROL_NONE |
                                                     USB_78K0_LINE_CONTROL_PARITY_NONE |
                                                     USB_78K0_LINE_CONTROL_STOP_BITS_1 |
                                                     USB_78K0_LINE_CONTROL_DATA_SIZE_8);
    ret = v850j_78k0_set_err_chr(handle, false, '\0');

    useconds_t tWT10 = (2384.0 / fxx()) * 1000000;
    usleep(tWT10);

    ret = send_command_frame(handle, V850ESJx3L_RESET, NULL, 0);
    if (ret != 0)
        return ret;
    //libusb_handle_events(NULL);
    size_t len;
    ret = receive_data_frame(handle, buf, &len);
    if (ret != 0)
        return ret;
    if (buf[0] != V850ESJx3L_STATUS_ACK) {
        fprintf(stderr, "%s: no ACK: %02" PRIX8 "\n", __func__, buf[0]);
        return -1;
    }
    return 0;
}
