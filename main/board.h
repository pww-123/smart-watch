/*
 * SPDX-License-Identifier: MIT
 *
 * board.h —— 板级硬件定义（单一事实来源）
 *
 * 所有 GPIO 引脚号、屏幕型号、背光电平、LCD 分辨率、按键等硬件参数
 * 集中在这里。任何模块要用硬件参数，一律 #include "board.h"，
 * 不要再在各自 .c 里写魔法数字。
 *
 * 换 PCB / 改接线时，只需改这一个文件。
 */

#ifndef BOARD_H
#define BOARD_H

#include "driver/spi_master.h"

/* ============================ 屏幕 ============================ */
/* 屏幕控制器型号（与 Kconfig 的 EXAMPLE_LCD_CONTROLLER_GC9A01 对应） */
#define BOARD_LCD_CONTROLLER_GC9A01   1
#define BOARD_LCD_CONTROLLER_ILI9341  0

#define BOARD_LCD_H_RES               240
#define BOARD_LCD_V_RES               240

/* SPI 像素时钟（Hz）。GC9A01 用 5MHz 稳定 */
#define BOARD_LCD_PIXEL_CLOCK_HZ      (5 * 1000 * 1000)

/* 背光：点亮电平（1=高电平点亮），BLK 引脚 */
#define BOARD_BK_LIGHT_GPIO           16
#define BOARD_BK_LIGHT_ON_LEVEL       1

/* ====================== LCD 引脚接线 ====================== */
#define BOARD_PIN_SCLK                12
#define BOARD_PIN_MOSI                11
#define BOARD_PIN_MISO                (-1)   /* GC9A01 无 MISO，未接 */
#define BOARD_PIN_LCD_DC              9
#define BOARD_PIN_LCD_RST             8
#define BOARD_PIN_LCD_CS              10
#define BOARD_PIN_TOUCH_CS            15     /* 触摸（当前未启用） */

/* ======================== 按键 ======================== */
/* 板载 BOOT 键，多数 S3 核心板接 GPIO0，按下为低电平 */
#define BOARD_BOOT_BUTTON_GPIO        0

/* ====================== SPI 总线 ====================== */
#define BOARD_LCD_SPI_HOST            SPI2_HOST

#endif /* BOARD_H */
