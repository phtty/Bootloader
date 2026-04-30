#include "dev_qspi_flash.h"
#include "pl_qspi.h"
#include "initcall.h"
#include <string.h>

/* ---- QSPI Flash 通用命令定义 ---- */
#define QSPI_CMD_READ_STATUS1     0x05
#define QSPI_CMD_READ_STATUS2     0x35
#define QSPI_CMD_WRITE_ENABLE     0x06
#define QSPI_CMD_WRITE_DISABLE    0x04
#define QSPI_CMD_READ_ID          0x9F
#define QSPI_CMD_READ_DATA        0x03
#define QSPI_CMD_PAGE_PROGRAM     0x02
#define QSPI_CMD_SECTOR_ERASE     0x20
#define QSPI_CMD_CHIP_ERASE       0xC7
#define QSPI_CMD_RESET_ENABLE     0x66
#define QSPI_CMD_RESET_DEVICE     0x99
#define QSPI_CMD_WRITE_STATUS2    0x31  /* 写 Status Register 2（W25Q 系列） */
#define QSPI_CMD_READ_DATA_FAST   0x0B  /* Fast Read, 1-1-1, ~133MHz */
#define QSPI_CMD_QUAD_IO_READ     0xEB  /* Quad I/O Fast Read, 1-4-4, ~133MHz */
#define QSPI_SECTOR_SIZE          4096

/* Status Register 2 中的 QE (Quad Enable) 位 */
#define QSPI_SR2_QE_BIT           (1 << 1)  /* W25Q/IS25/GD25 标准: S9 = QE */

/* ---- 设备实例 ---- */
typedef struct {
    dev_flash_ops_t   *ops;
    pl_qspi_handle_t   pl_handle;
    dev_flash_info_t   info;
    pl_qspi_mmap_cfg_t mmap_cfg;
    bool               initialized;
} qspi_flash_dev_t;

static qspi_flash_dev_t g_flash_dev;

/* 等待 Flash 内部操作完成（轮询 WIP 位） */
static int32_t flash_wait_busy(qspi_flash_dev_t *dev)
{
    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_READ_STATUS1,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .data_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 0,
        .dummy_cycles = 0,
    };
    uint8_t status;
    uint32_t timeout = 100000;

    do {
        if (pl_qspi_read(dev->pl_handle, &cmd, &status, 1) != 1)
            return -1;
        if (!(status & 0x01))  /* WIP bit cleared */
            return 0;
    } while (--timeout);

    return -1;  /* 超时 */
}

/* 写使能 */
static int32_t flash_write_enable(qspi_flash_dev_t *dev)
{
    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_WRITE_ENABLE,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .addr_size    = 0,
        .dummy_cycles = 0,
    };
    return pl_qspi_send_cmd(dev->pl_handle, &cmd);
}

/* ---- Flash 操作接口实现 ---- */

/* 尝试使能 QE (Quad Enable) 位，成功返回 true */
static bool flash_enable_quad(qspi_flash_dev_t *dev)
{
    pl_qspi_command_t cmd;
    uint8_t sr1, sr2;

    /* 读取 SR2，检查 QE 位 */
    cmd.instruction  = QSPI_CMD_READ_STATUS2;
    cmd.instr_mode   = PL_QSPI_MODE_SPI;
    cmd.data_mode    = PL_QSPI_MODE_SPI;
    cmd.addr_size    = 0;
    cmd.dummy_cycles = 0;
    if (pl_qspi_read(dev->pl_handle, &cmd, &sr2, 1) != 1)
        return false;

    if (sr2 & QSPI_SR2_QE_BIT)
        return true;  /* QE 已经使能 */

    /* 读 SR1（需要保留其原始值） */
    cmd.instruction  = QSPI_CMD_READ_STATUS1;
    if (pl_qspi_read(dev->pl_handle, &cmd, &sr1, 1) != 1)
        return false;

    /* 写使能 */
    cmd.instruction  = QSPI_CMD_WRITE_ENABLE;
    if (pl_qspi_send_cmd(dev->pl_handle, &cmd) != 0)
        return false;

    /* 写 SR1 + SR2（0x01 命令 + 2 字节，兼容 W25Q/IS25/GD25/MX25 系列） */
    uint8_t sr_buf[2] = { sr1, sr2 | QSPI_SR2_QE_BIT };
    pl_qspi_command_t wr_cmd = {
        .instruction  = 0x01,  /* Write Status Register (SR1 + SR2) */
        .instr_mode   = PL_QSPI_MODE_SPI,
        .data_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 0,
        .dummy_cycles = 0,
    };
    if (pl_qspi_write(dev->pl_handle, &wr_cmd, sr_buf, 2) != 2)
        return false;

    /* 等待写入完成 */
    flash_wait_busy(dev);

    /* 验证 QE 是否已置位 */
    cmd.instruction  = QSPI_CMD_READ_STATUS2;
    if (pl_qspi_read(dev->pl_handle, &cmd, &sr2, 1) != 1)
        return false;

    return (sr2 & QSPI_SR2_QE_BIT) != 0;
}

static int32_t qspi_flash_init(void *device)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;

    /* 读取 JEDEC ID */
    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_READ_ID,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .data_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 0,
        .dummy_cycles = 0,
    };
    uint8_t jedec_id[3];
    if (pl_qspi_read(dev->pl_handle, &cmd, jedec_id, 3) != 3)
        return -1;

    dev->info.manufacturer_id = jedec_id[0];
    dev->info.device_id       = ((uint16_t)jedec_id[1] << 8) | jedec_id[2];

    /*
     * 配置 memory-mapped 模式的读取命令。
     *
     * 优先尝试 Quad I/O Fast Read (0xEB, 1-4-4, 4 dummy cycles)，
     * 需要 Flash 芯片的 QE 位已使能。若 QE 使能失败，
     * 退回到 Fast Read (0x0B, 1-1-1, 8 dummy cycles)，
     * 此模式在 100MHz 下仍可正常工作。
     */
    bool quad_ok = flash_enable_quad(dev);

    if (quad_ok) {
        dev->mmap_cfg.instruction  = QSPI_CMD_QUAD_IO_READ;   /* 0xEB */
        dev->mmap_cfg.instr_mode   = PL_QSPI_MODE_SPI;         /* 1 线指令 */
        dev->mmap_cfg.addr_mode    = PL_QSPI_MODE_QUAD;        /* 4 线地址 */
        dev->mmap_cfg.data_mode    = PL_QSPI_MODE_QUAD;        /* 4 线数据 */
        dev->mmap_cfg.addr_size    = 3;                         /* 24-bit 地址 */
        dev->mmap_cfg.dummy_cycles = 4;                         /* 100MHz 需要 4 个空周期 */
    } else {
        dev->mmap_cfg.instruction  = QSPI_CMD_READ_DATA_FAST;  /* 0x0B */
        dev->mmap_cfg.instr_mode   = PL_QSPI_MODE_SPI;
        dev->mmap_cfg.addr_mode    = PL_QSPI_MODE_SPI;
        dev->mmap_cfg.data_mode    = PL_QSPI_MODE_SPI;
        dev->mmap_cfg.addr_size    = 3;
        dev->mmap_cfg.dummy_cycles = 8;                         /* 100MHz 需要 8 个空周期 */
    }

    dev->initialized = true;
    return 0;
}

static int32_t qspi_flash_read(void *device, uint32_t addr, uint8_t *buf, size_t len)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;

    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_READ_DATA,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .addr_mode    = PL_QSPI_MODE_SPI,
        .data_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 3,
        .dummy_cycles = 0,
    };
    return pl_qspi_read(dev->pl_handle, &cmd, buf, len);
}

static int32_t qspi_flash_write(void *device, uint32_t addr, const uint8_t *buf, size_t len)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;

    if (flash_write_enable(dev) != 0)
        return -1;

    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_PAGE_PROGRAM,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .addr_mode    = PL_QSPI_MODE_SPI,
        .data_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 3,
        .dummy_cycles = 0,
    };
    int32_t ret = pl_qspi_write(dev->pl_handle, &cmd, buf, len);
    if (ret < 0)
        return -1;

    return flash_wait_busy(dev);
}

static int32_t qspi_flash_erase_sector(void *device, uint32_t addr)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;

    if (flash_write_enable(dev) != 0)
        return -1;

    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_SECTOR_ERASE,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .addr_mode    = PL_QSPI_MODE_SPI,
        .addr_size    = 3,
        .dummy_cycles = 0,
    };
    if (pl_qspi_send_cmd(dev->pl_handle, &cmd) != 0)
        return -1;

    return flash_wait_busy(dev);
}

static int32_t qspi_flash_erase_chip(void *device)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;

    if (flash_write_enable(dev) != 0)
        return -1;

    pl_qspi_command_t cmd = {
        .instruction  = QSPI_CMD_CHIP_ERASE,
        .instr_mode   = PL_QSPI_MODE_SPI,
        .addr_size    = 0,
        .dummy_cycles = 0,
    };
    if (pl_qspi_send_cmd(dev->pl_handle, &cmd) != 0)
        return -1;

    return flash_wait_busy(dev);
}

static int32_t qspi_flash_get_info(void *device, dev_flash_info_t *info)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;
    memcpy(info, &dev->info, sizeof(dev_flash_info_t));
    return 0;
}

static int32_t qspi_flash_memory_mapped(void *device)
{
    qspi_flash_dev_t *dev = (qspi_flash_dev_t *)device;
    return pl_qspi_memory_mapped(dev->pl_handle, &dev->mmap_cfg);
}

static int32_t qspi_flash_deinit(void *device)
{
    (void)device;
    return 0;
}

static dev_flash_ops_t g_flash_ops = {
    .init          = qspi_flash_init,
    .read          = qspi_flash_read,
    .write         = qspi_flash_write,
    .erase_sector  = qspi_flash_erase_sector,
    .erase_chip    = qspi_flash_erase_chip,
    .get_info      = qspi_flash_get_info,
    .memory_mapped = qspi_flash_memory_mapped,
    .deinit        = qspi_flash_deinit,
};

/* ---- 公共接口 ---- */

void dev_flash_register(dev_flash_ops_t *ops, void *platform_handle)
{
    g_flash_dev.ops       = ops;
    g_flash_dev.pl_handle = (pl_qspi_handle_t)platform_handle;
    g_flash_dev.initialized = false;
}

void *dev_flash_get_default(void)
{
    return &g_flash_dev;
}

int32_t dev_flash_read(uint32_t addr, uint8_t *buf, size_t len)
{
    return g_flash_dev.ops->read(&g_flash_dev, addr, buf, len);
}

int32_t dev_flash_write(uint32_t addr, const uint8_t *buf, size_t len)
{
    return g_flash_dev.ops->write(&g_flash_dev, addr, buf, len);
}

int32_t dev_flash_erase_sector(uint32_t addr)
{
    return g_flash_dev.ops->erase_sector(&g_flash_dev, addr);
}

int32_t dev_flash_erase_chip(void)
{
    return g_flash_dev.ops->erase_chip(&g_flash_dev);
}

int32_t dev_flash_get_info(dev_flash_info_t *info)
{
    return g_flash_dev.ops->get_info(&g_flash_dev, info);
}

int32_t dev_flash_memory_mapped(void)
{
    return g_flash_dev.ops->memory_mapped(&g_flash_dev);
}

/* ---- initcall 注册 ---- */

void dev_qspi_flash_init(void)
{
    dev_flash_register(&g_flash_ops, pl_qspi_get_handle());
    if (g_flash_dev.ops->init(&g_flash_dev) != 0)
        return;
}

driver_initcall(dev_qspi_flash_init);
