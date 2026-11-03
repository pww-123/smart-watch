/*
 * SPDX-License-Identifier: MIT
 *
 * lvgl_port.h —— LVGL 移植层
 *
 * 职责：把 LVGL 接到 BSP 层提供的 panel 上。
 * 包含：flush 回调、tick 定时器、LVGL 任务、递归互斥锁。
 * UI 页面（表盘/信息页）不在这里，它们只通过 pages 模块使用 LVGL。
 */

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "lvgl.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"

/**
 * SPI 颜色传输完成回调，供 BSP 层在创建 panel IO 时注册。
 * 内部用标志位保证：显示驱动注册完成后才通知 LVGL flush 完成，
 * 否则初始化命令触发的回调会解引用未初始化的 disp_drv 而崩溃。
 */
bool lvgl_port_notify_flush_ready(esp_lcd_panel_io_handle_t io,
                                  esp_lcd_panel_io_event_data_t *edata, void *ctx);

/** 返回 disp_drv 的地址（稳定存储），供 bsp_lcd_init 注册 SPI 回调时作为 user_ctx。 */
void *lvgl_port_disp_drv_ptr(void);

/** 初始化 LVGL 库 + 显示驱动，返回注册后的 display。内部会启动 LVGL 任务。 */
lv_disp_t *lvgl_port_init(esp_lcd_panel_handle_t panel);

/** 加锁（递归），超时 -1 表示无限等待。返回是否拿到锁。 */
bool lvgl_port_lock(int timeout_ms);

/** 解锁（递归）。 */
void lvgl_port_unlock(void);

#endif /* LVGL_PORT_H */
