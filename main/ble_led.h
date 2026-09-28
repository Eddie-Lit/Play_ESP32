#pragma once

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLE_LED_DEVICE_NAME "ESP32-BLE-LED"

// Hardware Pin Configuration
#define CONFIG_SERVO_GPIO   2   // GPIO 2: SG90 Servo PWM output (0° ~ 180°)
#define CONFIG_LED_GPIO     4   // GPIO 4: LED digital output (0: OFF, 1: ON)

// Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
#define BLE_LED_SVC_UUID128 \
    0x4b, 0x91, 0x31, 0xc3, 0xc9, 0xc5, 0xcc, 0x8f, \
    0x9e, 0x45, 0xb5, 0x1f, 0x01, 0xc2, 0xaf, 0x4f

// Characteristic UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8
#define BLE_LED_CHR_UUID128 \
    0xa8, 0x26, 0x1b, 0x36, 0x07, 0xea, 0xf5, 0xb7, \
    0x88, 0x46, 0xe1, 0x36, 0x3e, 0x48, 0xb5, 0xbe

/**
 * @brief Initialize BLE stack, Servo PWM (GPIO 2), and LED GPIO (GPIO 4).
 *
 * @return ESP_OK on success.
 */
esp_err_t ble_led_init(void);

/**
 * @brief Set servo angle and LED state simultaneously.
 *
 * @param angle Servo angle (0 to 180 degrees)
 * @param led_state LED state (0 for OFF, 1 for ON)
 */
void ble_device_set(uint8_t angle, uint8_t led_state);

/**
 * @brief Toggle between State 0 (0 deg, LED OFF) and State 1 (180 deg, LED ON).
 */
void ble_device_toggle(void);

/**
 * @brief Adjust servo angle by delta (clamped to 0~180).
 * LED will be updated accordingly (0 deg -> OFF, 180 deg -> ON, intermediate -> ON if >= 90).
 *
 * @param delta Angle step (e.g. +5 or -5)
 */
void ble_servo_adjust_angle(int8_t delta);

/**
 * @brief Set servo angle directly.
 *
 * @param angle Angle (0 to 180)
 */
void ble_servo_set_angle(uint8_t angle);

/**
 * @brief Get current servo angle.
 *
 * @return Current angle (0 to 180).
 */
uint8_t ble_servo_get_angle(void);

/**
 * @brief Set LED state directly.
 *
 * @param state 0 for OFF, 1 for ON.
 */
void ble_led_set_state(uint8_t state);

/**
 * @brief Get current LED state.
 *
 * @return 0 for OFF, 1 for ON.
 */
uint8_t ble_led_get_state(void);

#ifdef __cplusplus
}
#endif
