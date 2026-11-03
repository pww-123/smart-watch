/*
 * SPDX-License-Identifier: MIT
 */

#include "button.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "button";

#define SCAN_PERIOD_US        (20 * 1000)   /* 20ms 扫描 */
#define DEBOUNCE_TICKS        2            /* 40ms 消抖 */
#define LONG_PRESS_TICKS      50           /* 1000ms 判为长按 */

static int s_gpio;
static esp_timer_handle_t s_timer;
static volatile int s_pending = BUTTON_EVT_NONE;

/* 状态机变量，仅在 esp_timer 回调中访问，无需加锁 */
static int s_press_ticks = 0;   /* 连续检测到按下的扫描次数 */
static bool s_released = true;  /* 上次扫描时按键处于松开状态 */
static bool s_long_fired = false;

static void button_scan_cb(void *arg)
{
    (void)arg;
    /* BOOT 键通常按下接地，空闲为高电平 */
    bool pressed = (gpio_get_level(s_gpio) == 0);

    if (pressed) {
        s_press_ticks++;
        s_released = false;

        if (!s_long_fired && s_press_ticks >= LONG_PRESS_TICKS) {
            s_pending = BUTTON_EVT_LONG;
            s_long_fired = true;
        }
    } else {
        /* 松开瞬间结算：超过消抖阈值且未触发过长按 => 短按 */
        if (!s_released && !s_long_fired && s_press_ticks >= DEBOUNCE_TICKS) {
            s_pending = BUTTON_EVT_SHORT;
        }
        s_press_ticks = 0;
        s_released = true;
        s_long_fired = false;
    }
}

button_evt_t button_poll_event(void)
{
    /* 单生产者、单消费者，32 位对齐读写在 Xtensa 上是原子的 */
    button_evt_t evt = (button_evt_t)s_pending;
    if (evt != BUTTON_EVT_NONE) {
        s_pending = BUTTON_EVT_NONE;
    }
    return evt;
}

void button_init(int gpio)
{
    s_gpio = gpio;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));

    const esp_timer_create_args_t args = {
        .callback = &button_scan_cb,
        .name = "btn_scan",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &s_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(s_timer, SCAN_PERIOD_US));

    ESP_LOGI(TAG, "按键已启用: GPIO%d, 扫描 %d ms, 长按阈值 %d ms",
             gpio, SCAN_PERIOD_US / 1000, LONG_PRESS_TICKS * SCAN_PERIOD_US / 1000);
}
