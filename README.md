# SmartWatch — 基于 ESP32-S3 的自制智能手表

从零开始做一块能戴在手上的智能手表：ESP32-S3-N16R8 + 1.28 寸圆形 GC9A01 TFT，
用 ESP-IDF v5.3 + LVGL 8.3 从驱动层一路写到 UI 层。

当前进度：**指针式模拟表盘已完成并跑通**（时/分/秒指针 + 1~12 数字刻度 + 星期日期）。

<!-- TODO: 把演示 gif 放到 docs/demo.gif，然后取消下面这行的注释 -->
<!-- ![demo](docs/demo.gif) -->

---

## 硬件 BOM

| 器件 | 型号 / 规格 | 数量 | 单价 | 备注 |
|---|---|---|---|---|
| 主控 | ESP32-S3-N16R8（16MB Flash + 8MB OPI PSRAM） | 1 | | 8MB PSRAM 目前尚未启用 |
| 屏幕 | 1.28" 圆形 TFT，GC9A01，240×240，SPI | 1 | | 13 针，3.3V 逻辑 |
| 充电模块 | TP4056（18650 1A） | 1 | | 只是充电器，**不是电池** |
| 电池 | 3.7V 锂电 500–1000mAh（带保护板） | 1 | | 尚未购买 |
| 升压模块 | MT3608 等 3.7V→5V | 1 | | 电池方案的必需品，见下方"踩过的坑" |

> 单价列请按实际购买价填。BOM 带价格能体现做过成本权衡。

## 接线表

| GC9A01 屏 | ESP32-S3 | 说明 |
|---|---|---|
| VCC | 3V3 | **绝对不要接 5V**，屏是 3.3V 逻辑 |
| GND | GND | 建议再引一根 GND 与 SCLK 双绞，见下方"踩过的坑" |
| SCL | GPIO12 | FSPI 默认 SCLK |
| SDA (MOSI) | GPIO11 | FSPI 默认 MOSI |
| CS | GPIO10 | FSPI 默认 CS |
| DC | GPIO9 | |
| RES | GPIO8 | |
| BLK | GPIO16 | 背光，GPIO 直驱；要调光需加 MOS 管 |
| MISO | 未接 | 屏不回传数据 |

12/11/10 正好是 ESP32-S3 的默认 FSPI 引脚，省掉一堆 IO MUX 配置。
避开了 Flash 专用脚 GPIO26–32，以及被板载 8MB OPI PSRAM 占用的 GPIO33–37。

## 软件栈

| 组件 | 版本 |
|---|---|
| ESP-IDF | v5.3.6 |
| LVGL | 8.3.11 |
| esp_lcd_gc9a01 | 1.2.0 |
| 编译器 | xtensa-esp-elf esp-13.2.0 |
| 目标芯片 | esp32s3 |

## 三步复现

```bash
# 1. 拉依赖（LVGL、GC9A01 驱动等由组件管理器自动下载）
idf.py set-target esp32s3

# 2. 编译
idf.py build

# 3. 烧录 + 看日志（串口换成你自己的 COM 口）
idf.py -p COM9 flash monitor
```

用 VSCode 的话装 ESP-IDF 插件，打开文件夹后点底部状态栏的火焰图标即可。
构建产物约 449 KB。

成功标志：圆屏出现会走动的模拟表盘，串口打印

```
I (xxx) clock: 时钟起点: 2026-11-01 20:30:00
I (xxx) example: Show analog watch face
I (xxx) watch_face: 表盘已创建，刷新周期 1000 ms
```

## 项目结构

```
smart-watch/
├── main/
│   ├── app_main.c      # 硬件初始化：SPI 总线、GC9A01 面板、LVGL 移植、背光
│   ├── clock.c/.h      # 走时模块（软件计时，预留 RTC 接口）
│   ├── watch_face.c/.h # 表盘 UI：刻度、数字、指针、日期
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig.defaults  # GC9A01、16MB Flash、USB 控制台等关键配置
└── .gitignore          # 忽略 build/、managed_components/、sdkconfig
```

`clock.c` 已经把 `clock_get()` / `clock_set()` 抽成接口，
后面接 DS3231 只改这一个文件的实现，表盘代码一行都不用动。

## 踩过的坑（都是真机实测出来的）

1. **IDF v5.3 内置 esp_lcd 只有 NT35510 / SSD1306 / ST7789，没有 GC9A01**
   → 通过组件管理器引入 `espressif/esp_lcd_gc9a01`。

2. **`CONFIG_LV_COLOR_16_SWAP=y` 不能删**
   GC9A01 按 RGB565 字节交换的方式收数据，删掉整屏颜色发紫。

3. **控制台默认从 UART0（GPIO43/44）输出**
   用板子原生 USB 口烧录时监视器一片空白。
   改成 `ESP_CONSOLE_USB_SERIAL_JTAG=y` 之后 `ESP_LOGI` 才走 USB。

4. **`sdkconfig.defaults` 不会覆盖已有的 `sdkconfig`**
   它只在 sdkconfig 首次生成时生效。改完默认值必须删掉 sdkconfig 重新
   `set-target`，配置才会真正应用。

5. **杜邦线跑 20MHz SPI 会在屏幕左边缘产生误码竖纹**
   已排除坐标偏移（`esp_lcd_gc9a01` 的 `x_gap`/`y_gap` 默认为 0）。
   解决方向：降 pclk、缩短 SCLK/MOSI、信号线与 GND 双绞。

6. **供电链路**
   TP4056 输出只有 3.7V，直接灌进开发板 5V 脚的话 AMS1117 压差不足，
   实测只剩约 2.6V，WiFi 一开就欠压重启。电池方案必须配 5V 升压模块。

## 路线图

- [x] 点亮屏幕，跑通 SPI + GC9A01 + LVGL 全链路
- [x] 指针式模拟表盘（刻度 + 数字 + 指针 + 日期）
- [ ] 演示 gif
- [ ] 功耗实测（深度睡眠电流 / 峰值电流 / 续航）
- [ ] DS3231 RTC，实现断电走时
- [ ] 电池 + 升压供电，脱离 USB
- [ ] 多页面滑动切换（表盘 / 计步 / 设置）
- [ ] 启用 8MB OPI PSRAM
- [ ] 3D 打印外壳

## License

MIT
