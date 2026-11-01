/*
 * SPDX-License-Identifier: MIT
 *
 * 指针式模拟表盘（240x240 圆屏）
 */

#ifndef WATCH_FACE_H
#define WATCH_FACE_H

#include "lvgl.h"

/**
 * 在当前活动屏幕上创建表盘，并启动刷新定时器。
 * 必须在 LVGL 初始化之后、持有 LVGL 互斥锁的情况下调用。
 */
void watch_face_create(lv_disp_t *disp);

#endif /* WATCH_FACE_H */
