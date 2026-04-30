/**
 * @file        app_bootloader.h
 * @brief       Bootloader 应用主入口
 */

#pragma once

/**
 * @brief   Bootloader 主循环
 *
 * 初始化协议栈后进入无限循环：接收帧 -> 处理帧 -> 发送 ACK。
 * 调用后不会返回。
 */
void app_bootloader_run(void);
