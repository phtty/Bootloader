#pragma once

#include <stdint.h>
#include <stddef.h>

typedef void *pl_qspi_handle_t;

/* QSPI 总线宽度模式 */
typedef enum {
    PL_QSPI_MODE_SPI  = 0,  /* 单线 SPI（1 根数据线） */
    PL_QSPI_MODE_DUAL = 1,  /* 双线（2 根数据线） */
    PL_QSPI_MODE_QUAD = 2,  /* 四线（4 根数据线） */
} pl_qspi_mode_t;

/*
 * QSPI 命令描述符
 *
 * QSPI Flash 的每次操作都是一个"命令周期"，由 5 个阶段组成：
 *   指令 → 地址 → 交替字节 → 空周期 → 数据
 *
 * 每个阶段可以独立选择使用的线宽（单线/双线/四线）。
 * 并非所有命令都需要全部 5 个阶段，不需要的阶段长度置零即可。
 *
 * 举例：读取 W25Q64 Flash 状态寄存器
 *   instruction=0x05, instr_mode=SPI, data_mode=SPI,
 *   addr_size=0, dummy_cycles=0  (因为该命令不需要地址和空周期)
 */
typedef struct {
    uint8_t       instruction;   /* 命令字节，如 0x03=READ, 0x02=PageProgram, 0x05=ReadStatus */
    pl_qspi_mode_t instr_mode;   /* 发送 instruction 时使用的线宽 */
    pl_qspi_mode_t addr_mode;    /* 发送地址时使用的线宽 */
    pl_qspi_mode_t data_mode;    /* 读写数据时使用的线宽 */
    pl_qspi_mode_t alt_mode;     /* 发送交替字节时使用的线宽（极少使用） */
    uint32_t      addr_size;     /* 地址字节数：0=无地址, 3=24bit地址, 4=32bit地址 */
    uint32_t      dummy_cycles;  /* 指令与数据之间的空时钟周期数（READ 命令通常需要） */
    uint32_t      alt_bytes;     /* 交替字节（极少数命令需要，通常为 0） */
} pl_qspi_command_t;

/* ---- 平台层对外接口 ---- */

void      pl_qspi_init(void);
pl_qspi_handle_t pl_qspi_get_handle(void);

/* 从 QSPI Flash 读取数据 */
int32_t   pl_qspi_read(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, uint8_t *buf, size_t len);
/* 向 QSPI Flash 写入数据 */
int32_t   pl_qspi_write(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, const uint8_t *buf, size_t len);
/* 仅发送命令（无数据阶段），如写使能、擦除指令 */
int32_t   pl_qspi_send_cmd(pl_qspi_handle_t handle, pl_qspi_command_t *cmd);
/* 将 QSPI Flash 映射到内存空间（0x90000000），之后可直接通过指针访问 */
int32_t   pl_qspi_memory_mapped(pl_qspi_handle_t handle);
/* 读取 QSPI 外设状态寄存器 */
int32_t   pl_qspi_get_status(pl_qspi_handle_t handle);
/* 中止正在进行的 QSPI 传输 */
void      pl_qspi_abort(pl_qspi_handle_t handle);
