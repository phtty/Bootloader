#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include "main.h"
#include <memory.h>

#define BUFFER_SIZE 2048 // 必须是2的幂，便于优化

// 环形缓冲区数据结构
typedef struct {
	uint8_t data[BUFFER_SIZE];
	volatile uint16_t read_index;  // 读指针
	volatile uint16_t write_index; // 写指针
} RingBuffer;

uint8_t RB_IsEmpty(RingBuffer *fifo);												   // 判空
uint8_t RB_IsFull(RingBuffer *fifo);												   // 判满
uint16_t RB_GetAvailable(RingBuffer *fifo);											   // 计算占用空间
uint16_t RB_GetFreeSpace(RingBuffer *fifo);											   // 计算剩余空间
uint8_t RB_PutByte(RingBuffer *fifo, uint8_t byte);									   // 写入单字节
uint16_t RB_PutByte_Bulk(RingBuffer *fifo, const uint8_t *data, uint16_t len);		   // 写入多字节
uint8_t RB_GetByte(RingBuffer *fifo, uint8_t *byte);								   // 读取单字节
uint16_t RB_GetByte_Bulk(RingBuffer *fifo, uint8_t *data, uint16_t len);			   // 读取多字节
uint8_t RB_PeekByte(RingBuffer *fifo, uint16_t offset, uint8_t *byte);				   // 窥视单个字节（不移动读指针）
uint16_t RB_PeekBlock(RingBuffer *fifo, uint16_t offset, uint8_t *dest, uint16_t len); // 窥视数据块（不移动读指针）
uint16_t RB_GetContiguousLength(RingBuffer *fifo, uint16_t offset);					   // 获取从指定偏移开始的连续数据长度
uint16_t RB_SkipBytes(RingBuffer *fifo, uint16_t len);								   // 跳过指定字节数（移动读指针）

#endif // !__RINGBUFFER_H__