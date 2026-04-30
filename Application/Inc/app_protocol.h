/**
 * @file        app_protocol.h
 * @brief       Bootloader 通信协议定义与帧收发 API
 *
 * 协议帧格式（大端序）：
 * ```
 *   SOF    CMD    LEN(2B)    Payload(0..256B)    CRC16(2B)
 *   0xAA   1B     big-endian                    big-endian
 * ```
 *
 * CRC-16-CCITT (poly=0x1021, init=0xFFFF)，由硬件 CRC 外设计算。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/* ---- 协议常量 ---- */
#define PROTO_SOF          0xAAu   /**< 帧起始标志 */
#define PROTO_MAX_PAYLOAD  256u    /**< 单个帧最大载荷（字节） */
#define PROTO_HEADER_SIZE  4u      /**< SOF + CMD + Len(2B) = 4 字节 */
#define PROTO_CRC_SIZE     2u      /**< CRC16 占用 2 字节 */
#define PROTO_MAX_FRAME    (PROTO_HEADER_SIZE + PROTO_MAX_PAYLOAD + PROTO_CRC_SIZE)

/* ---- 命令码 ---- */
typedef enum {
    PROTO_CMD_SYNC    = 0x01,  /**< 握手 / 查询设备信息 */
    PROTO_CMD_ERASE   = 0x02,  /**< 擦除指定扇区 */
    PROTO_CMD_WRITE   = 0x03,  /**< 写入固件数据块 */
    PROTO_CMD_VERIFY  = 0x04,  /**< 校验已写入的数据 */
    PROTO_CMD_LAUNCH  = 0x05,  /**< 校验通过后启动固件 */
    PROTO_CMD_RESET   = 0x06,  /**< 复位设备 */
    PROTO_CMD_INFO    = 0x07,  /**< 查询设备信息 */
    PROTO_ACK_OK      = 0x80,  /**< 操作成功 */
    PROTO_ACK_ERROR   = 0x81,  /**< 操作失败 */
    PROTO_ACK_BUSY    = 0x82,  /**< 设备忙（正在处理上次请求） */
    PROTO_ACK_CSUMERR = 0x83,  /**< 校验错误 */
    PROTO_ACK_INVALID = 0x84,  /**< 无效命令 / 参数 */
} proto_cmd_t;

/* ---- 帧结构（packed，禁止编译器填充）---- */
typedef struct __attribute__((packed)) {
    uint8_t  sof;                          /**< 帧起始标志，固定 0xAA */
    uint8_t  cmd;                          /**< 命令码 / ACK 码 */
    uint16_t payload_len;                  /**< 载荷长度（大端序） */
    uint8_t  payload[PROTO_MAX_PAYLOAD];   /**< 载荷数据 */
    uint16_t crc16;                        /**< CRC16 校验值（大端序） */
} proto_frame_t;

/** @brief 固件数据块描述（WRITE 命令的载荷格式） */
typedef struct __attribute__((packed)) {
    uint32_t flash_addr;     /**< 目标写入地址（QSPI Flash 偏移） */
    uint16_t data_len;       /**< 数据长度 */
    uint8_t  data[0];        /**< 柔性数组：实际数据紧跟其后 */
} proto_write_payload_t;

/* ---- API ---- */

/**
 * @brief   协议栈初始化
 *
 * 注册 DMA 接收回调并启动首次 DMA 空闲中断接收。
 */
void app_protocol_init(void);

/**
 * @brief   等待并接收一帧数据（阻塞）
 * @param   frame      [out] 接收到的帧
 * @param   timeout_ms 超时时间（毫秒，0 表示无限等待）
 * @retval  0  成功
 * @retval -1  超时
 * @retval -2  帧格式错误（SOF/长度/CRC 不匹配）
 */
int32_t proto_recv_frame(proto_frame_t *frame, uint32_t timeout_ms);

/**
 * @brief   发送 ACK 帧
 * @param   cmd      对应的命令码
 * @param   ack_code ACK 状态码
 * @param   data     附加数据（可为 NULL）
 * @param   len      附加数据长度
 * @return  成功返回发送字节数，失败返回 -1
 */
int32_t proto_send_ack(uint8_t cmd, uint8_t ack_code, const uint8_t *data, uint16_t len);

/**
 * @brief   Bootloader 协议状态机（帧分发）
 *
 * 根据帧命令码分发到对应处理逻辑：
 * - SYNC   → 返回 Flash 设备信息
 * - ERASE  → 擦除指定扇区
 * - WRITE  → 写入固件数据块
 * - VERIFY → 校验写入完整性
 * - LAUNCH → 跳转到应用程序
 *
 * @param   frame 接收到的帧
 * @retval  0 成功
 * @retval -1 处理失败
 */
int32_t proto_handle_frame(proto_frame_t *frame);
