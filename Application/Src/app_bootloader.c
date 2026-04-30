#include "app_bootloader.h"
#include "app_protocol.h"
#include "dev_console.h"

/*
 * Bootloader 主状态机
 *
 * 启动后初始化协议栈，进入事件循环：
 *   等待 DMA 接收完整帧 → 校验 CRC → 分发处理 → 回复 ACK
 */
void app_bootloader_run(void)
{
	proto_frame_t frame;

	/* 初始化协议栈（注册 DMA 回调，启动接收） */
	app_protocol_init();

	dev_console_puts("Bootloader ready.\r\n");

	for (;;) {
		/* 阻塞等待一帧完整数据 */
		if (proto_recv_frame(&frame, 0) != 0)
			continue;

		/* 分发到协议处理 */
		proto_handle_frame(&frame);
	}
}
