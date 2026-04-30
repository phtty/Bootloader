/**
 * @file        pl_clock.h
 * @brief       平台层系统时钟与外设共用时钟配置
 *
 * 时钟树（HSE = 25MHz）：
 *   PLL1: /5 ×192 /2 → SYSCLK 480MHz, HCLK 240MHz, APB 120MHz
 *   PLL2: /16 ×192 /2 → Q=75MHz(USART1), R=150MHz(QSPI)
 *   QSPI: R=150MHz ÷2(prescaler=1) = 75MHz
 *   LSI: 32.768kHz (RTC 时钟源)
 *
 * PWR: LDO 供电, voltage scaling 0（最高性能）
 */

#pragma once

/**
 * @brief   系统时钟配置
 *
 * 初始化 HSE、PLL1、AHB/APB 总线时钟和 Flash 延迟。
 * 在 HAL_Init() 之后、外设初始化之前调用。
 */
void SystemClock_Config(void);

/**
 * @brief   外设共用时钟配置
 *
 * 配置 PLL2 输出 Q (USART1) 和 R (QSPI) 的时钟源选择。
 * 在 SystemClock_Config() 之后调用。
 */
void PeriphCommonClock_Config(void);
