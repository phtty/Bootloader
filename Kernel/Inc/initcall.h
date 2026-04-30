/**
 * @file        initcall.h
 * @brief       类 Linux initcall 自动初始化框架
 *
 * 将各模块的初始化函数按优先级放入 linker 的 .initcall.N 段，
 * board_init() 自动按序调用，无需手动维护初始化顺序列表。
 *
 * 优先级（数字越小越早执行）：
 *   1. arch_initcall    — 架构级（GPIO 时钟）
 *   2. subsys_initcall  — 子系统级（DMA、IRQ）
 *   3. device_initcall  — 外设级（QSPI, UART, CRC, RTC）
 *   4. driver_initcall  — 驱动级（Flash 芯片, Console）
 *   5. late_initcall    — 后期初始化（预留）
 *
 * 用法：
 * @code
 *   #include "initcall.h"
 *   void my_driver_init(void) { ... }
 *   driver_initcall(my_driver_init);
 * @endcode
 */

#pragma once

#include <stdint.h>

/** @brief 初始化函数指针类型 */
typedef void (*initcall_fn)(void);

/** @brief initcall 表项（函数指针 + 名称，用于调试） */
typedef struct {
    initcall_fn fn;       /**< 初始化函数指针 */
    const char *name;     /**< 函数名称（编译期确定） */
} initcall_entry_t;

/** @brief 将函数放入指定优先级的 initcall 段 */
#define OS_INITCALL(lvl, fn) \
    static const initcall_entry_t __attribute__((used, section(".initcall." #lvl))) \
    __initcall_##lvl##_##fn = { (initcall_fn)(fn), #fn }

#define arch_initcall(fn)     OS_INITCALL(1, fn)  /**< 架构级 (1) */
#define subsys_initcall(fn)   OS_INITCALL(2, fn)  /**< 子系统级 (2) */
#define device_initcall(fn)   OS_INITCALL(3, fn)  /**< 外设级 (3) */
#define driver_initcall(fn)   OS_INITCALL(4, fn)  /**< 驱动级 (4) */
#define late_initcall(fn)     OS_INITCALL(5, fn)  /**< 后期初始化 (5) */

/** @brief initcall 段起始符号（linker 定义） */
extern initcall_entry_t __initcall_start[];
/** @brief initcall 段结束符号（linker 定义） */
extern initcall_entry_t __initcall_end[];

/**
 * @brief   板级初始化入口
 *
 * 阶段 1: 手动按序调用 MPU_Config → Cache → HAL_Init → 时钟配置
 * 阶段 2: 自动遍历所有 initcall 段中的初始化函数
 *
 * @note    由 main() 调用，在进入 app_bootloader_run() 之前执行。
 */
void board_init(void);
