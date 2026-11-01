/*
 * SPDX-License-Identifier: MIT
 */

#include "clock.h"

#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "esp_log.h"

static const char *TAG = "clock";

// 没有 RTC 时的默认起点。接上 DS3231 后这里会被替换成读硬件。
#define DEFAULT_YEAR  2026
#define DEFAULT_MONTH 11
#define DEFAULT_DAY   1
#define DEFAULT_HOUR  20
#define DEFAULT_MIN   30
#define DEFAULT_SEC   0

void clock_init(void)
{
    clock_set(DEFAULT_YEAR, DEFAULT_MONTH, DEFAULT_DAY,
              DEFAULT_HOUR, DEFAULT_MIN, DEFAULT_SEC);

    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);
    ESP_LOGI(TAG, "时钟起点: %04d-%02d-%02d %02d:%02d:%02d",
             tm_now.tm_year + 1900, tm_now.tm_mon + 1, tm_now.tm_mday,
             tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec);
}

void clock_set(int year, int month, int day, int hour, int minute, int second)
{
    struct tm tm_val;
    memset(&tm_val, 0, sizeof(tm_val));
    tm_val.tm_year = year - 1900;
    tm_val.tm_mon  = month - 1;
    tm_val.tm_mday = day;
    tm_val.tm_hour = hour;
    tm_val.tm_min  = minute;
    tm_val.tm_sec  = second;

    time_t ts = mktime(&tm_val);
    struct timeval tv = {
        .tv_sec  = ts,
        .tv_usec = 0,
    };
    settimeofday(&tv, NULL);
}

void clock_get(clock_time_t *out)
{
    time_t now = time(NULL);
    struct tm tm_now;
    localtime_r(&now, &tm_now);

    out->hour   = tm_now.tm_hour;
    out->minute = tm_now.tm_min;
    out->second = tm_now.tm_sec;
    out->wday   = tm_now.tm_wday;
    out->day    = tm_now.tm_mday;
    out->month  = tm_now.tm_mon + 1;
    out->year   = tm_now.tm_year + 1900;
}
