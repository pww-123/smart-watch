/*
 * SPDX-License-Identifier: MIT
 *
 * bsp_lcd.h —— 板级显示（BSP 层）
 *
 * 职责：SPI 总线初始化 + GC9A01 面板驱动安装。
 * 对外只暴露一个「给我一个初始化好的 panel 句柄」。
 * LVGL 移植、UI 都不属于本层。
 */

#ifndef BSP_LCD_H
#define BSP_LCD_H

#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"

/**
 * 初始化 SPI 总线并安装 GC9A01 面板。
 *
 * @param notify_flush_ready  颜色传输完成回调（由 LVGL 移植层传入，
 *                            用于通知 LVGL flush 完成）；可为 NULL。
 * @param user_ctx            回调的上下文（即 disp_drv 指针）。
 * @param[out] out_panel      输出初始化好的面板句柄。
 *
 * 调用后屏幕已显示、GRAM 已清零、背光仍由调用方（backlight）控制。
 */
esp_err_t bsp_lcd_init(
    bool (*notify_flush_ready)(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *ctx),
    void *user_ctx,
    esp_lcd_panel_handle_t *out_panel);

#endif /* BSP_LCD_H */
