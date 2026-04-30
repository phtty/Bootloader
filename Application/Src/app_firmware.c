#include "app_firmware.h"
#include "dev_qspi_flash.h"
#include "pl_crc.h"
#include "stm32h7xx_hal.h"
#include <string.h>

uint32_t app_fw_crc32(uint32_t crc, const uint8_t *buf, size_t len)
{
	if (crc == 0) {
		pl_crc32_reset();
	} else {
		CRC->DR = crc;
	}
	pl_crc32_feed(buf, len);
	return pl_crc32_get();
}

bool app_fw_validate_header(const firmware_header_t *header)
{
	if (header->magic != FIRMWARE_MAGIC)
		return false;

	if (header->image_size == 0 || header->entry_point == 0)
		return false;

	/* 验证头部 CRC（不含 header_crc 字段本身）*/
	uint32_t crc = app_fw_crc32(0, (const uint8_t *)header,
								FIRMWARE_HEADER_SIZE - sizeof(uint32_t));
	return (crc == header->header_crc);
}

bool app_fw_validate_image(const uint8_t *data, size_t size, uint32_t expected_crc)
{
	uint32_t crc = app_fw_crc32(0, data, size);
	return (crc == expected_crc);
}

int32_t app_fw_write_data(firmware_ctx_t *ctx, const uint8_t *data, size_t len)
{
	if (!ctx->header_valid) {
		if (len < FIRMWARE_HEADER_SIZE)
			return -1;

		memcpy(&ctx->header, data, FIRMWARE_HEADER_SIZE);
		if (!app_fw_validate_header(&ctx->header))
			return -1;

		ctx->header_valid  = true;
		ctx->total_size	   = FIRMWARE_HEADER_SIZE + ctx->header.image_size;
		ctx->target_addr   = ctx->header.load_address;
		ctx->bytes_written = 0;
		ctx->crc32_accum   = 0;
		ctx->write_done	   = false;

		data += FIRMWARE_HEADER_SIZE;
		len -= FIRMWARE_HEADER_SIZE;
	}

	if (len == 0)
		return 0;

	if (dev_flash_write(ctx->target_addr + ctx->bytes_written, data, len) != (int32_t)len)
		return -1;

	ctx->crc32_accum = app_fw_crc32(ctx->crc32_accum, data, len);
	ctx->bytes_written += (uint32_t)len;

	if (ctx->bytes_written >= ctx->header.image_size)
		ctx->write_done = true;

	return 0;
}

void app_fw_jump_to_app(uint32_t entry_addr)
{
	/* 1. 将 QSPI Flash 配置为 memory-mapped 模式，之后 0x90000000 可直接寻址 */
	dev_flash_memory_mapped();

	/* 2. 关闭全局中断，停止 SysTick */
	__disable_irq();
	SysTick->CTRL = 0;

	/* 3. 清理 D-Cache（确保 bootloader 写入 Flash 的数据对 AHB 总线可见） */
	SCB_CleanDCache();

	/* 4. 反初始化 USART1 和 DMA（QSPI 保持不动，memory-mapped 依赖其时钟） */
	__HAL_RCC_USART1_CLK_DISABLE();
	__HAL_RCC_DMA1_CLK_DISABLE();

	/* 5. 将 VTOR 指向新固件的向量表 */
	SCB->VTOR = entry_addr;
	__DSB();
	__ISB();

	/* 6. 设置 MSP（向量表第一个字），跳转到 Reset_Handler（向量表第二个字） */
	uint32_t app_msp = *(volatile uint32_t *)(entry_addr);
	__set_MSP(app_msp);

	uint32_t app_reset = *(volatile uint32_t *)(entry_addr + 4);
	((void (*)(void))app_reset)();

	for (;;) {}
}
