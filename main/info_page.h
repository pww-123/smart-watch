/*
 * SPDX-License-Identifier: MIT
 *
 * 系统信息页：运行时长、时钟、显示与固件信息
 */

#ifndef INFO_PAGE_H
#define INFO_PAGE_H

#include "lvgl.h"

void info_page_create(lv_obj_t *parent);

/** 页面隐藏时暂停刷新，避免无意义的重绘（低功耗设计的第一小步） */
void info_page_set_active(bool active);

#endif /* INFO_PAGE_H */
