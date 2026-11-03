/*
 * SPDX-License-Identifier: MIT
 */

#include "bsp_lcd.h"
#include "board.h"

#include <string.h>
#include <assert.h>

#include "esp_heap_caps.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
#include "esp_lcd_ili9341.h"
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
#include "esp_lcd_gc9a01.h"
#endif

static const char *TAG = "bsp_lcd";

/*
 * 左边缘竖纹的成因：GC9A01 驱动 mirror() 只改 MADCTL 的 MX 位、不改 CASET 列窗口，
 * 物理左边缘对应显存最后一列；该列若上电后从未被写入，GRAM 随机内容就表现为固定竖带。
 * 开机时把整块 GRAM 写黑即可消除。
 * 置 0 可复现原现象做对比。
 */
#define PANEL_CLEAR_GRAM_ON_BOOT       1
#define PANEL_MIRROR_X                 1

/* LCD 命令/参数位宽 */
#define LCD_CMD_BITS                   8
#define LCD_PARAM_BITS                 8

/** 上电后把整块 GRAM 写黑，消除未触及列的随机竖带，也给开机一帧干净底色 */
static void panel_clear_gram(esp_lcd_panel_handle_t panel)
{
    const int lines = 20;
    const size_t bytes = BOARD_LCD_H_RES * lines * sizeof(uint16_t);
    uint16_t *buf = heap_caps_malloc(bytes, MALLOC_CAP_DMA);
    assert(buf);
    memset(buf, 0, bytes);

    for (int y = 0; y < BOARD_LCD_V_RES; y += lines) {
        int h = lines;
        if (y + h > BOARD_LCD_V_RES) {
            h = BOARD_LCD_V_RES - y;
        }
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel, 0, y, BOARD_LCD_H_RES, y + h, buf));
    }
    free(buf);
    ESP_LOGI(TAG, "GRAM 全屏清零完成 (%dx%d)", BOARD_LCD_H_RES, BOARD_LCD_V_RES);
}

esp_err_t bsp_lcd_init(
    bool (*notify_flush_ready)(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata, void *ctx),
    void *user_ctx,
    esp_lcd_panel_handle_t *out_panel)
{
    ESP_LOGI(TAG, "初始化 SPI 总线");
    spi_bus_config_t buscfg = {
        .sclk_io_num = BOARD_PIN_SCLK,
        .mosi_io_num = BOARD_PIN_MOSI,
        .miso_io_num = BOARD_PIN_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = BOARD_LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(BOARD_LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "安装面板 IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = BOARD_PIN_LCD_DC,
        .cs_gpio_num = BOARD_PIN_LCD_CS,
        .pclk_hz = BOARD_LCD_PIXEL_CLOCK_HZ,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_flush_ready,
        .user_ctx = user_ctx,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)BOARD_LCD_SPI_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = BOARD_PIN_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
#if CONFIG_EXAMPLE_LCD_CONTROLLER_ILI9341
    ESP_LOGI(TAG, "安装 ILI9341 面板驱动");
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));
#elif CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
    ESP_LOGI(TAG, "安装 GC9A01 面板驱动");
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
#endif

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
#if CONFIG_EXAMPLE_LCD_CONTROLLER_GC9A01
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
#endif
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, PANEL_MIRROR_X, false));

    /* 在开背光前先把画面准备好 */
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

#if PANEL_CLEAR_GRAM_ON_BOOT
    panel_clear_gram(panel_handle);
#endif

    *out_panel = panel_handle;
    ESP_LOGI(TAG, "屏幕初始化完成");
    return ESP_OK;
}
