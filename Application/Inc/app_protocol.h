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

/* ---- 帧结构（Packed，禁止编译器填充）---- */
typedef struct __attribute__((packed)) {
    uint8_t  sof;                          /**< 帧起始标志，固定 0xAA */
    uint8_t  cmd;                          /**< 命令码 / ACK 码 */
    uint16_t payload_len;                  /**< 载荷长度（大端序） */
    uint8_t  payload[PROTO_MAX_PAYLOAD];   /**< 载荷数据 */
    uint16_t crc16;                        /**< CRC16 校验值（大端序） */
} proto_frame_t;

/* ---- 固件数据块描述 ---- */
typedef struct __attribute__((packed)) {
    uint32_t flash_addr;     /**< 目标写入地址（QSPI Flash 偏移） */
    uint16_t data_len;       /**< 数据长度 */
    uint8_t  data[0];        /**< 柔性数组：实际数据紧跟其后 */
} proto_write_payload_t;

/* ---- API ---- */

/**
 * @brief 协议栈初始化
 */
void app_protocol_init(void);

/**
 * @brief 等待并接收一帧数据（阻塞）
 * @param frame  [out] 接收到的帧
 * @param timeout_ms  超时时间（毫秒）
 * @return 成功返回 0，超时返回 -1，格式错误返回 -2
 */
int32_t proto_recv_frame(proto_frame_t *frame, uint32_t timeout_ms);

/**
 * @brief 发送 ACK 帧
 * @param cmd      对应的命令码
 * @param ack_code ACK 状态码
 * @param data     附加数据（可为 NULL）
 * @param len      附加数据长度
 * @return 成功返回 0，失败返回 -1
 */
int32_t proto_send_ack(uint8_t cmd, uint8_t ack_code, const uint8_t *data, uint16_t len);

/**
 * @brief 处理接收到的帧（Bootloader 协议状态机核心）
 * @param frame  接收到的帧
 * @return 成功返回 0，失败返回 -1
 */
int32_t proto_handle_frame(proto_frame_t *frame);
