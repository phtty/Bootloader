/**
 * @file        pl_uart.h
 * @brief       平台层 USART 抽象接口（DMA 空闲中断模式）
 *
 * 封装 STM32H7 USART1 外设 + DMA1_Stream0 RX，
 * 提供阻塞收发、DMA 空闲中断接收和回调注册。
 * DMA1_Stream0 IRQ 在此层处理并转发至上层。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** @brief UART 平台层句柄（不透明指针） */
typedef void *pl_uart_handle_t;

/** @brief UART 配置参数 */
typedef struct {
    uint32_t baudrate;   /**< 波特率 */
    uint8_t  data_bits;  /**< 数据位（通常 8） */
    uint8_t  stop_bits;  /**< 停止位（通常 1） */
    char     parity;     /**< 校验：'N'=无, 'E'=偶, 'O'=奇 */
} pl_uart_config_t;

/**
 * @brief UART DMA 接收回调类型
 *
 * 在 HAL_UARTEx_RxEventCallback (ISR context) 中被调用，应尽快返回。
 * @param data 接收到的数据指针
 * @param len  数据长度（字节）
 */
typedef void (*pl_uart_rx_callback_t)(const uint8_t *data, size_t len);

/* ---- API ---- */

/**
 * @brief   UART1 初始化（device_initcall 自动调用）
 *
 * 115200-8N1, FIFO 阈值 1/8 后禁用 FIFO, DMA RX 使用 DMA1_Stream0。
 */
void pl_uart_init(void);

/** @brief 获取 UART 句柄 @return 句柄指针 */
pl_uart_handle_t pl_uart_get_handle(void);

/**
 * @brief   阻塞发送数据
 * @param   handle     UART 句柄
 * @param   buf        数据缓冲区
 * @param   len        发送字节数
 * @param   timeout_ms 超时时间（毫秒）
 * @return  成功返回实际发送字节数，失败返回 -1
 */
int32_t pl_uart_send(pl_uart_handle_t handle, const uint8_t *buf, size_t len, uint32_t timeout_ms);

/**
 * @brief   阻塞接收数据
 * @param   handle     UART 句柄
 * @param   buf        接收缓冲区
 * @param   len        期望接收字节数
 * @param   timeout_ms 超时时间（毫秒）
 * @return  成功返回实际接收字节数，失败返回 -1
 */
int32_t pl_uart_recv(pl_uart_handle_t handle, uint8_t *buf, size_t len, uint32_t timeout_ms);

/**
 * @brief   注册 DMA 空闲中断接收回调
 * @param   handle UART 句柄
 * @param   cb     回调函数指针
 */
void pl_uart_set_rx_callback(pl_uart_handle_t handle, pl_uart_rx_callback_t cb);

/**
 * @brief   检查 UART 是否忙
 * @param   handle UART 句柄
 * @retval  true  忙（正在发送/接收）
 * @retval  false 空闲
 */
bool pl_uart_is_busy(pl_uart_handle_t handle);

/**
 * @brief   清空 UART 接收寄存器
 * @param   handle UART 句柄
 */
void pl_uart_flush(pl_uart_handle_t handle);

/**
 * @brief   启动 DMA 空闲中断接收
 *
 * 使用 HAL_UARTEx_ReceiveToIdle_DMA()，接收到数据直到
 * 空闲线检测触发后回调 rx_callback。
 * @param   handle UART 句柄
 * @param   buf    接收缓冲区
 * @param   len    缓冲区最大容量
 * @return  成功返回 0，失败返回 -1
 */
int32_t pl_uart_start_dma_rx(pl_uart_handle_t handle, uint8_t *buf, size_t len);
