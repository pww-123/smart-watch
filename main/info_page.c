/*
 * SPDX-License-Identifier: MIT
 */

#include "info_page.h"
#include "board.h"

#include <stdio.h>

#include "esp_timer.h"
#include "esp_log.h"

static const char *TAG = "info_page";

#define BG_COLOR      0x0B0E12
#define TITLE_COLOR   0xFF5A36
#define KEY_COLOR     0x7E8894
#define VALUE_COLOR   0xE6E9EE

static lv_obj_t *s_label_uptime;
static lv_timer_t *s_timer;
static bool s_active = true;

static lv_obj_t *add_row(lv_obj_t *parent, const char *key, const char *value, int y)
{
    lv_obj_t *k = lv_label_create(parent);
    lv_label_set_text(k, key);
    lv_obj_set_style_text_color(k, lv_color_hex(KEY_COLOR), 0);
    lv_obj_set_pos(k, 34, y);

    lv_obj_t *v = lv_label_create(parent);
    lv_label_set_text(v, value);
    lv_obj_set_style_text_color(v, lv_color_hex(VALUE_COLOR), 0);
    lv_obj_set_pos(v, 34, y + 17);
    return v;
}

static void refresh_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!s_active) {
        return;
    }

    int64_t us = esp_timer_get_time();
    unsigned long sec = (unsigned long)(us / 1000000ULL);
    char buf[32];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
             sec / 3600, (sec / 60) % 60, sec % 60);
    lv_label_set_text(s_label_uptime, buf);
}

void info_page_create(lv_obj_t *parent)
{
    lv_obj_set_style_bg_color(parent, lv_color_hex(BG_COLOR), 0);
    lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "SYSTEM");
    lv_obj_set_style_text_color(title, lv_color_hex(TITLE_COLOR), 0);
    lv_obj_set_pos(title, 34, 34);

    /* 标题下的橙色分隔线 */
    static lv_point_t line_pts[] = {{34, 62}, {206, 62}};
    lv_obj_t *line = lv_line_create(parent);
    lv_line_set_points(line, line_pts, 2);
    lv_obj_set_style_line_width(line, 2, 0);
    lv_obj_set_style_line_color(line, lv_color_hex(TITLE_COLOR), 0);

    s_label_uptime = add_row(parent, "UPTIME", "00:00:00", 78);

    /* 屏幕型号与时钟从 board.h 读取，避免信息页与实际硬件脱节 */
    char display_buf[24];
    snprintf(display_buf, sizeof(display_buf), "GC9A01 %dx%d",
             BOARD_LCD_H_RES, BOARD_LCD_V_RES);
    add_row(parent, "DISPLAY", display_buf, 122);

    char clk_buf[24];
    snprintf(clk_buf, sizeof(clk_buf), "%d MHz",
             BOARD_LCD_PIXEL_CLOCK_HZ / 1000000);
    add_row(parent, "SPI CLOCK", clk_buf, 166);

    lv_obj_t *hint = lv_label_create(parent);
    lv_label_set_text(hint, "BOOT: short=page  long=off");
    lv_obj_set_style_text_color(hint, lv_color_hex(KEY_COLOR), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -14);

    s_timer = lv_timer_create(refresh_cb, 1000, NULL);
    s_active = true;

    ESP_LOGI(TAG, "信息页已创建");
}

void info_page_set_active(bool active)
{
    s_active = active;
    if (s_timer == NULL) {
        return;
    }
    if (active) {
        lv_timer_resume(s_timer);
        refresh_cb(NULL);
    } else {
        lv_timer_pause(s_timer);
    }
}
