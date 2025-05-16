/*
 * Copyright (c) 2025
 * Regis Rousseau
 * Univ Lyon, INSA Lyon, Inria, CITI, EA3720
 * SPDX-License-Identifier: Apache-2.0
 */

//  ========== includes ====================================================================
#include "app_lorawan.h"

//  ========== globals =====================================================================
static const struct gpio_dt_spec led_tx = GPIO_DT_SPEC_GET(LED_TX, gpios);
static const struct gpio_dt_spec led_rx = GPIO_DT_SPEC_GET(LED_RX, gpios);

// downlink callback
static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *data)
{
	printk("downlink data received: ");
    for(int8_t i=0; i < len; i++) {
		printk("%02X ", data[i]);
	}
    printk("\n");
	printk("port: %d, pending: %d, RSSI: %ddB, SNR: %dBm\n", port, data_pending, rssi, snr);
}

// ADR change callback
static void lorwan_datarate_changed(enum lorawan_datarate dr)
{
	uint8_t unused, max_size;

	lorawan_get_payload_sizes(&unused, &max_size);
	printk("new datarate: DR_%d, max payload: %d\n", dr, max_size);
}

//  ========== app_loarwan_init ============================================================
int8_t app_lorawan_init(void)
{
	struct lorawan_join_config join_cfg;

	uint8_t dev_addr[] = LORAWAN_DEV_ADDR;
	uint8_t app_skey[] = LORAWAN_APP_SKEY;
    uint8_t nwk_skey[] = LORAWAN_NWK_SKEY;
	uint8_t app_eui[]  = LORAWAN_DEV_EUI;

	printk("starting LoRaWAN node initialization\n");

    // retrieve the LoRa SX1276 device
	const struct device *lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
	if (!device_is_ready(lora_dev)) {
		printk("%s: LoRaWAN device not ready\n", lora_dev->name);
		return 0;
	}

	printk("starting LoRaWAN stack\n");

    // set the region (Europe)
	int8_t ret = lorawan_set_region(LORAWAN_REGION_EU868);
	if (ret < 0) {
		printk("failed to start LoRaWAN stack. error: %d\n", ret);
		return 0;
	}

	// indicate device activity by toggling the transmission LED
	gpio_pin_set_dt(&led_tx, 1);

	// start the LoRaWAN stack
	ret = lorawan_start();
	if (ret < 0) {
		printk("failed to start LoRaWAN stack. error: %d\n", ret);
		return 0;
	} else {
			// allow some time for the stack to stabilize
			k_sleep(K_MSEC(500));
	}

	// enable Adaptive Data Rate (ADR) to optimize communication settings
    lorawan_enable_adr(true);

    // register downlink and data rate change callbacks for receiving messages and updates
	struct lorawan_downlink_cb downlink_cb = {
		.port = LW_RECV_PORT_ANY,
		.cb = dl_callback
	};
	lorawan_register_downlink_callback(&downlink_cb);
	lorawan_register_dr_changed_callback(lorwan_datarate_changed); 

	// configuration of lorawan network using ABP
    join_cfg.mode = LORAWAN_ACT_ABP;
    join_cfg.abp.dev_addr = dev_addr;
    join_cfg.abp.app_skey = app_skey;
    join_cfg.abp.nwk_skey = nwk_skey;
	join_cfg.abp.app_eui = app_eui;

	int8_t retry_count = 0;
	// attempt to join the LoRaWAN network using ABP
	do {
		printk("attempting to join LoRaWAN network using ABP\n");

		// indicate receiving activity by toggling the reception LED
		gpio_pin_set_dt(&led_rx, 1);

		ret = lorawan_join(&join_cfg);
		if (ret < 0) {
			if (ret == -ETIMEDOUT) {
				printk("join request timed out. retrying...\n");
			} else {
				printk("failed to join network. error: %d\n", ret);
			}
		}
		
		gpio_pin_set_dt(&led_rx, 0);

		if (++retry_count < MAX_JOIN_ATTEMPTS) {
			printk("maximum retries reached. aborting.\n");
            return -1;
		}
		k_sleep(K_MSEC(10000));  // Wait before retrying

	} while (ret < 0);

	printk("successfully joined LoRaWAN network using ABP.\n");
	// turn off LEDs to indicate the end of the process
	gpio_pin_set_dt(&led_tx, 0);
	gpio_pin_set_dt(&led_rx, 0);
	
    return 1;
}

  
    