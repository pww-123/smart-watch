/*
 * SPDX-License-Identifier: MIT
 *
 * 页面管理：表盘页 / 系统信息页，短按 BOOT 键切换
 */

#ifndef PAGES_H
#define PAGES_H

#include "lvgl.h"

/** 创建全部页面并启动按键事件轮询，须在持有 LVGL 互斥锁时调用 */
void pages_init(lv_disp_t *disp);

/** 当前页面索引：0=表盘，1=信息 */
int pages_current(void);

#endif /* PAGES_H */
