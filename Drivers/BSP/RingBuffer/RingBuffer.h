#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include "main.h"

#define BUFFER_SIZE 256 // 必须是2的幂，便于优化

// 环形缓冲区数据结构
typedef struct {
	uint8_t data[BUFFER_SIZE];
	volatile uint16_t head; // 读指针
	volatile uint16_t tail; // 写指针
} RingBuffer;

inline uint8_t buffer_is_empty(RingBuffer *buf);							  // 判空
inline uint8_t buffer_is_full(RingBuffer *buf);								  // 判满
inline uint16_t buffer_available(RingBuffer *buf);							  // 计算占用空间
inline uint16_t buffer_free_space(RingBuffer *buf);							  // 计算剩余空间
uint8_t buffer_put(RingBuffer *buf, uint8_t byte);							  // 写入单字节
uint16_t buffer_put_bulk(RingBuffer *buf, const uint8_t *data, uint16_t len); // 写入多字节
uint8_t buffer_get(RingBuffer *buf, uint8_t *byte);							  // 读取单字节
uint16_t buffer_get_bulk(RingBuffer *buf, uint8_t *data, uint16_t len);		  // 读取多字节

#endif // !__RINGBUFFER_H__