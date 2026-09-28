#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "rotary_encoder.h"
#include "ble_led.h"

static const char *TAG = "rotary_encoder";

// Servo angle adjustment step size per click (degrees)
#define ANGLE_STEP              5

typedef enum {
    ROTARY_EVENT_ROTATE_CW,   // 顺时针旋转（增大舵机角度）
    ROTARY_EVENT_ROTATE_CCW,  // 逆时针旋转（减小舵机角度）
    ROTARY_EVENT_BUTTON_PRESS // SW 按键按下（一键在 0°/关灯 与 180°/开灯 之间切换）
} rotary_event_t;

static QueueHandle_t s_rotary_queue = NULL;

/*
 * Gray code state transition table.
 * State index: (prev_state << 2) | curr_state
 * where state = (CLK << 1) | DT
 */
static const int8_t s_rotary_table[16] = {
     0, // 0:  00 -> 00
    -1, // 1:  00 -> 01 (CCW)
     1, // 2:  00 -> 10 (CW)
     0, // 3:  00 -> 11 (invalid)
     1, // 4:  01 -> 00 (CW)
     0, // 5:  01 -> 01
     0, // 6:  01 -> 10 (invalid)
    -1, // 7:  01 -> 11 (CCW)
    -1, // 8:  10 -> 00 (CCW)
     0, // 9:  10 -> 01 (invalid)
     0, // 10: 10 -> 10
     1, // 11: 10 -> 11 (CW)
     0, // 12: 11 -> 00 (invalid)
     1, // 13: 11 -> 01 (CW)
    -1, // 14: 11 -> 10 (CCW)
     0  // 15: 11 -> 11
};

static void IRAM_ATTR rotary_isr_handler(void *arg)
{
    static uint8_t s_history = 0x03;
    static int8_t s_counter = 0;

    uint8_t clk = gpio_get_level(ROTARY_CLK_GPIO);
    uint8_t dt  = gpio_get_level(ROTARY_DT_GPIO);
    uint8_t curr = (clk << 1) | dt;

    s_history = ((s_history << 2) | curr) & 0x0F;
    s_counter += s_rotary_table[s_history];

    // Detent position is reached when both CLK and DT return to HIGH (11)
    if (curr == 0x03) {
        BaseType_t high_task_awoken = pdFALSE;
        rotary_event_t evt;
        if (s_counter >= 2) {
            evt = ROTARY_EVENT_ROTATE_CW;
            xQueueSendFromISR(s_rotary_queue, &evt, &high_task_awoken);
        } else if (s_counter <= -2) {
            evt = ROTARY_EVENT_ROTATE_CCW;
            xQueueSendFromISR(s_rotary_queue, &evt, &high_task_awoken);
        }
        s_counter = 0;
        if (high_task_awoken) {
            portYIELD_FROM_ISR();
        }
    }
}

static void IRAM_ATTR button_isr_handler(void *arg)
{
    static uint32_t s_last_press_tick = 0;
    uint32_t now = xTaskGetTickCountFromISR();
    // 200ms debounce
    if ((now - s_last_press_tick) > pdMS_TO_TICKS(200)) {
        s_last_press_tick = now;
        rotary_event_t evt = ROTARY_EVENT_BUTTON_PRESS;
        BaseType_t high_task_awoken = pdFALSE;
        xQueueSendFromISR(s_rotary_queue, &evt, &high_task_awoken);
        if (high_task_awoken) {
            portYIELD_FROM_ISR();
        }
    }
}

static void rotary_task(void *pvParameters)
{
    rotary_event_t evt;
    ESP_LOGI(TAG, "Rotary encoder worker task started");

    while (1) {
        if (xQueueReceive(s_rotary_queue, &evt, portMAX_DELAY)) {
            switch (evt) {
            case ROTARY_EVENT_ROTATE_CW:
                // 顺时针旋转：增加舵机角度
                ble_servo_adjust_angle(+ANGLE_STEP);
                break;

            case ROTARY_EVENT_ROTATE_CCW:
                // 逆时针旋转：减小舵机角度
                ble_servo_adjust_angle(-ANGLE_STEP);
                break;

            case ROTARY_EVENT_BUTTON_PRESS:
                // SW 按钮按下：一键在 (0° / 关灯) 与 (180° / 开灯) 间切换
                ESP_LOGI(TAG, "SW button pressed -> Toggling state (0 deg / OFF <-> 180 deg / ON)");
                ble_device_toggle();
                break;
            }
        }
    }
}

esp_err_t rotary_encoder_init(void)
{
    ESP_LOGI(TAG, "Initializing KY040 Rotary Encoder (CLK: %d, DT: %d, SW: %d)...",
             ROTARY_CLK_GPIO, ROTARY_DT_GPIO, ROTARY_SW_GPIO);

    // 1. Create event queue
    s_rotary_queue = xQueueCreate(20, sizeof(rotary_event_t));
    if (s_rotary_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create rotary event queue");
        return ESP_ERR_NO_MEM;
    }

    // 2. Configure CLK and DT GPIOs
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ROTARY_CLK_GPIO) | (1ULL << ROTARY_DT_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CLK/DT GPIOs: %d", ret);
        return ret;
    }

    // 3. Configure SW (Button) GPIO
    gpio_config_t sw_conf = {
        .pin_bit_mask = (1ULL << ROTARY_SW_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    ret = gpio_config(&sw_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure SW GPIO: %d", ret);
        return ret;
    }

    // 4. Install GPIO ISR service if not already installed
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %d", ret);
        return ret;
    }

    // 5. Hook ISR handlers
    gpio_isr_handler_add(ROTARY_CLK_GPIO, rotary_isr_handler, NULL);
    gpio_isr_handler_add(ROTARY_DT_GPIO, rotary_isr_handler, NULL);
    gpio_isr_handler_add(ROTARY_SW_GPIO, button_isr_handler, NULL);

    // 6. Create FreeRTOS worker task
    BaseType_t task_ret = xTaskCreate(rotary_task, "rotary_task", 3072, NULL, 10, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create rotary FreeRTOS task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "KY040 Rotary Encoder initialized successfully.");
    return ESP_OK;
}
