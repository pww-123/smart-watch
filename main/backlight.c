/*
 * SPDX-License-Identifier: MIT
 */

#include "backlight.h"

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "backlight";

static int s_gpio = -1;
static int s_on_level = 1;
static bool s_on = false;

void backlight_init(int gpio, int on_level)
{
    s_gpio = gpio;
    s_on_level = on_level;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));

    /* 上电先熄灭，待 UI 就绪后再点亮，避免闪屏 */
    gpio_set_level(s_gpio, s_on_level ? 0 : 1);
    s_on = false;

    ESP_LOGI(TAG, "背光已初始化: GPIO%d, 点亮电平=%d", gpio, on_level);
}

void backlight_on(void)
{
    if (s_gpio < 0 || s_on) {
        return;
    }
    gpio_set_level(s_gpio, s_on_level);
    s_on = true;
}

void backlight_off(void)
{
    if (s_gpio < 0 || !s_on) {
        return;
    }
    gpio_set_level(s_gpio, s_on_level ? 0 : 1);
    s_on = false;
}

bool backlight_is_on(void)
{
    return s_on;
}
