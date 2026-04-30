/**
 * @file        pl_crc.h
 * @brief       平台层硬件 CRC 计算接口
 *
 * 封装 STM32H7 CRC 外设，支持：
 *   - CRC-16-CCITT (poly=0x1021, init=0xFFFF) — 协议帧校验
 *   - CRC32-MPEG2 (poly=0x04C11DB7) — 固件镜像校验
 *
 * CRC16 计算时会自动保存/恢复 CRC32 累加器状态，
 * 确保协议帧校验不会破坏固件流式 CRC32 的中间结果。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/**
 * @brief   CRC 外设初始化（device_initcall 自动调用）
 *
 * 默认配置为 CRC32-MPEG2 模式，使能 CRC 时钟。
 */
void pl_crc_init(void);

/**
 * @brief   一次性的 CRC-16-CCITT 计算
 * @param   data 数据缓冲区
 * @param   len  数据长度（字节）
 * @return  CRC16 校验值
 */
uint16_t pl_crc16_calculate(const uint8_t *data, size_t len);

/**
 * @brief   重置 CRC32 累加器
 *
 * 开始新的流式 CRC32 计算前调用。
 * 将 CRC 单元初始化为默认 Ethernet 多项式 (0x04C11DB7)、
 * 初始值 0xFFFFFFFF 并复位。
 */
void pl_crc32_reset(void);

/**
 * @brief   向 CRC32 累加器喂入数据（流式）
 *
 * 可在多次调用间累积 CRC，适用于分块收发的固件写入场景。
 * @param   data 数据缓冲区
 * @param   len  数据长度（字节）
 */
void pl_crc32_feed(const uint8_t *data, size_t len);

/**
 * @brief   获取当前 CRC32 累加值
 * @return  当前的 CRC32 结果
 */
uint32_t pl_crc32_get(void);
