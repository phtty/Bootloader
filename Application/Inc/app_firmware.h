#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief 固件镜像头部（位于固件数据块最前面）
 *
 * 上位机打包时在固件 bin 文件前面附加此头部，
 * Bootloader 通过头部信息确定固件大小、入口地址等。
 */
typedef struct __attribute__((packed)) {
    uint32_t magic;          /**< 魔数 0x0D000721 */
    uint32_t version;        /**< 固件版本号（如 0x00010000 = v1.0.0） */
    uint32_t image_size;     /**< 固件镜像大小（字节，不含头部） */
    uint32_t entry_point;    /**< 固件入口地址 */
    uint32_t load_address;   /**< 固件加载基地址 */
    uint32_t image_crc32;    /**< 固件镜像的 CRC32 校验值 */
    uint32_t header_crc;     /**< 本头部自身的 CRC32（不含本字段） */
} firmware_header_t;

#define FIRMWARE_MAGIC       0x0D000721u
#define FIRMWARE_HEADER_SIZE sizeof(firmware_header_t)

/**
 * @brief 固件写入上下文
 */
typedef struct {
    firmware_header_t header;        /**< 固件头部 */
    uint32_t          target_addr;   /**< QSPI Flash 中的写入目标地址 */
    uint32_t          bytes_written; /**< 已写入字节数 */
    uint32_t          total_size;    /**< 总大小（含头部） */
    uint32_t          crc32_accum;   /**< CRC32 累计值 */
    bool              header_valid;  /**< 头部是否已校验通过 */
    bool              write_done;    /**< 写入是否已完成 */
} firmware_ctx_t;

/* ---- API ---- */

bool app_fw_validate_header(const firmware_header_t *header);
bool app_fw_validate_image(const uint8_t *data, size_t size, uint32_t expected_crc);
int32_t app_fw_write_data(firmware_ctx_t *ctx, const uint8_t *data, size_t len);
void app_fw_jump_to_app(uint32_t entry_addr);

uint32_t app_fw_crc32(uint32_t crc, const uint8_t *buf, size_t len);
