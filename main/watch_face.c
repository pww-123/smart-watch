/*
 * SPDX-License-Identifier: MIT
 */

#include "watch_face.h"
#include "clock.h"

#include <math.h>
#include <stdio.h>

#include "esp_log.h"

static const char *TAG = "watch_face";

#define SCREEN_SIZE      240
#define CENTER           (SCREEN_SIZE / 2)

/* 从外到内的分层：刻度环 114..101，数字环中心半径 82（占 72..92），指针在数字以内 */
#define DIAL_RADIUS      114
#define TICK_LEN_MAJOR   13
#define TICK_LEN_MINOR   6
#define NUM_RADIUS       82

#define HAND_HOUR_LEN    48
#define HAND_MIN_LEN     70
#define HAND_SEC_LEN     96
#define HAND_TAIL_LEN    12

#define REFRESH_PERIOD_MS 1000

static const char *WEEKDAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

// 样式必须是静态存储的，LVGL 不会拷贝 lv_style_t
static lv_style_t s_style_tick_major;
static lv_style_t s_style_tick_minor;
static lv_style_t s_style_num_box;
static lv_style_t s_style_hand_hour;
static lv_style_t s_style_hand_min;
static lv_style_t s_style_hand_sec;

static lv_obj_t *s_hand_hour;
static lv_obj_t *s_hand_min;
static lv_obj_t *s_hand_sec;
static lv_obj_t *s_label_date;

static lv_point_t s_pts_hour[2];
static lv_point_t s_pts_min[2];
static lv_point_t s_pts_sec[2];

static int s_last_day = -1;

/**
 * 极坐标转屏幕坐标。0 度指向 12 点方向，顺时针增大。
 */
static void polar(float angle_deg, float radius, lv_point_t *out)
{
    float rad = (angle_deg - 90.0f) * (float)M_PI / 180.0f;
    out->x = (lv_coord_t)lroundf(CENTER + radius * cosf(rad));
    out->y = (lv_coord_t)lroundf(CENTER + radius * sinf(rad));
}

static void set_hand(lv_obj_t *line, lv_point_t *pts, float angle_deg, float len)
{
    polar(angle_deg, len, &pts[0]);
    polar(angle_deg + 180.0f, HAND_TAIL_LEN, &pts[1]);
    lv_line_set_points(line, pts, 2);
}

static void init_styles(void)
{
    lv_style_init(&s_style_tick_minor);
    lv_style_set_line_width(&s_style_tick_minor, 2);
    lv_style_set_line_color(&s_style_tick_minor, lv_color_hex(0x4C5560));
    lv_style_set_line_rounded(&s_style_tick_minor, true);
    lv_style_set_line_opa(&s_style_tick_minor, LV_OPA_COVER);

    lv_style_init(&s_style_tick_major);
    lv_style_set_line_width(&s_style_tick_major, 4);
    lv_style_set_line_color(&s_style_tick_major, lv_color_hex(0xE6E9EE));
    lv_style_set_line_rounded(&s_style_tick_major, true);
    lv_style_set_line_opa(&s_style_tick_major, LV_OPA_COVER);

    /* 数字用的透明容器：只负责居中定位，不画背景和边框 */
    lv_style_init(&s_style_num_box);
    lv_style_set_bg_opa(&s_style_num_box, LV_OPA_TRANSP);
    lv_style_set_border_width(&s_style_num_box, 0);
    lv_style_set_pad_all(&s_style_num_box, 0);
    lv_style_set_radius(&s_style_num_box, 0);

    lv_style_init(&s_style_hand_hour);
    lv_style_set_line_width(&s_style_hand_hour, 7);
    lv_style_set_line_color(&s_style_hand_hour, lv_color_hex(0xFFFFFF));
    lv_style_set_line_rounded(&s_style_hand_hour, true);
    lv_style_set_line_opa(&s_style_hand_hour, LV_OPA_COVER);

    lv_style_init(&s_style_hand_min);
    lv_style_set_line_width(&s_style_hand_min, 5);
    lv_style_set_line_color(&s_style_hand_min, lv_color_hex(0xFFFFFF));
    lv_style_set_line_rounded(&s_style_hand_min, true);
    lv_style_set_line_opa(&s_style_hand_min, LV_OPA_COVER);

    lv_style_init(&s_style_hand_sec);
    lv_style_set_line_width(&s_style_hand_sec, 3);
    lv_style_set_line_color(&s_style_hand_sec, lv_color_hex(0xFF5A36));
    lv_style_set_line_rounded(&s_style_hand_sec, true);
    lv_style_set_line_opa(&s_style_hand_sec, LV_OPA_COVER);
}

static void create_ticks(lv_obj_t *parent)
{
    for (int i = 0; i < 60; i++) {
        bool major = (i % 5 == 0);
        float angle = i * 6.0f;
        float len = major ? TICK_LEN_MAJOR : TICK_LEN_MINOR;

        lv_point_t pts[2];
        polar(angle, DIAL_RADIUS, &pts[0]);
        polar(angle, DIAL_RADIUS - len, &pts[1]);

        lv_obj_t *tick = lv_line_create(parent);
        lv_line_set_points(tick, pts, 2);
        lv_obj_add_style(tick, major ? &s_style_tick_major : &s_style_tick_minor, 0);
    }
}

/**
 * 12 个数字时标。用固定大小的透明容器承载 label，
 * 这样不用先算文字宽度就能把数字精确居中到极坐标点上。
 */
static void create_numbers(lv_obj_t *parent)
{
    static const int box_w = 24;
    static const int box_h = 20;

    for (int h = 1; h <= 12; h++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", h);

        lv_point_t p;
        polar(h * 30.0f, NUM_RADIUS, &p);

        lv_obj_t *box = lv_obj_create(parent);
        lv_obj_add_style(box, &s_style_num_box, 0);
        lv_obj_set_size(box, box_w, box_h);
        lv_obj_set_pos(box, p.x - box_w / 2, p.y - box_h / 2);
        lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *num = lv_label_create(box);
        lv_label_set_text(num, buf);
        lv_obj_set_style_text_font(num, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(num, lv_color_hex(0xC2CAD3), 0);
        lv_obj_center(num);
    }
}

static void create_center_dot(lv_obj_t *parent){
    lv_obj_t *dot = lv_obj_create(parent);
    lv_obj_set_size(dot, 10, 10);
    lv_obj_align(dot, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(dot, lv_color_hex(0xFF5A36), 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
}

static void update_date_label(int wday, int day)
{
    if (day == s_last_day) {
        return;
    }
    s_last_day = day;

    char buf[16];
    snprintf(buf, sizeof(buf), "%s %02d", WEEKDAYS[wday], day);
    lv_label_set_text(s_label_date, buf);
}

static void refresh_cb(lv_timer_t *timer)
{
    (void)timer;

    clock_time_t now;
    clock_get(&now);

    float angle_sec  = now.second * 6.0f;
    float angle_min  = now.minute * 6.0f + now.second * 0.1f;
    float angle_hour = (now.hour % 12) * 30.0f + now.minute * 0.5f;

    set_hand(s_hand_hour, s_pts_hour, angle_hour, HAND_HOUR_LEN);
    set_hand(s_hand_min,  s_pts_min,  angle_min,  HAND_MIN_LEN);
    set_hand(s_hand_sec,  s_pts_sec,  angle_sec,  HAND_SEC_LEN);

    update_date_label(now.wday, now.day);
}

void watch_face_create(lv_disp_t *disp)
{
    (void)disp;

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0B0E12), 0);

    init_styles();
    create_ticks(scr);
    create_numbers(scr);

    s_hand_hour = lv_line_create(scr);
    lv_obj_add_style(s_hand_hour, &s_style_hand_hour, 0);

    s_hand_min = lv_line_create(scr);
    lv_obj_add_style(s_hand_min, &s_style_hand_min, 0);

    s_label_date = lv_label_create(scr);
    lv_label_set_text(s_label_date, "");
    lv_obj_set_style_text_color(s_label_date, lv_color_hex(0x9AA3AD), 0);
    lv_obj_align(s_label_date, LV_ALIGN_CENTER, 0, 52);

    s_hand_sec = lv_line_create(scr);
    lv_obj_add_style(s_hand_sec, &s_style_hand_sec, 0);

    create_center_dot(scr);

    refresh_cb(NULL);
    lv_timer_create(refresh_cb, REFRESH_PERIOD_MS, NULL);

    ESP_LOGI(TAG, "表盘已创建，刷新周期 %d ms", REFRESH_PERIOD_MS);
}
