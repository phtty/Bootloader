#include "app_protocol.h"
#include "app_firmware.h"
#include "dev_console.h"
#include "dev_qspi_flash.h"
#include "pl_crc.h"
#include <string.h>

/* ---- DMA 接收缓冲区 ---- */
static uint8_t g_rx_buf[PROTO_MAX_FRAME];
static bool g_frame_ready;
static uint16_t g_rx_len;

static void rx_frame_callback(const uint8_t *data, size_t len)
{
	if (len <= PROTO_MAX_FRAME) {
		memcpy(g_rx_buf, data, len);
		g_rx_len	  = (uint16_t)len;
		g_frame_ready = true;
	}
	dev_console_start_rx(g_rx_buf, PROTO_MAX_FRAME);
}

/* ---- API 实现 ---- */

void app_protocol_init(void)
{
	g_frame_ready = false;
	dev_console_set_rx_callback(rx_frame_callback);
	dev_console_start_rx(g_rx_buf, PROTO_MAX_FRAME);
}

int32_t proto_recv_frame(proto_frame_t *frame, uint32_t timeout_ms)
{
	(void)timeout_ms;

	while (!g_frame_ready);
	g_frame_ready = false;

	if (g_rx_len < PROTO_HEADER_SIZE + PROTO_CRC_SIZE)
		return -2;

	memcpy(frame, g_rx_buf, g_rx_len);

	if (frame->sof != PROTO_SOF)
		return -2;

	/* 大端转主机序 */
	uint16_t plen = (uint16_t)((frame->payload_len >> 8) | (frame->payload_len << 8));
	if (plen > PROTO_MAX_PAYLOAD)
		return -2;

	uint16_t expected_len = (uint16_t)(PROTO_HEADER_SIZE + plen);
	if (g_rx_len != expected_len + PROTO_CRC_SIZE)
		return -2;

	uint16_t crc_calc = pl_crc16_calculate((const uint8_t *)frame, expected_len);
	uint16_t crc_recv = (uint16_t)((frame->crc16 >> 8) | (frame->crc16 << 8));
	if (crc_calc != crc_recv)
		return -2;

	frame->payload_len = plen;
	return 0;
}

int32_t proto_send_ack(uint8_t cmd, uint8_t ack_code, const uint8_t *data, uint16_t len)
{
	uint16_t plen_be, crc_val, payload_len;
	uint8_t tx_buf[PROTO_MAX_FRAME];
	uint8_t ack_payload[PROTO_MAX_PAYLOAD];

	payload_len	   = len + 1; /* 1 字节为原始命令码 */
	ack_payload[0] = cmd;
	if (data && len > 0)
		memcpy(&ack_payload[1], data, len);

	plen_be = (uint16_t)((payload_len >> 8) | (payload_len << 8));

	uint16_t total_len = (uint16_t)(PROTO_HEADER_SIZE + payload_len);

	/* 构造帧并计算 CRC */
	tx_buf[0] = PROTO_SOF;
	tx_buf[1] = ack_code;
	tx_buf[2] = (uint8_t)(plen_be >> 8);
	tx_buf[3] = (uint8_t)(plen_be & 0xFF);
	memcpy(&tx_buf[4], ack_payload, payload_len);

	crc_val				  = pl_crc16_calculate(tx_buf, total_len);
	tx_buf[total_len]	  = (uint8_t)(crc_val >> 8);
	tx_buf[total_len + 1] = (uint8_t)(crc_val & 0xFF);

	return dev_console_write(tx_buf, total_len + PROTO_CRC_SIZE);
}

/* ---- 帧处理状态机 ---- */
static firmware_ctx_t g_fw_ctx;

int32_t proto_handle_frame(proto_frame_t *frame)
{
	uint32_t addr;

	switch (frame->cmd) {
		case PROTO_CMD_SYNC: {
			dev_flash_info_t info;
			dev_flash_get_info(&info);
			proto_send_ack(PROTO_CMD_SYNC, PROTO_ACK_OK,
						   (const uint8_t *)&info, sizeof(info));
		} break;

		case PROTO_CMD_ERASE:
			if (frame->payload_len >= 4) {
				memcpy(&addr, frame->payload, 4);
				if (dev_flash_erase_sector(addr) == 0)
					proto_send_ack(PROTO_CMD_ERASE, PROTO_ACK_OK, NULL, 0);
				else
					proto_send_ack(PROTO_CMD_ERASE, PROTO_ACK_ERROR, NULL, 0);
			} else {
				proto_send_ack(PROTO_CMD_ERASE, PROTO_ACK_INVALID, NULL, 0);
			}
			break;

		case PROTO_CMD_WRITE:
			if (frame->payload_len > sizeof(proto_write_payload_t)) {
				const proto_write_payload_t *wp =
					(const proto_write_payload_t *)frame->payload;
				if (app_fw_write_data(&g_fw_ctx, wp->data, wp->data_len) == 0)
					proto_send_ack(PROTO_CMD_WRITE, PROTO_ACK_OK, NULL, 0);
				else
					proto_send_ack(PROTO_CMD_WRITE, PROTO_ACK_ERROR, NULL, 0);
			} else {
				proto_send_ack(PROTO_CMD_WRITE, PROTO_ACK_INVALID, NULL, 0);
			}
			break;

		case PROTO_CMD_VERIFY:
			if (g_fw_ctx.header_valid && g_fw_ctx.write_done) {
				proto_send_ack(PROTO_CMD_VERIFY, PROTO_ACK_OK, NULL, 0);
			} else {
				proto_send_ack(PROTO_CMD_VERIFY, PROTO_ACK_ERROR, NULL, 0);
			}
			break;

		case PROTO_CMD_LAUNCH:
			if (g_fw_ctx.header_valid) {
				proto_send_ack(PROTO_CMD_LAUNCH, PROTO_ACK_OK, NULL, 0);
				app_fw_jump_to_app(g_fw_ctx.header.entry_point);
			} else {
				proto_send_ack(PROTO_CMD_LAUNCH, PROTO_ACK_ERROR, NULL, 0);
			}
			break;

		default:
			proto_send_ack(frame->cmd, PROTO_ACK_INVALID, NULL, 0);
			break;
	}

	return 0;
}
