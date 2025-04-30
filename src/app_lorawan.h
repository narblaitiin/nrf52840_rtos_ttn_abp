/*
 * Copyright (c) 2025
 * Regis Rousseau
 * Univ Lyon, INSA Lyon, Inria, CITI, EA3720
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef APP_LORAWAN_H
#define APP_LORAWAN_H

//  ========== includes ====================================================================
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/random/random.h>

//  ========== defines =====================================================================
#define LED_TX                  DT_ALIAS(ledtx)     // declared in device tree 
#define LED_RX                  DT_ALIAS(ledrx)     // declared in device tree
#define DELAY 			        K_MINUTES(2)   
#define PAYLOAD_SIZE            12

// customize based on network configuration
#define LORAWAN_DEV_ADDR		{ 0x26, 0x0B, 0x15, 0xAF }
#define LORAWAN_DEV_EUI			{ 0x70, 0xB3, 0xD5, 0x7E, 0xD0, 0x06, 0xD8, 0xDE }
#define LORAWAN_NWK_SKEY        { 0x7D, 0x04, 0x12, 0x67, 0x7E, 0xD4, 0xCC, 0x1B, 0xF5, 0xB3, 0xE7, 0xFC, 0xF1, 0xB9, 0xE5, 0xA2 }
#define LORAWAN_APP_SKEY		{ 0xE9, 0x4B, 0xCA, 0x98, 0x22, 0x78, 0x62, 0x1D, 0x26, 0xCA, 0xDF, 0x7D, 0x6D, 0xF4, 0x56, 0x23 }
#define LORAWAN_PORT            2       // application port

//  ========== prototypes ==================================================================
int8_t app_lorawan_init(void);

#endif /* APP_LORAWAN_H */