/**
 * @file        app_firmware.h
 * @brief       固件镜像管理与校验 API
 *
 * 固件镜像由"28 字节头部 + 固件 bin"组成，头部包含魔数、版本、
 * 大小、入口地址、CRC32 等元数据。Bootloader 通过头部校验和
 * CRC32 完整性检查确保固件有效后再跳转执行。
 *
 * CRC32 使用 STM32H7 硬件 CRC 外设（Ethernet polynomial 0x04C11DB7）。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 固件镜像头部（packed，28 字节）
 *
 * 上位机打包时在固件 bin 前面附加此头部。
 * Bootloader 通过头部信息确定固件大小、入口地址等。
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /**< 魔数 0x0D000721 */
    uint32_t version;        /**< 固件版本号（如 0x00010000 = v1.0.0） */
    uint32_t image_size;     /**< 固件镜像大小（字节，不含头部） */
    uint32_t entry_point;    /**< 固件入口地址（Reset_Handler） */
    uint32_t load_address;   /**< 固件加载基地址（QSPI Flash 偏移） */
    uint32_t image_crc32;    /**< 固件镜像的 CRC32 校验值 */
    uint32_t header_crc;     /**< 本头部自身的 CRC32（计算时不包含本字段） */
} firmware_header_t;

/** @brief 固件魔数 @hideinitializer */
#define FIRMWARE_MAGIC       0x0D000721u
/** @brief 固件头部大小（字节） @hideinitializer */
#define FIRMWARE_HEADER_SIZE sizeof(firmware_header_t)

/**
 * @brief 固件写入上下文
 *
 * 在多次 WRITE 命令间维持写入状态，支持断点续传式的
 * 数据块累积写入和 CRC32 递增校验。
 */
typedef struct {
    firmware_header_t header;        /**< 固件头部 */
    uint32_t          target_addr;   /**< QSPI Flash 中的写入目标地址 */
    uint32_t          bytes_written; /**< 已写入字节数（不含头部） */
    uint32_t          total_size;    /**< 总大小（头部 + 固件，字节） */
    uint32_t          crc32_accum;   /**< CRC32 累加值（增量校验） */
    bool              header_valid;  /**< 头部是否已校验通过 */
    bool              write_done;    /**< 写入是否已完成 */
} firmware_ctx_t;

/* ---- API ---- */

/**
 * @brief   校验固件头部合法性
 *
 * 检查魔数、image_size、entry_point 非零，以及头部自身的 CRC32。
 *
 * @param   header 固件头部指针
 * @retval  true  头部合法
 * @retval  false 魔数/大小/入口/CRC 任一不匹配
 */
bool app_fw_validate_header(const firmware_header_t *header);

/**
 * @brief   校验固件镜像 CRC32
 * @param   data         固件镜像数据
 * @param   size         数据大小（字节）
 * @param   expected_crc 期望的 CRC32 值
 * @retval  true  CRC32 匹配
 * @retval  false CRC32 不匹配
 */
bool app_fw_validate_image(const uint8_t *data, size_t size, uint32_t expected_crc);

/**
 * @brief   写入固件数据块到 QSPI Flash
 *
 * 首次调用时提取并校验头部，后续调用累积写入固件数据。
 * 同步更新 ctx->crc32_accum（增量 CRC32）、bytes_written 和 write_done。
 *
 * @param   ctx  固件写入上下文
 * @param   data 数据块指针
 * @param   len  数据块长度（字节）
 * @retval  0  成功
 * @retval -1  头部校验失败 / Flash 写入失败
 */
int32_t app_fw_write_data(firmware_ctx_t *ctx, const uint8_t *data, size_t len);

/**
 * @brief   跳转到应用程序固件
 *
 * 执行以下流程后永不返回：
 * 1. QSPI 切换为内存映射模式
 * 2. 关闭全局中断、停止 SysTick
 * 3. 清理 D-Cache
 * 4. 关闭 USART1 / DMA1 时钟
 * 5. 设置 VTOR → 加载 MSP → 跳转 Reset_Handler
 *
 * @param   entry_addr 固件入口地址（Vector Table 基址）
 */
void app_fw_jump_to_app(uint32_t entry_addr);

/**
 * @brief   流式 CRC32 计算（硬件 CRC 外设）
 *
 * 调用前确保 CRC 时钟已使能（由 Platform 层自动初始化）。
 *
 * @param   crc 初始/累计 CRC 值（首次调用传 0）
 * @param   buf 数据缓冲区
 * @param   len 数据长度（字节）
 * @return  更新后的 CRC32 值
 */
uint32_t app_fw_crc32(uint32_t crc, const uint8_t *buf, size_t len);
