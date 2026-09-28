#include <string.h>
#include <assert.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "sdkconfig.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "ble_led.h"

static const char *TAG = "ble_dev";

// SG90 Servo PWM Configuration (50Hz, 20ms period, GPIO 2)
#define LEDC_PWM_TIMER          LEDC_TIMER_0
#define LEDC_PWM_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_PWM_CHANNEL        LEDC_CHANNEL_0
#define LEDC_PWM_DUTY_RES       LEDC_TIMER_14_BIT  // Resolution: 0 ~ 16383
#define LEDC_PWM_MAX_DUTY       16383
#define LEDC_PWM_FREQ_HZ        50                 // SG90 requires 50 Hz (20ms period)

static const ble_uuid128_t s_svc_uuid = BLE_UUID128_INIT(BLE_LED_SVC_UUID128);
static const ble_uuid128_t s_chr_uuid = BLE_UUID128_INIT(BLE_LED_CHR_UUID128);

static uint16_t s_chr_val_handle;
static uint8_t s_servo_angle = 0;                  // 0 ~ 180 degrees (Default 0)
static uint8_t s_led_state = 0;                    // 0: OFF, 1: ON (Default 0)
static uint8_t s_own_addr_type;

static int ble_gap_event(struct ble_gap_event *event, void *arg);

static void servo_apply_pwm(uint8_t angle)
{
    if (angle > 180) angle = 180;
    // 0 deg -> 500us (0.5ms), 180 deg -> 2500us (2.5ms) in 20ms (20000us) period
    uint32_t pulse_us = 500 + ((uint32_t)angle * 2000) / 180;
    uint32_t duty = (pulse_us * LEDC_PWM_MAX_DUTY) / 20000;
    if (duty > LEDC_PWM_MAX_DUTY) duty = LEDC_PWM_MAX_DUTY;

    ESP_ERROR_CHECK(ledc_set_duty(LEDC_PWM_MODE, LEDC_PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_PWM_MODE, LEDC_PWM_CHANNEL));
}

void ble_device_set(uint8_t angle, uint8_t led_state)
{
    if (angle > 180) angle = 180;
    s_servo_angle = angle;
    s_led_state = led_state ? 1 : 0;

    servo_apply_pwm(s_servo_angle);
    gpio_set_level(CONFIG_LED_GPIO, s_led_state);

    ESP_LOGI(TAG, "Device Updated -> Servo Angle: %d deg (GPIO %d), LED: %s (GPIO %d)",
             s_servo_angle, CONFIG_SERVO_GPIO,
             s_led_state ? "ON" : "OFF", CONFIG_LED_GPIO);

    if (s_chr_val_handle != 0) {
        ble_gatts_chr_updated(s_chr_val_handle);
    }
}

void ble_device_toggle(void)
{
    if (s_servo_angle == 0 && s_led_state == 0) {
        // Toggle to 180 deg and LED ON
        ble_device_set(180, 1);
    } else {
        // Toggle to 0 deg and LED OFF
        ble_device_set(0, 0);
    }
}

void ble_servo_adjust_angle(int8_t delta)
{
    int16_t new_angle = (int16_t)s_servo_angle + delta;
    if (new_angle > 180) new_angle = 180;
    if (new_angle < 0) new_angle = 0;

    // Follow LED state corresponding to angle: 0 -> 0, 180 -> 1, >= 90 -> 1
    uint8_t new_led = (new_angle >= 90) ? 1 : 0;
    ble_device_set((uint8_t)new_angle, new_led);
}

void ble_servo_set_angle(uint8_t angle)
{
    uint8_t new_led = (angle >= 90) ? 1 : 0;
    ble_device_set(angle, new_led);
}

uint8_t ble_servo_get_angle(void)
{
    return s_servo_angle;
}

void ble_led_set_state(uint8_t state)
{
    ble_device_set(s_servo_angle, state ? 1 : 0);
}

uint8_t ble_led_get_state(void)
{
    return s_led_state;
}

static int ble_device_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        uint8_t payload[2] = { s_servo_angle, s_led_state };
        int rc = os_mbuf_append(ctxt->om, payload, sizeof(payload));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
        uint8_t data[2] = {0};
        uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
        if (len == 0) {
            return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
        }

        int rc = os_mbuf_copydata(ctxt->om, 0, len > 2 ? 2 : len, data);
        if (rc != 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }

        if (len == 1) {
            uint8_t val = data[0];
            if (val == 0 || val == '0') {
                // One-click State 0: 0 degrees, LED OFF
                ble_device_set(0, 0);
            } else if (val == 1 || val == '1') {
                // One-click State 1: 180 degrees, LED ON
                ble_device_set(180, 1);
            } else {
                // Direct angle value (e.g. 2 ~ 180)
                uint8_t angle = val > 180 ? 180 : val;
                uint8_t led = (angle >= 90) ? 1 : 0;
                ble_device_set(angle, led);
            }
        } else {
            // 2 bytes format: [angle, led_state]
            uint8_t angle = data[0] > 180 ? 180 : data[0];
            uint8_t led = data[1] ? 1 : 0;
            ble_device_set(angle, led);
        }
        return 0;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static const struct ble_gatt_svc_def s_gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &s_chr_uuid.u,
                .access_cb = ble_device_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &s_chr_val_handle,
            },
            {
                0, // End of characteristics
            }
        },
    },
    {
        0, // End of services
    },
};

static void ble_device_advertise(void)
{
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields rsp_fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting adv fields: rc=%d", rc);
        return;
    }

    memset(&rsp_fields, 0, sizeof(rsp_fields));
    rsp_fields.uuids128 = (ble_uuid128_t *)&s_svc_uuid;
    rsp_fields.num_uuids128 = 1;
    rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting scan response fields: rc=%d", rc);
        return;
    }

    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error starting advertising: rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "BLE advertising started. Device Name: %s", name);
}

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "BLE Connection %s; status=%d",
                 event->connect.status == 0 ? "established" : "failed",
                 event->connect.status);
        if (event->connect.status != 0) {
            ble_device_advertise();
        }
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "BLE Client disconnected; reason=%d. Resuming advertising...",
                 event->disconnect.reason);
        ble_device_advertise();
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete. Resuming advertising...");
        ble_device_advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ESP_LOGI(TAG, "BLE Subscribe event: attr_handle=%d, cur_notify=%d",
                 event->subscribe.attr_handle, event->subscribe.cur_notify);
        return 0;

    case BLE_GAP_EVENT_MTU:
        ESP_LOGI(TAG, "MTU update event: mtu=%d", event->mtu.value);
        return 0;

    default:
        return 0;
    }
}

static void ble_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    assert(rc == 0);

    ble_device_advertise();
}

static void ble_on_reset(int reason)
{
    ESP_LOGE(TAG, "NimBLE host reset; reason=%d", reason);
}

static void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

esp_err_t ble_led_init(void)
{
    // 1. Configure SG90 Servo PWM on GPIO 2 (LEDC)
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_PWM_MODE,
        .duty_resolution  = LEDC_PWM_DUTY_RES,
        .timer_num        = LEDC_PWM_TIMER,
        .freq_hz          = LEDC_PWM_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_PWM_MODE,
        .channel        = LEDC_PWM_CHANNEL,
        .timer_sel      = LEDC_PWM_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = CONFIG_SERVO_GPIO,
        .duty           = 0,
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

    // 2. Configure LED GPIO on GPIO 4
    gpio_reset_pin(CONFIG_LED_GPIO);
    gpio_set_direction(CONFIG_LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(CONFIG_LED_GPIO, s_led_state);

    // Initial Servo position
    servo_apply_pwm(s_servo_angle);

    // 3. Initialize NimBLE Port
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NimBLE port: %d", ret);
        return ret;
    }

    // 4. Set Device Name
    ble_svc_gap_device_name_set(BLE_LED_DEVICE_NAME);

    // 5. Initialize GAP & GATT services
    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to count GATT svcs: %d", rc);
        return ESP_FAIL;
    }

    rc = ble_gatts_add_svcs(s_gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to add GATT svcs: %d", rc);
        return ESP_FAIL;
    }

    // 6. Host callbacks
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.reset_cb = ble_on_reset;

    // 7. Start NimBLE Host FreeRTOS task
    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "Hardware initialized: Servo GPIO %d, LED GPIO %d", CONFIG_SERVO_GPIO, CONFIG_LED_GPIO);
    return ESP_OK;
}
