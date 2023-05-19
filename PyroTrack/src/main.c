/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include "lib_sensirion/svm41_i2c.h"

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/services/bas.h>

#include "temp.h"
#include "hum.h"
#include "voc.h"
#include "nox.h"
#include "fire.h"

static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL,
                  BT_UUID_16_ENCODE(BT_UUID_HTS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL),
                  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL)),
};

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err)
    {
        printk("Connection failed (err 0x%02x)\n", err);
    }
    else
    {
        printk("Connected\n");
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("Disconnected (reason 0x%02x)\n", reason);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
};

static void bt_ready(void)
{
    int err;

    printk("Bluetooth initialized\n");

    // hts_init();
    temp_init();
    hum_init();
    voc_init();
    nox_init();
    fire_init();

    err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
    if (err)
    {
        printk("Advertising failed to start (err %d)\n", err);
        return;
    }

    printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
    .cancel = auth_cancel,
};

static void bas_notify(void)
{
    uint8_t battery_level = bt_bas_get_battery_level();

    battery_level--;

    if (!battery_level)
    {
        battery_level = 100U;
    }

    bt_bas_set_battery_level(battery_level);
}

void main(void)
{
    int err;

    err = bt_enable(NULL);
    if (err)
    {
        printk("Bluetooth init failed (err %d)\n", err);
        return;
    }

    bt_ready();

    bt_conn_auth_cb_register(&auth_cb_display);

    int16_t error = 0;

    sensirion_i2c_hal_init();

    error = svm41_device_reset();
    if (error)
    {
        printf("Error executing svm41_device_reset(): %i\n", error);
    }

    unsigned char serial_number[26];
    uint8_t serial_number_size = 26;
    error = svm41_get_serial_number(serial_number, serial_number_size);
    if (error)
    {
        printf("Error executing svm41_get_serial_number(): %i\n", error);
    }
    else
    {
        printf("Serial number: %s\n", serial_number);
    }

    uint8_t firmware_major;
    uint8_t firmware_minor;
    bool firmware_debug;
    uint8_t hardware_major;
    uint8_t hardware_minor;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    error = svm41_get_version(&firmware_major, &firmware_minor, &firmware_debug,
                              &hardware_major, &hardware_minor, &protocol_major,
                              &protocol_minor);

    if (error)
    {
        printf("Error executing svm41_get_version(): %i\n", error);
    }
    else
    {
        printf("Firmware: %i.%i Debug: %i\n", firmware_major, firmware_minor,
               firmware_debug);
        printf("Hardware: %i.%i\n", hardware_major, hardware_minor);
        printf("Protocol: %i.%i\n", protocol_major, protocol_minor);
    }

    int16_t t_offset;
    error = svm41_get_temperature_offset_for_rht_measurements(&t_offset);
    if (error)
    {
        printf("Error executing "
               "svm41_get_temperature_offset_for_rht_measurements(): %i\n",
               error);
    }
    else
    {
        printf("Temperature Offset: %i ticks\n", t_offset);
    }

    // Start Measurement
    error = svm41_start_measurement();
    if (error)
    {
        printf("Error executing svm41_start_measurement(): %i\n", error);
    }

    for (;;)
    {
        // Read Measurement
        sensirion_i2c_hal_sleep_usec(10000000); // Delay 10s
        int16_t humidity;
        int16_t temperature;
        int16_t voc_index;
        int16_t nox_index;
        int16_t fire;
        error = svm41_read_measured_values_as_integers(&humidity, &temperature,
                                                       &voc_index, &nox_index);
        if (error)
        {
            printf("Error executing svm41_read_measured_values_as_integers(): "
                   "%i\n",
                   error);
        }
        else
        {
            printf("Humidity: %i milli %% RH\n", humidity * 10);
            printf("Temperature: %i milli °C\n", (temperature >> 1) * 10);
            printf("VOC index: %i (index * 10)\n", voc_index);
            printf("NOx index: %i (index * 10)\n", nox_index);

            if (((temperature >> 1) / 100)>65 && humidity<40 && voc_index>2000){
                fire = 1;
            }else{
                fire = 0;
            }

            temp_indicate((temperature >> 1) / 100);
            hum_indicate((humidity)/100);
            voc_indicate(voc_index);
            nox_indicate(nox_index);
            fire_indicate(fire);

            
            /* Battery level simulation */
            bas_notify();
        }
        k_sleep(K_SECONDS(1));
    }

    error = svm41_stop_measurement();

    if (error)
    {
        printf("Error executing svm41_stop_measurement(): %i\n", error);
    }

    return 0;
}