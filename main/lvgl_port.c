/*
 * SPDX-License-Identifier: MIT
 */

#include "lvgl_port.h"
#include "board.h"

#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_log.h"

static const char *TAG = "lvgl_port";

#define TICK_PERIOD_MS          2
#define TASK_MAX_DELAY_MS       500
#define TASK_MIN_DELAY_MS       1
/* 两页 90+ 个对象 + 线条/文字绘制，4KB 栈在刷新时容易溢出，提到 8KB */
#define TASK_STACK_SIZE         (8 * 1024)
#define TASK_PRIORITY           2

static SemaphoreHandle_t s_mux = NULL;

/*
 * 修复 StoreProhibited 崩溃的关键：
 * SPI 的 on_color_trans_done 回调在 esp_lcd_panel_init 发送初始化命令时就会被触发，
 * 而此时 lv_disp_drv 尚未初始化。用标志位保证：只有显示驱动注册完成后，
 * 才允许回调通知 LVGL flush 完成。
 */
static bool s_disp_drv_ready = false;

/* disp_drv 是稳定存储，其地址在 SPI 回调注册时（早于 lvgl_port_init）就要被引用 */
static lv_disp_drv_t s_disp_drv;

void *lvgl_port_disp_drv_ptr(void)
{
    return &s_disp_drv;
}

bool lvgl_port_notify_flush_ready(esp_lcd_panel_io_handle_t panel_io,
                                  esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    if (s_disp_drv_ready) {
        lv_disp_flush_ready(disp_driver);
    }
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    esp_lcd_panel_draw_bitmap(panel_handle, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
}

static void lvgl_port_update_callback(lv_disp_drv_t *drv)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;

    switch (drv->rotated) {
    case LV_DISP_ROT_NONE:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, 1, false);
        break;
    case LV_DISP_ROT_90:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, true, true);
        break;
    case LV_DISP_ROT_180:
        esp_lcd_panel_swap_xy(panel_handle, false);
        esp_lcd_panel_mirror(panel_handle, false, true);
        break;
    case LV_DISP_ROT_270:
        esp_lcd_panel_swap_xy(panel_handle, true);
        esp_lcd_panel_mirror(panel_handle, false, false);
        break;
    }
}

static void increase_lvgl_tick(void *arg)
{
    (void)arg;
    lv_tick_inc(TICK_PERIOD_MS);
}

static void lvgl_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "启动 LVGL 任务");
    uint32_t task_delay_ms = TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_port_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_port_unlock();
        }
        if (task_delay_ms > TASK_MAX_DELAY_MS) {
            task_delay_ms = TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < TASK_MIN_DELAY_MS) {
            task_delay_ms = TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

lv_disp_t *lvgl_port_init(esp_lcd_panel_handle_t panel)
{
    static lv_disp_draw_buf_t disp_buf;

    ESP_LOGI(TAG, "初始化 LVGL 库");
    lv_init();

    lv_color_t *buf1 = heap_caps_malloc(BOARD_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    lv_color_t *buf2 = heap_caps_malloc(BOARD_LCD_H_RES * 20 * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buf1 && buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, BOARD_LCD_H_RES * 20);

    ESP_LOGI(TAG, "注册显示驱动");
    lv_disp_drv_init(&s_disp_drv);
    s_disp_drv.hor_res = BOARD_LCD_H_RES;
    s_disp_drv.ver_res = BOARD_LCD_V_RES;
    s_disp_drv.flush_cb = lvgl_flush_cb;
    s_disp_drv.drv_update_cb = lvgl_port_update_callback;
    s_disp_drv.draw_buf = &disp_buf;
    s_disp_drv.user_data = panel;
    lv_disp_t *disp = lv_disp_drv_register(&s_disp_drv);

    /* 显示驱动已注册，此后 SPI 传输完成回调才允许通知 LVGL flush 完成 */
    s_disp_drv_ready = true;

    ESP_LOGI(TAG, "安装 LVGL tick 定时器");
    const esp_timer_create_args_t tick_args = {
        .callback = &increase_lvgl_tick,
        .name = "lvgl_tick",
    };
    esp_timer_handle_t tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, TICK_PERIOD_MS * 1000));

    s_mux = xSemaphoreCreateRecursiveMutex();
    assert(s_mux);

    ESP_LOGI(TAG, "创建 LVGL 任务");
    xTaskCreate(lvgl_task, "LVGL", TASK_STACK_SIZE, NULL, TASK_PRIORITY, NULL);

    return disp;
}

bool lvgl_port_lock(int timeout_ms)
{
    const TickType_t timeout_ticks = (timeout_ms == -1) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTakeRecursive(s_mux, timeout_ticks) == pdTRUE;
}

void lvgl_port_unlock(void)
{
    xSemaphoreGiveRecursive(s_mux);
}
