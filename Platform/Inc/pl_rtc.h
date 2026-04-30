/**
 * @file        pl_rtc.h
 * @brief       平台层 RTC 实时时钟接口
 *
 * 封装 STM32H7 RTC 外设（LSI 32.768kHz 时钟源）。
 * 24 小时格式，异步预分频 127、同步预分频 255。
 */

#pragma once

#include <stdint.h>

/** @brief RTC 平台层句柄（不透明指针） */
typedef void *pl_rtc_handle_t;

/**
 * @brief   获取 RTC 句柄
 * @return  RTC 句柄指针
 */
pl_rtc_handle_t pl_rtc_get_handle(void);

/**
 * @brief   RTC 外设初始化（device_initcall 自动调用）
 *
 * 配置 RTC 为 24H 格式，时钟源选择 LSI (32.768kHz)。
 * @note   仅初始化外设和预分频器，不设置当前时间
 *         （时间应由应用程序或上位机同步）。
 */
void pl_rtc_init(void);
