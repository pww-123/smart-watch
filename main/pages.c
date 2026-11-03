/*
 * SPDX-License-Identifier: MIT
 */

#include "pages.h"
#include "button.h"
#include "watch_face.h"
#include "info_page.h"
#include "backlight.h"

#include "esp_log.h"

static const char *TAG = "pages";

#define PAGE_COUNT       2
#define POLL_PERIOD_MS   50

static lv_obj_t *s_pages[PAGE_COUNT];
static int s_current = 0;
static bool s_sleeping = false;

static lv_obj_t *make_page(lv_obj_t *scr)
{
    lv_obj_t *p = lv_obj_create(scr);
    lv_obj_set_size(p, 240, 240);
    lv_obj_set_pos(p, 0, 0);
    lv_obj_set_style_border_width(p, 0, 0);
    lv_obj_set_style_outline_width(p, 0, 0);
    lv_obj_set_style_radius(p, 0, 0);
    lv_obj_set_style_pad_all(p, 0, 0);
    lv_obj_clear_flag(p, LV_OBJ_FLAG_SCROLLABLE);
    return p;
}

static void show_page(int idx)
{
    if (idx < 0 || idx >= PAGE_COUNT || idx == s_current) {
        return;
    }

    lv_obj_add_flag(s_pages[s_current], LV_OBJ_FLAG_HIDDEN);
    s_current = idx;
    lv_obj_clear_flag(s_pages[s_current], LV_OBJ_FLAG_HIDDEN);

    /* 隐藏的页面暂停自己的刷新定时器 */
    watch_face_set_active(s_current == 0);
    info_page_set_active(s_current == 1);

    ESP_LOGI(TAG, "切换到页面 %d", s_current);
}

/* 息屏：关背光 + 暂停页面刷新，走时照常（esp_timer 不暂停） */
static void enter_sleep(void)
{
    if (s_sleeping) {
        return;
    }
    s_sleeping = true;

    /* 先暂停刷新，再关背光，避免关灯瞬间残留一帧重绘 */
    watch_face_set_active(false);
    info_page_set_active(false);
    backlight_off();

    ESP_LOGI(TAG, "息屏（走时保持）");
}

/* 唤醒：开背光 + 恢复当前页刷新 */
static void exit_sleep(void)
{
    if (!s_sleeping) {
        return;
    }
    s_sleeping = false;

    backlight_on();
    watch_face_set_active(s_current == 0);
    info_page_set_active(s_current == 1);

    ESP_LOGI(TAG, "唤醒");
}

static void poll_cb(lv_timer_t *timer)
{
    (void)timer;

    button_evt_t evt = button_poll_event();
    switch (evt) {
    case BUTTON_EVT_SHORT:
        if (s_sleeping) {
            exit_sleep();
        } else {
            show_page((s_current + 1) % PAGE_COUNT);
        }
        break;
    case BUTTON_EVT_LONG:
        enter_sleep();
        break;
    default:
        break;
    }
}

int pages_current(void)
{
    return s_current;
}

void pages_init(lv_disp_t *disp)
{
    (void)disp;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0E12), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_pages[0] = make_page(scr);
    s_pages[1] = make_page(scr);

    watch_face_create(s_pages[0]);
    info_page_create(s_pages[1]);

    lv_obj_add_flag(s_pages[1], LV_OBJ_FLAG_HIDDEN);
    info_page_set_active(false);

    /* 在 LVGL 任务上下文中消费按键事件，无需跨任务加锁 */
    lv_timer_create(poll_cb, POLL_PERIOD_MS, NULL);

    ESP_LOGI(TAG, "页面管理已启动，共 %d 页", PAGE_COUNT);
}
