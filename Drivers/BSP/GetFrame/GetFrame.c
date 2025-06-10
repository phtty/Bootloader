#include "GetFrame.h"

extern CRC_HandleTypeDef hcrc;

/**
 * @brief 在环形缓冲区中查找帧头，并移动读指针到帧头
 *
 * @param fifo 环形缓冲区句柄
 * @retval -1 环形缓冲区操作异常
 * @retval 0 环形缓冲区中没有数据
 * @retval 1 找到帧头
 */
int8_t SearchFrameHead(RingBuffer *fifo, uint16_t *offset)
{
	uint8_t crt_data = 0;
	int8_t state	 = 0;
	uint16_t i		 = 0;
	// 遍历当前所有待处理数据
	for (i = 0; i <= RB_GetAvailable(fifo); i++) {
		if (!RB_PeekByte(fifo, i, &crt_data)) {
			state = -1; // 偏移量越界
			break;
		}
		if (DATA_FRAME_HEAD == crt_data) {
			state = 1; // 成功找到帧头
			break;
		}
	}
	switch (state) {
		case -1: // 偏移量越界，坏帧或残帧
			return -1;
			break;
		case 0: // 当前缓冲区没有帧头
			return 0;
			break;
		case 1: // 找到帧头，给出帧头的偏移
			*offset = i;
			return 1;
			break;

		default:
			break;
	}

	return -1;
}

/**
 * @brief 检查数据帧是否完整，并给出数据帧长度
 *
 * @param fifo 环形缓冲区句柄
 * @param lenth 数据部分长度指针
 * @retval -1 环形缓冲区操作异常
 * @retval 0 帧数据异常
 * @retval 1 数据帧完整
 */
int8_t CheckFrameIntegrity(RingBuffer *fifo, uint16_t offset, uint16_t *lenth)
{
	uint8_t tmp			= 0;
	uint16_t data_lenth = 0;

	if (!RB_PeekByte(fifo, offset, &tmp)) // 验证当前读指针指向帧头
		return -1;						  // 缓冲区查询越界

	if (DATA_FRAME_HEAD != tmp)
		return 0; // 帧头数据非法

	if (!RB_PeekByte(fifo, 2, &data_lenth)) // 查询数据帧的数据长度
		return -1;

	if (!RB_PeekByte(fifo, data_lenth + 6, &tmp)) // 验证当前读指针指向帧尾
		return -1;								  // 缓冲区查询越界

	if (DATA_FRAME_TAIL != tmp)
		return 0; // 帧尾数据非法

	lenth = data_lenth;
	return 1; // 数据帧完整，传递数据长度
}

/**
 * @brief 检查接收到的数据CRC校验码是否一致
 *
 * @param data 数据缓存数组
 * @param lenth 数据部分长度
 * @param frame_crc 收到的数据帧校验码
 * @retval 1 CRC校验通过
 * @retval 0 CRC校验未通过
 */
uint8_t Data_CRC(uint8_t *data, uint16_t lenth, uint16_t frame_crc)
{
	return frame_crc == HAL_CRC_Calculate(&hcrc, (uint32_t *)data, lenth);
}

/**
 * @brief 解析数据帧，若残帧或坏帧则不进行下一步解析
 *
 * @param fifo
 * @param data
 * @param frame
 * @return uint8_t
 */
uint8_t GetDataFrame(RingBuffer *fifo, DataFrame *frame)
{
	uint8_t tmp	 = 0;
	uint16_t len = 0, receive_crc = 0, offset = 0;
	if (1 != SearchFrameHead(fifo, &offset))
		return 0;

	if (1 != CheckFrameIntegrity(fifo, offset, len))
		return 0;

	RB_PeekBlock(fifo, 0, frame->data, len);
	RB_PeekByte(fifo, len + 4, &tmp);
	receive_crc = 8 << tmp;
	RB_PeekByte(fifo, len + 5, &tmp);
	receive_crc |= tmp;
	if (!Data_CRC(frame->data, len, receive_crc)) {
		for (int i = 0; i < len; i++)
			frame->data[i] = 0;
		return 0;
	}

	RB_GetByte_Bulk(fifo, &(frame->frame_head), 1);
	RB_GetByte_Bulk(fifo, &(frame->cmd), 1);
	RB_GetByte_Bulk(fifo, &(frame->lenth), 2);
	RB_GetByte_Bulk(fifo, frame->data, len);
	RB_GetByte_Bulk(fifo, &(frame->CRC_code), 2);
	RB_GetByte_Bulk(fifo, &(frame->frame_tail), 1);
	return 1;
}
