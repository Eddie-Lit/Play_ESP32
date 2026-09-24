/*
 * ESP-IDF BLE LED Controller
 * 
 * Controlled via Web Bluetooth API from PC / browser.
 */

#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "ble_led.h"

static const char *TAG = "main";

void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  ESP32 BLE Web Controller Starting...     ");
    ESP_LOGI(TAG, "===========================================");

    // 1. Initialize NVS (Required for Bluetooth PHY and calibration)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS partition due to initialization error...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash initialized successfully.");

    // 2. Initialize BLE LED Service & Advertising
    ESP_ERROR_CHECK(ble_led_init());
    ESP_LOGI(TAG, "BLE LED GATT Server initialized.");
    ESP_LOGI(TAG, "Device Name : %s", BLE_LED_DEVICE_NAME);
    ESP_LOGI(TAG, "Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    ESP_LOGI(TAG, "Char UUID   : beb5483e-36e1-4688-b7f5-ea07361b26a8");
    ESP_LOGI(TAG, "Waiting for Web Bluetooth connection...");

    // 3. Keep main task alive
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
