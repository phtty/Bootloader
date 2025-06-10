#ifndef __GET_FRAME__
#define __GET_FRAME__

#include "main.h"
#include "RingBuffer.h"

#define DATA_FRAME_HEAD 0x55
#define DATA_FRAME_TAIL 0xaa

typedef struct
{
	uint8_t frame_head;
	uint8_t cmd;
	uint16_t lenth;
	uint8_t data[1024];
	uint16_t CRC_code;
	uint8_t frame_tail;
} DataFrame;

int8_t SearchFrameHead(RingBuffer *fifo, uint16_t *offset);
int8_t CheckFrameIntegrity(RingBuffer *fifo, uint16_t offset, uint16_t *lenth);
uint8_t Data_CRC(uint8_t *data, uint16_t lenth, uint16_t frame_crc);
uint8_t GetDataFrame(RingBuffer *fifo, DataFrame *frame);

#endif // !__GET_FRAME__
