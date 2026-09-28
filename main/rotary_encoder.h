#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// KY040 Rotary Encoder Pin Definitions
#define ROTARY_CLK_GPIO     18
#define ROTARY_DT_GPIO      19
#define ROTARY_SW_GPIO      21

/**
 * @brief Initialize KY040 rotary encoder GPIOs, interrupts, and decoding task.
 *
 * @return ESP_OK on success.
 */
esp_err_t rotary_encoder_init(void);

#ifdef __cplusplus
}
#endif
