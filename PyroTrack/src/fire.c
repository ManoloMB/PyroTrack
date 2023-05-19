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
static const struct device *fire_dev;
#endif

static uint8_t simulate_fire;
static uint8_t indicating_fire;
static struct bt_gatt_indicate_params ind_params_fire;

static void htmc_ccc_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	simulate_fire = (value == BT_GATT_CCC_INDICATE) ? 1 : 0;
}

static void indicate_cb(struct bt_conn *conn,
			struct bt_gatt_indicate_params *params, uint8_t err)
{
	printk("Indication %s\n", err != 0U ? "fail" : "success");
}

static void indicate_destroy(struct bt_gatt_indicate_params *params)
{
	printk("Indication complete\n");
	indicating_fire = 0U;
}

/* Temperature Service Declaration */
BT_GATT_SERVICE_DEFINE(fire_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_HTS),
	BT_GATT_CHARACTERISTIC(BT_UUID_DIS_SOFTWARE_REVISION, BT_GATT_CHRC_INDICATE,
			       BT_GATT_PERM_NONE, NULL, NULL, NULL),
	BT_GATT_CCC(htmc_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),
	/* more optional Characteristics */
);

void fire_init(void)
{
	if (fire_dev == NULL || !device_is_ready(fire_dev)) {
		printk("no temperature device; using simulated data\n");
		fire_dev = NULL;
	} else {
		printk("temp device is %p, name is %s\n", fire_dev,
		       fire_dev->name);
	}
}

void fire_indicate(int16_t fire)
{
	/* Temperature measurements simulation */
	struct sensor_value fire_value;

	if (simulate_fire) {
		static uint8_t htm[10];
		//static double temperature = 20U;
		uint32_t mantissa;
		uint8_t exponent;
		int r;

		if (indicating_fire) {
			return;
		}

		if (!fire_dev) {
			/*temperature++;
			if (temperature == 30U) {
				temperature = 20U;
			}
*/
			goto gatt_indicate;
		}

		r = sensor_sample_fetch(fire_dev);
		if (r) {
			printk("sensor_sample_fetch failed return: %d\n", r);
		}

		r = sensor_channel_get(fire_dev, SENSOR_CHAN_DIE_TEMP,
				       &fire_value);
		if (r) {
			printk("sensor_channel_get failed return: %d\n", r);
		}

		//temperature = sensor_value_to_double(&temp_value);
		

gatt_indicate:
		printf("fire is %d\n", fire);
		
		char buffer[32];
		const char *str;
		if (fire == 1){
			sprintf(buffer, "FIRE!");
		}else{
			sprintf(buffer, "OK");
		}
		
		str = buffer;
		uint8_t message_length = strlen(str);
		//mantissa = (uint32_t)(voc * 100);
		//exponent = (uint8_t)-2;

		htm[0] = message_length; /* temperature in celsius */
		memcpy(&htm[1], str, message_length);

		ind_params_fire.attr = &fire_svc.attrs[2];
		ind_params_fire.func = indicate_cb;
		ind_params_fire.destroy = indicate_destroy;
		ind_params_fire.data = &htm;
		ind_params_fire.len = sizeof(htm);

		if (bt_gatt_indicate(NULL, &ind_params_fire) == 0) {
			indicating_fire = 1;
		}
	}
}