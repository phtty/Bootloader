/**
 * @file        pl_gpio.h
 * @brief       平台层 GPIO 初始化
 *
 * 使能所有用到的 GPIO 端口时钟（Port A~E, H）。
 * 注册于 arch_initcall (优先级 1)，在所有外设初始化前执行。
 */

#pragma once

/**
 * @brief   GPIO 端口时钟使能
 *
 * 使能 GPIOA~E, GPIOH 时钟。实际引脚复用配置
 * 由各外设的 HAL_xxx_MspInit() 回调完成。
 */
void pl_gpio_init(void);
