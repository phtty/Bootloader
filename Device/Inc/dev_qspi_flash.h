/**
 * @file        dev_qspi_flash.h
 * @brief       QSPI Flash 设备驱动抽象层
 *
 * 提供类 Linux file_operations 的 ops 表驱动框架，
 * 上层通过 dev_flash_read/write/erase 等便捷函数操作 Flash，
 * 无需关心芯片型号差异。
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Flash 设备信息结构体
 */
typedef struct {
    uint32_t  total_size_mb;    /**< Flash 总容量（MB） */
    uint32_t  sector_size_kb;   /**< 最小擦除单元大小（KB） */
    uint32_t  page_size_bytes;  /**< 编程页大小（字节） */
    uint8_t   manufacturer_id;  /**< JEDEC 制造商 ID */
    uint16_t  device_id;        /**< JEDEC 设备 ID */
    char      name[16];         /**< 芯片型号名称 */
} dev_flash_info_t;

/**
 * @brief Flash 设备操作接口（类似 Linux file_operations）
 *
 * 每种 Flash 芯片（W25Q64、IS25LP128 等）实现自己的 ops，
 * 通过 dev_flash_register() 注册到框架中。
 * 应用层通过便捷函数 dev_flash_read/write/erase 调用，无需关心芯片型号。
 */
typedef struct dev_flash_ops {
    /** @brief 初始化芯片：读取 JEDEC ID、配置寄存器 */
    int32_t (*init)(void *dev);

    /** @brief 从 Flash 指定地址读取数据
     *  @param dev   Flash 设备句柄
     *  @param addr  Flash 内部地址（0-based）
     *  @param buf   输出缓冲区
     *  @param len   读取字节数
     *  @return 成功返回实际读取字节数，失败返回 -1
     */
    int32_t (*read)(void *dev, uint32_t addr, uint8_t *buf, size_t len);

    /** @brief 向 Flash 指定地址写入数据（需先擦除目标扇区）
     *  @param dev   Flash 设备句柄
     *  @param addr  Flash 内部地址（0-based）
     *  @param buf   数据缓冲区
     *  @param len   写入字节数
     *  @return 成功返回实际写入字节数，失败返回 -1
     *  @note   自动处理写使能和忙等待，单次写入不可跨页
     */
    int32_t (*write)(void *dev, uint32_t addr, const uint8_t *buf, size_t len);

    /** @brief 擦除指定地址所在的扇区（典型值 4KB）
     *  @param dev   Flash 设备句柄
     *  @param addr  扇区内任意地址
     *  @return 成功返回 0，失败返回 -1
     *  @note   擦除后扇区全部变为 0xFF，操作耗时约 50-400ms
     */
    int32_t (*erase_sector)(void *dev, uint32_t addr);

    /** @brief 整片擦除
     *  @param dev   Flash 设备句柄
     *  @return 成功返回 0，失败返回 -1
     *  @note   操作耗时较长（几十秒量级），Bootloader 场景慎用
     */
    int32_t (*erase_chip)(void *dev);

    /** @brief 获取 Flash 芯片信息
     *  @param dev   Flash 设备句柄
     *  @param info  [out] 设备信息结构体指针
     *  @return 成功返回 0，失败返回 -1
     */
    int32_t (*get_info)(void *dev, dev_flash_info_t *info);

    /** @brief 将 Flash 配置为内存映射模式
     *
     *  内存映射后，Flash 内容可通过指针直接访问（0x90000000），
     *  常用于执行存储在外部 Flash 中的固件代码。
     *  @param dev   Flash 设备句柄
     *  @return 成功返回 0，失败返回 -1
     */
    int32_t (*memory_mapped)(void *dev);

    /** @brief 反初始化，释放资源
     *  @param dev   Flash 设备句柄
     *  @return 成功返回 0，失败返回 -1
     */
    int32_t (*deinit)(void *dev);
} dev_flash_ops_t;

/* ================================================================
 *  以下为 Application 层可直接调用的公共接口
 * ================================================================ */

/**
 * @brief 注册 Flash 设备（由具体芯片驱动模块调用，应用层无需关心）
 * @param ops             设备操作接口指针
 * @param platform_handle 平台层 QSPI 句柄
 */
void dev_flash_register(dev_flash_ops_t *ops, void *platform_handle);

/** @brief 获取默认 Flash 设备句柄
 *  @return 设备句柄指针 */
void *dev_flash_get_default(void);

/** @brief 从 Flash 读取数据 */
int32_t dev_flash_read(uint32_t addr, uint8_t *buf, size_t len);

/** @brief 向 Flash 写入数据 */
int32_t dev_flash_write(uint32_t addr, const uint8_t *buf, size_t len);

/** @brief 擦除指定扇区 */
int32_t dev_flash_erase_sector(uint32_t addr);

/** @brief 整片擦除 */
int32_t dev_flash_erase_chip(void);

/** @brief 获取 Flash 芯片信息 */
int32_t dev_flash_get_info(dev_flash_info_t *info);

/** @brief 将 Flash 映射到内存地址空间 */
int32_t dev_flash_memory_mapped(void);

/* 设备驱动初始化入口（driver_initcall 自动调用，应用层无需调用） */
void dev_qspi_flash_init(void);
