#include "app_protocol.h"
#include "app_firmware.h"
#include "dev_console.h"
#include "dev_qspi_flash.h"
#include <string.h>

/*
 * TODO: 替换为 STM32H7 硬件 CRC
 *
 * H7 CRC 单元可配置 16 位多项式模式，
 * 使用 HAL_CRC_Calculate() 替代软件查表实现。
 * 使能 CRC 时钟后，可删除本 CRC-16 查表函数。
 */
static uint16_t crc16_sw(const uint8_t *data, size_t len)
{
	static const uint16_t table[256] = {
		0x0000,
		0x1021,
		0x2042,
		0x3063,
		0x4084,
		0x50A5,
		0x60C6,
		0x70E7,
		0x8108,
		0x9129,
		0xA14A,
		0xB16B,
		0xC18C,
		0xD1AD,
		0xE1CE,
		0xF1EF,
		0x1231,
		0x0210,
		0x3273,
		0x2252,
		0x52B5,
		0x4294,
		0x72F7,
		0x62D6,
		0x9339,
		0x8318,
		0xB37B,
		0xA35A,
		0xD3BD,
		0xC39C,
		0xF3FF,
		0xE3DE,
		0x2462,
		0x3443,
		0x0420,
		0x1401,
		0x64E6,
		0x74C7,
		0x44A4,
		0x5485,
		0xA56A,
		0xB54B,
		0x8528,
		0x9509,
		0xE5EE,
		0xF5CF,
		0xC5AC,
		0xD58D,
		0x3653,
		0x2672,
		0x1611,
		0x0630,
		0x76D7,
		0x66F6,
		0x5695,
		0x46B4,
		0xB75B,
		0xA77A,
		0x9719,
		0x8738,
		0xF7DF,
		0xE7FE,
		0xD79D,
		0xC7BC,
		0x48C4,
		0x58E5,
		0x6886,
		0x78A7,
		0x0840,
		0x1861,
		0x2802,
		0x3823,
		0xC9CC,
		0xD9ED,
		0xE98E,
		0xF9AF,
		0x8948,
		0x9969,
		0xA90A,
		0xB92B,
		0x5AF5,
		0x4AD4,
		0x7AB7,
		0x6A96,
		0x1A71,
		0x0A50,
		0x3A33,
		0x2A12,
		0xDBFD,
		0xCBDC,
		0xFBBF,
		0xEB9E,
		0x9B79,
		0x8B58,
		0xBB3B,
		0xAB1A,
		0x6CA6,
		0x7C87,
		0x4CE4,
		0x5CC5,
		0x2C22,
		0x3C03,
		0x0C60,
		0x1C41,
		0xEDAE,
		0xFD8F,
		0xCDEC,
		0xDDCD,
		0xAD2A,
		0xBD0B,
		0x8D68,
		0x9D49,
		0x7E97,
		0x6EB6,
		0x5ED5,
		0x4EF4,
		0x3E13,
		0x2E32,
		0x1E51,
		0x0E70,
		0xFF9F,
		0xEFBE,
		0xDFDD,
		0xCFFC,
		0xBF1B,
		0xAF3A,
		0x9F59,
		0x8F78,
		0x9188,
		0x81A9,
		0xB1CA,
		0xA1EB,
		0xD10C,
		0xC12D,
		0xF14E,
		0xE16F,
		0x1080,
		0x00A1,
		0x30C2,
		0x20E3,
		0x5004,
		0x4025,
		0x7046,
		0x6067,
		0x83B9,
		0x9398,
		0xA3FB,
		0xB3DA,
		0xC33D,
		0xD31C,
		0xE37F,
		0xF35E,
		0x02B1,
		0x1290,
		0x22F3,
		0x32D2,
		0x4235,
		0x5214,
		0x6277,
		0x7256,
		0xB5EA,
		0xA5CB,
		0x95A8,
		0x8589,
		0xF56E,
		0xE54F,
		0xD52C,
		0xC50D,
		0x34E2,
		0x24C3,
		0x14A0,
		0x0481,
		0x7466,
		0x6447,
		0x5424,
		0x4405,
		0xA7DB,
		0xB7FA,
		0x8799,
		0x97B8,
		0xE75F,
		0xF77E,
		0xC71D,
		0xD73C,
		0x26D3,
		0x36F2,
		0x0691,
		0x16B0,
		0x6657,
		0x7676,
		0x4615,
		0x5634,
		0xD94C,
		0xC96D,
		0xF90E,
		0xE92F,
		0x99C8,
		0x89E9,
		0xB98A,
		0xA9AB,
		0x5844,
		0x4865,
		0x7806,
		0x6827,
		0x18C0,
		0x08E1,
		0x3882,
		0x28A3,
		0xCB7D,
		0xDB5C,
		0xEB3F,
		0xFB1E,
		0x8BF9,
		0x9BD8,
		0xABBB,
		0xBB9A,
		0x4A75,
		0x5A54,
		0x6A37,
		0x7A16,
		0x0AF1,
		0x1AD0,
		0x2AB3,
		0x3A92,
		0xFD2E,
		0xED0F,
		0xDD6C,
		0xCD4D,
		0xBDAA,
		0xAD8B,
		0x9DE8,
		0x8DC9,
		0x7C26,
		0x6C07,
		0x5C64,
		0x4C45,
		0x3CA2,
		0x2C83,
		0x1CE0,
		0x0CC1,
		0xEF1F,
		0xFF3E,
		0xCF5D,
		0xDF7C,
		0xAF9B,
		0xBFBA,
		0x8FD9,
		0x9FF8,
		0x6E17,
		0x7E36,
		0x4E55,
		0x5E74,
		0x2E93,
		0x3EB2,
		0x0ED1,
		0x1EF0,
	};
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; i++)
		crc = (uint16_t)((crc << 8) ^ table[((crc >> 8) ^ data[i]) & 0xFF]);
	return crc;
}

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

	uint16_t crc_calc = crc16_sw((const uint8_t *)frame, expected_len);
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

	crc_val				  = crc16_sw(tx_buf, total_len);
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
