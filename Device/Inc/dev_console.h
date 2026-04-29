#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief 控制台 DMA 接收回调类型
 * @param data  接收到的数据指针
 * @param len   数据长度（字节）
 */
typedef void (*dev_console_rx_callback_t)(const uint8_t *data, size_t len);

/* ================================================================
 *  控制台设备驱动接口（Application 层直接调用）
 * ================================================================ */

/**
 * @brief 初始化控制台设备（绑定 UART 并注册 DMA 回调）
 * @return 成功返回 0，失败返回 -1
 */
int32_t dev_console_init(void);

/**
 * @brief 输出一个字符
 * @param c  要输出的字符
 * @return 成功返回 0，失败返回 -1
 */
int32_t dev_console_putc(char c);

/**
 * @brief 输出一个字符串（以 '\\0' 结尾）
 * @param s  要输出的字符串
 * @return 成功返回 0，失败返回 -1
 */
int32_t dev_console_puts(const char *s);

/**
 * @brief 阻塞读取一个字符
 * @param timeout_ms  超时时间（毫秒）
 * @return 成功返回读取到的字符（0-255），超时或失败返回 -1
 */
int32_t dev_console_getc(uint32_t timeout_ms);

/**
 * @brief 阻塞发送二进制数据
 * @param data  数据缓冲区
 * @param len   发送字节数
 * @return 成功返回实际发送字节数，失败返回 -1
 */
int32_t dev_console_write(const uint8_t *data, size_t len);

/**
 * @brief 阻塞接收二进制数据
 * @param buf        接收缓冲区
 * @param len        期望接收字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return 成功返回实际接收字节数，超时或失败返回 -1
 */
int32_t dev_console_read(uint8_t *buf, size_t len, uint32_t timeout_ms);

/**
 * @brief 注册 DMA 空闲中断接收回调
 *
 * 注册后，当 UART 接收到一帧完整数据（空闲中断触发）时，
 * 会回调此函数。回调在 ISR 上下文中执行，需尽快返回。
 * @param cb  回调函数指针
 */
void dev_console_set_rx_callback(dev_console_rx_callback_t cb);

/**
 * @brief 启动 DMA 空闲中断接收
 *
 * 配置 DMA 将 UART 接收数据自动写入 buf，当检测到空闲线时
 * 触发 rx_callback。
 * @param buf  接收缓冲区
 * @param len  缓冲区最大容量
 * @return 成功返回 0，失败返回 -1
 * @note   每次接收到完整帧后需重新调用此函数启动下一次接收
 */
int32_t dev_console_start_rx(uint8_t *buf, size_t len);
