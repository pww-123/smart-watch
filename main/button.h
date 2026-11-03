/*
 * SPDX-License-Identifier: MIT
 *
 * 板载 BOOT 键输入：消抖 + 短按/长按识别
 *
 * 设计约束：esp_timer 回调运行在独立的 esp_timer 任务上下文，
 * 不是 LVGL 任务，因此这里绝不调用任何 lv_* API。
 * 事件通过一个简单的待取标志交给 LVGL 定时器消费（见 pages.c），
 * 从而无需队列、信号量或额外的 FreeRTOS 任务。
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

typedef enum {
    BUTTON_EVT_NONE = 0,
    BUTTON_EVT_SHORT,   /* 短按：切换页面 */
    BUTTON_EVT_LONG,    /* 长按：预留给息屏 */
} button_evt_t;

/**
 * @param gpio  按键 GPIO（BOOT 键通常为 GPIO0，按下接地）
 */
void button_init(int gpio);

/**
 * 取出一个待处理事件并清除。可在任意上下文调用；
 * 建议由 LVGL 定时器在 LVGL 任务内轮询，这样切页面时天然持有正确上下文。
 */
button_evt_t button_poll_event(void);

#endif /* BUTTON_H */
