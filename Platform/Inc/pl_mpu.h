/**
 * @file        pl_mpu.h
 * @brief       平台层 MPU（内存保护单元）配置
 *
 * 默认拒绝全部地址空间，仅开放 QSPI Flash 和 DTCM 两个区域。
 */

#pragma once

/**
 * @brief   配置 MPU 区域
 *
 * Region 0: 背景区域 — 拒绝 4GB 地址空间（SubRegionDisable=0x87 放行部分子区域）
 * Region 1: QSPI Flash 0x90000000, 8MB — 完全访问、可执行、可缓存
 * Region 2: DTCM 0x20000000, 1MB — 完全访问、不可缓存（保证 DMA 一致性）
 *
 * @note    在 board_init() 中最早调用，在 Cache 使能之前。
 */
void MPU_Config(void);
