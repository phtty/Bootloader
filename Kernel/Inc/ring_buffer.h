/**
 * @file        ring_buffer.h
 * @brief       环形缓冲区（FIFO）通用实现
 *
 * 单生产者/单消费者模型的环形缓冲区，读写指针均为 volatile，
 * 允许 ISR 与主循环间零锁数据传递。
 * 容量可任意指定，内部使用取模运算回绕索引。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/** @brief 环形缓冲区容量（字节） */
#define RING_BUFFER_SIZE 2048

/** @brief 环形缓冲区结构体 */
typedef struct {
    uint8_t data[RING_BUFFER_SIZE];     /**< 数据存储区 */
    volatile uint16_t read_index;       /**< 读指针（ISR 可能修改） */
    volatile uint16_t write_index;      /**< 写指针（ISR 可能修改） */
} ring_buffer_t;

/** @brief 检查缓冲区是否为空 @retval true 空 @retval false 非空 */
bool rb_is_empty(const ring_buffer_t *rb);

/** @brief 检查缓冲区是否已满 @retval true 满 @retval false 未满 */
bool rb_is_full(const ring_buffer_t *rb);

/**
 * @brief   获取缓冲区中可读字节数
 * @param   rb 环形缓冲区指针
 * @return  可读字节数
 */
uint16_t rb_get_available(const ring_buffer_t *rb);

/**
 * @brief   获取缓冲区剩余可写空间
 * @param   rb 环形缓冲区指针
 * @return  剩余空间（字节）
 */
uint16_t rb_get_free_space(const ring_buffer_t *rb);

/**
 * @brief   写入一个字节
 * @param   rb   环形缓冲区指针
 * @param   byte 要写入的字节
 * @retval  true  写入成功
 * @retval  false 缓冲区已满
 */
bool rb_put_byte(ring_buffer_t *rb, uint8_t byte);

/**
 * @brief   批量写入字节
 * @param   rb   环形缓冲区指针
 * @param   data 数据源指针
 * @param   len  要写入的字节数
 * @return  实际写入的字节数（可能小于 len，缓冲区满时截断）
 */
uint16_t rb_put_bytes(ring_buffer_t *rb, const uint8_t *data, uint16_t len);

/**
 * @brief   读取一个字节
 * @param   rb   环形缓冲区指针
 * @param   byte [out] 读取结果
 * @retval  true  读取成功
 * @retval  false 缓冲区为空
 */
bool rb_get_byte(ring_buffer_t *rb, uint8_t *byte);

/**
 * @brief   批量读取字节
 * @param   rb   环形缓冲区指针
 * @param   data [out] 目标缓冲区
 * @param   len  要读取的字节数
 * @return  实际读取的字节数（可能小于 len，缓冲区空时截断）
 */
uint16_t rb_get_bytes(ring_buffer_t *rb, uint8_t *data, uint16_t len);

/**
 * @brief   窥视一个字节（不移动读指针）
 * @param   rb     环形缓冲区指针
 * @param   offset 从读指针开始的偏移量
 * @param   byte   [out] 读取结果
 * @retval  true  成功
 * @retval  false 偏移超出有效范围
 */
bool rb_peek_byte(const ring_buffer_t *rb, uint16_t offset, uint8_t *byte);

/**
 * @brief   窥视数据块（不移动读指针）
 *
 * 自动处理环形回绕，最多读取 (available - offset) 字节。
 * @param   rb     环形缓冲区指针
 * @param   offset 从读指针开始的偏移量
 * @param   dest   [out] 目标缓冲区
 * @param   len    期望读取字节数
 * @return  实际读取字节数
 */
uint16_t rb_peek_block(const ring_buffer_t *rb, uint16_t offset, uint8_t *dest, uint16_t len);

/**
 * @brief   获取从指定偏移开始的连续可读长度
 *
 * 用于 DMA 接收场景：确定可连续写入的最大长度。
 * @param   rb     环形缓冲区指针
 * @param   offset 从读指针开始的偏移量
 * @return  连续字节数
 */
uint16_t rb_get_contiguous_length(const ring_buffer_t *rb, uint16_t offset);

/**
 * @brief   跳过指定字节数（移动读指针）
 *
 * 通常用于丢弃已处理的数据帧。
 * @param   rb  环形缓冲区指针
 * @param   len 要跳过的字节数
 * @return  实际跳过的字节数（不超过可读数据量）
 */
uint16_t rb_skip_bytes(ring_buffer_t *rb, uint16_t len);
