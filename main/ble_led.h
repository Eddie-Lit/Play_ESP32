#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_LED_DEVICE_NAME "ESP32-BLE-LED"

// Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
#define BLE_LED_SVC_UUID128 \
    0x4b, 0x91, 0x31, 0xc3, 0xc9, 0xc5, 0xcc, 0x8f, \
    0x9e, 0x45, 0xb5, 0x1f, 0x01, 0xc2, 0xaf, 0x4f

// Characteristic UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
#define BLE_LED_CHR_UUID128 \
    0xa8, 0x26, 0x1b, 0x36, 0x07, 0xea, 0xf5, 0xb7, \
    0x88, 0x46, 0xe1, 0x36, 0x3e, 0x48, 0xb5, 0xbe

/**
 * @brief Initialize BLE stack and start advertising for LED control.
 *
 * @return ESP_OK on success.
 */
esp_err_t ble_led_init(void);

/**
 * @brief Set the LED state and notify connected BLE clients if changed.
 *
 * @param state 0 for OFF, 1 for ON.
 */
void ble_led_set_state(uint8_t state);

/**
 * @brief Get the current LED state.
 *
 * @return 0 for OFF, 1 for ON.
 */
uint8_t ble_led_get_state(void);

#ifdef __cplusplus
}
#endif
