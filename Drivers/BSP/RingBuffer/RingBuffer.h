#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include "main.h"

#define BUFFER_SIZE 2048 // 必须是2的幂，便于优化

// 环形缓冲区数据结构
typedef struct {
	uint8_t data[BUFFER_SIZE];
	volatile uint16_t head; // 读指针
	volatile uint16_t tail; // 写指针
} RingBuffer;

uint8_t RB_IsEmpty(RingBuffer *buf);										  // 判空
uint8_t RB_IsFull(RingBuffer *buf);											  // 判满
uint16_t RB_GetAvailable(RingBuffer *buf);									  // 计算占用空间
uint16_t RB_GetFreeSpace(RingBuffer *buf);									  // 计算剩余空间
uint8_t RB_PutByte(RingBuffer *buf, uint8_t byte);							  // 写入单字节
uint16_t RB_PutByte_Bulk(RingBuffer *buf, const uint8_t *data, uint16_t len); // 写入多字节
uint8_t RB_GetByte(RingBuffer *buf, uint8_t *byte);							  // 读取单字节
uint16_t RB_GetByte_Bulk(RingBuffer *buf, uint8_t *data, uint16_t len);		  // 读取多字节

#endif // !__RINGBUFFER_H__