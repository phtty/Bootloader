#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 环形缓冲区容量（不再限制为 2 的幂，内部使用取模运算） */
#define RING_BUFFER_SIZE 2048

typedef struct {
    uint8_t data[RING_BUFFER_SIZE];
    volatile uint16_t read_index;
    volatile uint16_t write_index;
} ring_buffer_t;

bool rb_is_empty(const ring_buffer_t *rb);
bool rb_is_full(const ring_buffer_t *rb);
uint16_t rb_get_available(const ring_buffer_t *rb);
uint16_t rb_get_free_space(const ring_buffer_t *rb);
bool rb_put_byte(ring_buffer_t *rb, uint8_t byte);
uint16_t rb_put_bytes(ring_buffer_t *rb, const uint8_t *data, uint16_t len);
bool rb_get_byte(ring_buffer_t *rb, uint8_t *byte);
uint16_t rb_get_bytes(ring_buffer_t *rb, uint8_t *data, uint16_t len);
bool rb_peek_byte(const ring_buffer_t *rb, uint16_t offset, uint8_t *byte);
uint16_t rb_peek_block(const ring_buffer_t *rb, uint16_t offset, uint8_t *dest, uint16_t len);
uint16_t rb_get_contiguous_length(const ring_buffer_t *rb, uint16_t offset);
uint16_t rb_skip_bytes(ring_buffer_t *rb, uint16_t len);
