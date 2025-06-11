#include "GetFrame.h"

extern CRC_HandleTypeDef hcrc;

/**
 * @brief 在环形缓冲区中查找帧头，并给出帧头的偏移
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
		case -1: // 偏移量越界，可能是坏帧或残帧
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
	uint8_t tmp[2] = {0};

	if (!RB_PeekByte(fifo, offset, tmp)) // 验证当前读指针指向帧头数据
		return -1;						 // 缓冲区查询越界

	if (DATA_FRAME_HEAD != tmp[0])
		return 0; // 帧头数据非法

	if (!RB_PeekBlock(fifo, offset + 2, tmp, 2)) // 查询数据帧的数据长度
		return -1;

	*lenth = (uint16_t)((tmp[0] << 8) | tmp[1]);

	if (!RB_PeekByte(fifo, offset + *lenth + 6, tmp)) // 验证当前读指针指向帧尾数据
		return -1;									  // 缓冲区查询越界

	if (DATA_FRAME_TAIL != tmp[0])
		return 0; // 帧尾数据非法

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
 * @brief 从环形缓冲区中解析数据帧
 *
 * @param fifo 环形缓冲区句柄
 * @param frame 数据帧句柄
 * @retval 0 解析失败
 * @retval 1 解析成功
 */
uint8_t GetDataFrame(RingBuffer *fifo, DataFrame *frame)
{
	uint8_t tmp[2] = {0};
	uint16_t len = 0, receive_crc = 0, offset = 0;
	if (1 != SearchFrameHead(fifo, &offset)) // 寻找帧头并拿到帧头在fifo中的偏移，未找到则返回0
		return 0;

	if (1 != CheckFrameIntegrity(fifo, offset, &len)) // 若寻找到了帧头，则验证帧数据是否完整（是否在指定位置存在正确的帧尾数据）
		return 0;

	RB_PeekBlock(fifo, offset + 4, frame->data, len); // 将数据帧的数据部分提取出来
	RB_PeekBlock(fifo, offset + len + 4, tmp, 2);	  // 取出CRC数据
	receive_crc = (uint16_t)(tmp[0] << 8) | tmp[1];
	if (!Data_CRC(frame->data, len, receive_crc)) { // CRC校验失败则清空帧数据并返回0
		// 或可添加重传机制，通过包的序列号标识哪个包需要重传
		for (int i = 0; i < len; i++)
			frame->data[i] = 0;

		RB_SkipBytes(fifo, offset + len + 7); // 移动读指针到坏帧的下一byte，丢弃坏帧

		return 0;
	}

	RB_GetByte_Bulk(fifo, &(frame->frame_head), 1);
	RB_GetByte_Bulk(fifo, &(frame->cmd), 1);
	RB_GetByte_Bulk(fifo, tmp, 2);
	frame->lenth = ((tmp[0] << 8) | tmp[1]);
	RB_GetByte_Bulk(fifo, frame->data, len);
	RB_GetByte_Bulk(fifo, tmp, 2);
	frame->CRC_code = ((tmp[0] << 8) | tmp[1]);
	RB_GetByte_Bulk(fifo, &(frame->frame_tail), 1);
	return 1;
}
