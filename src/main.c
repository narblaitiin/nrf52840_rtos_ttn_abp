/*
 * Copyright (c) 2025
 * Regis Rousseau
 * Univ Lyon, INSA Lyon, Inria, CITI, EA3720
 * SPDX-License-Identifier: Apache-2.0
 */

//  ========== includes ====================================================================
#include "app_lorawan.h"
#include "app_rtc.h"

//  ========== globals =====================================================================
// define GPIO specifications for the LEDs used to indicate transmission (TX) and reception (RX)
static const struct gpio_dt_spec led_tx = GPIO_DT_SPEC_GET(LED_TX, gpios);
static const struct gpio_dt_spec led_rx = GPIO_DT_SPEC_GET(LED_RX, gpios);

//  ========== main ========================================================================
int8_t main(void)
{
	uint8_t payload[PAYLOAD_SIZE] = {0};

	// structure to hold simulated sensor data
	struct data_t {
		int32_t timestamp;
		int16_t vbat;  // battery voltage in millivolts
		int16_t temp;  // temperature in tenths of a degree Celsius
		int16_t hum;   // humidity as a percentage
		int16_t vadc;  // ADC voltage in millivolts
	} data;

	// configure LEDs for TX and RX indication
	gpio_pin_configure_dt(&led_tx, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&led_rx, GPIO_OUTPUT_ACTIVE);
	gpio_pin_set_dt(&led_tx, 0);		// turn off TX LED
	gpio_pin_set_dt(&led_rx, 0);		// turn off TX LED

	// initialize LoRaWAN protocol and register the device
	int8_t ret = app_lorawan_init();
	if (ret != 1) {
		printk("failed to initialze LoRaWAN protocol\n");
		return 0;
	}

	// initialize DS3231 RTC device via I2C (Pins: SDA -> P0.09, SCL -> P0.0)
    const struct device *rtc_dev = app_rtc_init();
    if (!rtc_dev) {
        printk("failed to initialize RTC device\n");
        return 0;
    }

	// retrieve the current timestamp from the RTC device 
	struct tm new_time = {
        .tm_sec = 0,
        .tm_min = 0,
        .tm_hour = 12,
        .tm_mday = 30,
        .tm_mon = 3,   // April (0-based)
        .tm_year = 125, // 2025 (since 1900)
        .tm_wday = 3    // Wednesday
    };

	printk("Geophone Measurement Simulation and Process Information\n");
	
	// start the main loop for data simulation and transmission
	while (1) {

		// retrieve the current timestamp from the RTC device 
		if (app_rtc_set_time(rtc_dev, &new_time) == 0) {
			struct tm current_time;
			data.timestamp = app_rtc_get_time(rtc_dev, &current_time);
		} else {
			printk("RTC set time failed\n");
			k_sleep(K_SECONDS(10));
			continue;
		}

		// generate random simulated sensor data
		data.vbat = sys_rand16_get() % 101;            // battery voltage: 0-100
		data.temp = sys_rand16_get() % 201 - 100;      // temperature: -100 to 100
		data.hum = sys_rand16_get() % 101;             // humidity: 0-100
		data.vadc = sys_rand16_get() % 3301;           // ADC voltage: 0-3300 mV

		// add the timestamp to the start of the data buffer
		payload[0] = (data.timestamp >> 24) & 0xFF; // most significant byte
		payload[1] = (data.timestamp >> 16) & 0xFF;
		payload[2] = (data.timestamp >> 8) & 0xFF;
		payload[3] = data.timestamp & 0xFF;        // least significant byte
		
		// encode sensor data into the payload
        payload[4] = (data.vbat >> 8) & 0xFF;
        payload[5] = data.vbat & 0xFF;

        payload[6] = (data.temp >> 8) & 0xFF;
        payload[7] = data.temp & 0xFF;

        payload[8] = (data.hum >> 8) & 0xFF;
        payload[9] = data.hum & 0xFF;

        payload[10] = (data.vadc >> 8) & 0xFF;
        payload[11] = data.vadc & 0xFF;

		// indicate data transmission with the TX LED
		printk("sending random data...\n");
		gpio_pin_set_dt(&led_tx, 1);

		// send the payload over LoRaWAN (unconfirmed message)
		ret = lorawan_send(LORAWAN_PORT, payload, PAYLOAD_SIZE, LORAWAN_MSG_UNCONFIRMED);
		
		// handle transmission errors
		if (ret == -EAGAIN) {
			printk("LoRaWAN send failed (retry). error: %d\n", ret);
			gpio_pin_set_dt(&led_tx, 0);
			k_sleep(K_SECONDS(10));
			continue;
		}
		
		if (ret < 0) {
			printk("LoRaWAN send failed. error: %d. retrying\n", ret);
			gpio_pin_set_dt(&led_tx, 0);
        	k_sleep(K_SECONDS(10));
        	continue;
		}

		printk("data sent successfully!\n");
		gpio_pin_set_dt(&led_tx, 0);

		// wait before the next iteration
		k_sleep(DELAY);
	}
	return 0;
}