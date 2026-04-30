/**
 * @file        pl_dma.h
 * @brief       平台层 DMA 初始化
 *
 * 使能 DMA1 时钟并配置 DMA1_Stream0 IRQ (USART1 RX)。
 * 注册于 subsys_initcall（优先级 2），在 UART 外设初始化前执行。
 */

#pragma once

/**
 * @brief   DMA1 时钟使能与 IRQ 配置
 *
 * 使能 DMA1 时钟，设置 DMA1_Stream0 IRQ 优先级为 0/0（最高），并使能中断。
 */
void pl_dma_init(void);
