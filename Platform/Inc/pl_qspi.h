/**
 * @file        pl_qspi.h
 * @brief       平台层 QSPI 外设抽象接口
 *
 * 封装 STM32H7 QUADSPI 外设，提供命令发送、数据读写和
 * 内存映射模式的统一接口。上层 Device 层通过此接口操作
 * QSPI Flash，无需直接操作 HAL。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/** @brief QSPI 平台层句柄（不透明指针） */
typedef void *pl_qspi_handle_t;

/** @brief QSPI 总线宽度模式 */
typedef enum {
    PL_QSPI_MODE_SPI  = 0,  /**< 单线 SPI（1 根数据线） */
    PL_QSPI_MODE_DUAL = 1,  /**< 双线（2 根数据线） */
    PL_QSPI_MODE_QUAD = 2,  /**< 四线（4 根数据线） */
} pl_qspi_mode_t;

/**
 * @brief QSPI 命令描述符
 *
 * QSPI Flash 每次操作由 5 个阶段组成：
 *   指令 → 地址 → 交替字节 → 空周期 → 数据
 *
 * 每个阶段可独立选择线宽。不需要的阶段长度置零即可。
 *
 * @example 读取 W25Q64 状态寄存器：
 *          instruction=0x05, instr_mode=SPI, data_mode=SPI,
 *          addr_size=0, dummy_cycles=0
 */
typedef struct {
    uint8_t        instruction;   /**< 命令字节，如 0x03=READ, 0x20=SectorErase */
    pl_qspi_mode_t instr_mode;    /**< 发送 instruction 时使用的线宽 */
    pl_qspi_mode_t addr_mode;     /**< 发送地址时使用的线宽 */
    pl_qspi_mode_t data_mode;     /**< 读写数据时使用的线宽 */
    pl_qspi_mode_t alt_mode;      /**< 发送交替字节时使用的线宽（极少使用） */
    uint32_t       addr_size;     /**< 地址字节数：0=无地址, 3=24bit, 4=32bit */
    uint32_t       dummy_cycles;  /**< 指令与数据之间的空时钟周期数 */
    uint32_t       alt_bytes;     /**< 交替字节值（极少使用，通常为 0） */
} pl_qspi_command_t;

/**
 * @brief Memory-mapped 模式的读取命令配置
 *
 * QSPI 进入 memory-mapped 模式后，任何对 0x90000000 的 AHB 读访问
 * 都会自动触发 QSPI Flash 读取命令。
 *
 * 常用配置（取决于 Flash 芯片和 QE 位）：
 *   - Fast Read:        0x0B, 1-1-1, 8 dummy
 *   - Quad I/O Fast Read: 0xEB, 1-4-4, 4 dummy (需要 QE=1, 性能最优)
 */
typedef struct {
    uint8_t        instruction;    /**< 读取命令字节 */
    pl_qspi_mode_t instr_mode;     /**< 指令发送线宽 */
    pl_qspi_mode_t addr_mode;      /**< 地址发送线宽 */
    pl_qspi_mode_t data_mode;      /**< 数据读取线宽 */
    uint32_t       addr_size;      /**< 地址字节数 */
    uint32_t       dummy_cycles;   /**< 空周期数 */
} pl_qspi_mmap_cfg_t;

/* ---- 平台层对外接口 ---- */

/**
 * @brief   QSPI 外设初始化（device_initcall 自动调用）
 *
 * 配置 QUADSPI：单 Bank, FlashSize=23 (8MB), prescaler=1 (75MHz),
 * 半周期采样偏移, ClockMode=0。
 */
void pl_qspi_init(void);

/** @brief 获取 QSPI 平台层句柄 @return 句柄指针 */
pl_qspi_handle_t pl_qspi_get_handle(void);

/**
 * @brief   从 QSPI Flash 读取数据
 * @param   handle QSPI 句柄
 * @param   cmd    QSPI 命令描述符
 * @param   buf    接收缓冲区
 * @param   len    读取字节数
 * @return  成功返回实际读取字节数，失败返回 -1
 */
int32_t pl_qspi_read(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, uint8_t *buf, size_t len);

/**
 * @brief   向 QSPI Flash 写入数据
 * @param   handle QSPI 句柄
 * @param   cmd    QSPI 命令描述符
 * @param   buf    数据缓冲区
 * @param   len    写入字节数
 * @return  成功返回实际写入字节数，失败返回 -1
 */
int32_t pl_qspi_write(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, const uint8_t *buf, size_t len);

/**
 * @brief   仅发送命令（无数据阶段），如 WriteEnable、Erase
 * @param   handle QSPI 句柄
 * @param   cmd    QSPI 命令描述符
 * @return  成功返回 0，失败返回 -1
 */
int32_t pl_qspi_send_cmd(pl_qspi_handle_t handle, pl_qspi_command_t *cmd);

/**
 * @brief   将 QSPI Flash 映射到内存地址空间（0x90000000）
 *
 * 映射后可通过普通指针直接读取 Flash 内容，用于 XIP 固件执行。
 * @param   handle QSPI 句柄
 * @param   cfg    Memory-mapped 读取命令配置
 * @return  成功返回 0，失败返回 -1
 */
int32_t pl_qspi_memory_mapped(pl_qspi_handle_t handle, const pl_qspi_mmap_cfg_t *cfg);

/**
 * @brief   读取 QSPI 外设状态
 * @param   handle QSPI 句柄
 * @return  QSPI 状态寄存器值，失败返回 -1
 */
int32_t pl_qspi_get_status(pl_qspi_handle_t handle);

/**
 * @brief   中止正在进行的 QSPI 传输
 * @param   handle QSPI 句柄
 */
void pl_qspi_abort(pl_qspi_handle_t handle);
