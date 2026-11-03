/*
 * SPDX-License-Identifier: MIT
 *
 * 背光控制。封装 BLK 引脚的开关电平，供息屏/唤醒逻辑复用。
 */

#ifndef BACKLIGHT_H
#define BACKLIGHT_H

#include <stdbool.h>

/**
 * 初始化背光 GPIO 为输出，并默认关闭。
 * @param gpio  BLK 引脚号
 * @param on_level  点亮时的电平（1=高电平点亮）
 */
void backlight_init(int gpio, int on_level);

/** 开背光 */
void backlight_on(void);

/** 关背光 */
void backlight_off(void);

/** 当前是否点亮 */
bool backlight_is_on(void);

#endif /* BACKLIGHT_H */
