/*
 * SPDX-License-Identifier: MIT
 *
 * 走时模块。
 *
 * 当前实现：上电后从固定起点开始用 esp_timer 计数（断电归零）。
 * 后续接入 DS3231 时，只需替换 clock.c 里的 clock_init() 和 clock_get()，
 * 上层表盘代码不用改。
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int hour;    // 0-23
    int minute;  // 0-59
    int second;  // 0-59
    int wday;    // 0=周日, 1=周一 ... 6=周六
    int day;     // 1-31
    int month;   // 1-12
    int year;    // 完整年份，如 2026
} clock_time_t;

/**
 * 初始化时钟。设置起始时间。
 */
void clock_init(void);

/**
 * 读取当前时间。
 */
void clock_get(clock_time_t *out);

/**
 * 手动校时。接上 RTC 或联网校时后调用。
 */
void clock_set(int year, int month, int day, int hour, int minute, int second);

#endif /* CLOCK_H */
