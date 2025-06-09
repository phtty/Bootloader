#include "RingBuffer.h"

// 检查缓冲区是否为空
uint8_t RB_IsEmpty(RingBuffer *buf)
{
	return buf->head == buf->tail;
}

// 检查缓冲区是否已满
uint8_t RB_IsFull(RingBuffer *buf)
{
	return ((buf->tail + 1) & (BUFFER_SIZE - 1)) == buf->head;
}

// 获取可用数据量
uint16_t RB_GetAvailable(RingBuffer *buf)
{
	return (buf->tail - buf->head) & (BUFFER_SIZE - 1);
}

// 获取剩余空间
uint16_t RB_GetFreeSpace(RingBuffer *buf)
{
	return BUFFER_SIZE - RB_GetAvailable(buf) - 1;
}

// 写入单字节
uint8_t RB_PutByte(RingBuffer *buf, uint8_t byte)
{
	if (RB_IsFull(buf)) {
		return 0; // 缓冲区满，写入失败
	}

	buf->data[buf->tail] = byte;
	buf->tail			 = (buf->tail + 1) & (BUFFER_SIZE - 1);
	return 1; // 写入成功
}

// 写入多个字节
uint16_t RB_PutByte_Bulk(RingBuffer *buf, const uint8_t *data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++) {
		if (!RB_PutByte(buf, data[i])) {
			break; // 缓冲区满，停止写入
		}
	}
	return i; // 返回实际写入的字节数
}

// 读取单字节
uint8_t RB_GetByte(RingBuffer *buf, uint8_t *byte)
{
	if (RB_IsEmpty(buf)) {
		return 0; // 缓冲区空，读取失败
	}

	*byte	  = buf->data[buf->head];
	buf->head = (buf->head + 1) & (BUFFER_SIZE - 1);
	return 1; // 读取成功
}

// 读取多个字节
uint16_t RB_GetByte_Bulk(RingBuffer *buf, uint8_t *data, uint16_t len)
{
	uint16_t i;
	for (i = 0; i < len; i++) {
		if (!RB_GetByte(buf, &data[i])) {
			break; // 缓冲区空，停止读取
		}
	}
	return i; // 返回实际读取的字节数
}
