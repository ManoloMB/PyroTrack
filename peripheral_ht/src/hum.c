/** @file
 *  @brief HTS Service sample
 */

/*
 * Copyright (c) 2020 SixOctets Systems
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/*
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>


#ifdef CONFIG_TEMP_NRF5
static const struct device *hum_dev = DEVICE_DT_GET_ANY(nordic_nrf_hum);
#else
static const struct device *hum_dev;
#endif

static uint8_t simulate_hum;
static uint8_t indicating_hum;
static struct bt_gatt_indicate_params ind_params_hum;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	simulate_hum = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	indicating_hum = 0U;
}


// humerature Service Declaration 
BT_GATT_SERVICE_DEFINE(hum_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HUMIDITY),
	BT_GATT_CHARACTERISTIC(BT_UUID_HUMIDITY, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	// more optional Characteristics 
);


void hum_init(void)
{
	if (hum_dev == NULL || !device_is_ready(hum_dev)) {
		printk("no temperature device; using simulated data\n");
		hum_dev = NULL;
	} else {
		printk("hum device is %p, name is %s\n", hum_dev,
		       hum_dev->name);
	}
}

void hum_indicate(void)
{
	// Temperature measurements simulation 
	struct sensor_value hum_value;

	if (simulate_hum) {
		static uint8_t htm[5];
		static double humedity = 50.0;
		uint32_t mantissa;
		uint8_t exponent;
		int r;

		if (indicating_hum) {
			return;
		}

		if (!hum_dev) {
			humedity++;
			if (humedity == 100.0) {
				humedity = 50.0;
			}

			goto gatt_indicate;
		}

		r = sensor_sample_fetch(hum_dev);
		if (r) {
			printk("sensor_sample_fetch failed return: %d\n", r);
		}

		r = sensor_channel_get(hum_dev, SENSOR_CHAN_HUMIDITY,
				       &hum_value);
		if (r) {
			printk("sensor_channel_get failed return: %d\n", r);
		}

		humedity = sensor_value_to_double(&hum_value);

gatt_indicate:
		printf("Humidity is %g%%\n", humedity);

		mantissa = (uint32_t)(humedity * 100);
		exponent = (uint8_t)0;

		htm[0] = 1; // temperature in celsius 
		sys_put_le24(mantissa, (uint8_t *)&htm[1]);
		htm[4] = exponent;

		ind_params_hum.attr = &hum_svc.attrs[2];
		ind_params_hum.func = indicate_cb;
		ind_params_hum.destroy = indicate_destroy;
		ind_params_hum.data = &htm;
		ind_params_hum.len = sizeof(htm);

		if (bt_gatt_indicate(NULL, &ind_params_hum) == 0) {
			indicating_hum = 1U;
		}
	}
}*/