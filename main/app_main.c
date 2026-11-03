/*
 * SPDX-License-Identifier: MIT
 *
 * app_main.c —— 应用装配层
 *
 * 只负责「初始化顺序」：走时 → 背光 → 屏幕(BSP) → LVGL 移植 → 页面 → 按键。
 * 具体实现都在各自的模块里，这里不放硬件细节。
 */

#include <stdio.h>

#include "esp_lcd_panel_ops.h"
#include "esp_log.h"

#include "board.h"
#include "clock.h"
#include "backlight.h"
#include "bsp_lcd.h"
#include "lvgl_port.h"
#include "pages.h"
#include "button.h"

static const char *TAG = "main";

void app_main(void)
{
    /* 1. 走时（软件计时，后续可换 DS3231） */
    clock_init();

    /* 2. 背光：先熄，等 UI 就绪再点亮，避免闪屏 */
    backlight_init(BOARD_BK_LIGHT_GPIO, BOARD_BK_LIGHT_ON_LEVEL);

    /* 3. 屏幕：SPI + GC9A01 面板（回调由 LVGL 移植层提供） */
    esp_lcd_panel_handle_t panel = NULL;
    ESP_ERROR_CHECK(bsp_lcd_init(lvgl_port_notify_flush_ready, lvgl_port_disp_drv_ptr(), &panel));

    /* 4. LVGL 移植 */
    lv_disp_t *disp = lvgl_port_init(panel);

    /* 5. 页面 */
    if (lvgl_port_lock(-1)) {
        pages_init(disp);
        lvgl_port_unlock();
    }

    /* 6. 按键（esp_timer 扫描，不占额外任务） */
    button_init(BOARD_BOOT_BUTTON_GPIO);

    ESP_LOGI(TAG, "启动完成");
}
