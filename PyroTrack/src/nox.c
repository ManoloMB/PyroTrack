/** @file
 *  @brief HTS Service sample
 */

/*
 * Copyright (c) 2020 SixOctets Systems
 * Copyright (c) 2019 Aaron Tsui <aaron.tsui@outlook.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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
static const struct device *temp_dev = DEVICE_DT_GET_ANY(nordic_nrf_temp);
#else
static const struct device *nox_dev;
#endif

static uint8_t simulate_nox;
static uint8_t indicating_nox;
static struct bt_gatt_indicate_params ind_params_nox;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	simulate_nox = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	indicating_nox = 0U;
}

/* Temperature Service Declaration */
BT_GATT_SERVICE_DEFINE(nox_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_VOCS),
	BT_GATT_CHARACTERISTIC(BT_UUID_HTS_MEASUREMENT, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	/* more optional Characteristics */
);

void nox_init(void)
{
	if (nox_dev == NULL || !device_is_ready(nox_dev)) {
		printk("no temperature device; using simulated data\n");
		nox_dev = NULL;
	} else {
		printk("temp device is %p, name is %s\n", nox_dev,
		       nox_dev->name);
	}
}

void nox_indicate(int16_t nox)
{
	/* Temperature measurements simulation */
	struct sensor_value voc_value;

	if (simulate_nox) {
		static uint8_t htm[5];
		//static double temperature = 20U;
		uint32_t mantissa;
		uint8_t exponent;
		int r;

		if (indicating_nox) {
			return;
		}

		if (!nox_dev) {
			/*temperature++;
			if (temperature == 30U) {
				temperature = 20U;
			}
*/
			goto gatt_indicate;
		}

		r = sensor_sample_fetch(nox_dev);
		if (r) {
			printk("sensor_sample_fetch failed return: %d\n", r);
		}

		r = sensor_channel_get(nox_dev, SENSOR_CHAN_DIE_TEMP,
				       &voc_value);
		if (r) {
			printk("sensor_channel_get failed return: %d\n", r);
		}

		//temperature = sensor_value_to_double(&temp_value);
		

gatt_indicate:
		printf("NOX is %fC\n", nox);
		
		mantissa = (uint32_t)(nox * 100);
		exponent = (uint8_t)-2;

		htm[0] = 0; /* temperature in celsius */
		sys_put_le24(mantissa, (uint8_t *)&htm[1]);
		htm[4] = exponent;

		ind_params_nox.attr = &nox_svc.attrs[2];
		ind_params_nox.func = indicate_cb;
		ind_params_nox.destroy = indicate_destroy;
		ind_params_nox.data = &htm;
		ind_params_nox.len = sizeof(htm);

		if (bt_gatt_indicate(NULL, &ind_params_nox) == 0) {
			indicating_nox = 1;
		}
	}
}