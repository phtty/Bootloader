#pragma once

#include <stdint.h>
#include <stddef.h>

void pl_crc_init(void);

/* CRC-16-CCITT (协议帧校验, poly=0x1021, init=0xFFFF) */
uint16_t pl_crc16_calculate(const uint8_t *data, size_t len);

/* CRC32-MPEG2 (固件镜像校验, poly=0x04C11DB7) */
void pl_crc32_reset(void);
void pl_crc32_feed(const uint8_t *data, size_t len);
uint32_t pl_crc32_get(void);
