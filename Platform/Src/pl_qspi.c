#include "pl_qspi.h"
#include "stm32h7xx_hal.h"
#include "initcall.h"
#include "main.h"

/* ---- HAL 句柄（static，外部不可见）---- */
static QSPI_HandleTypeDef hqspi;

/* ---- 将平台枚举映射为 HAL 宏 ---- */
static uint32_t map_qspi_mode(pl_qspi_mode_t mode)
{
    switch (mode) {
    case PL_QSPI_MODE_SPI:  return QSPI_DATA_1_LINE;
    case PL_QSPI_MODE_DUAL: return QSPI_DATA_2_LINES;
    case PL_QSPI_MODE_QUAD: return QSPI_DATA_4_LINES;
    default:                return QSPI_DATA_1_LINE;
    }
}

/* 将 pl_qspi_command_t 转换为 HAL 的 QSPI_CommandTypeDef */
static void build_hal_command(QSPI_CommandTypeDef *hal_cmd, pl_qspi_command_t *cmd)
{
    hal_cmd->Instruction        = cmd->instruction;
    hal_cmd->InstructionMode    = map_qspi_mode(cmd->instr_mode);
    hal_cmd->AddressMode        = map_qspi_mode(cmd->addr_mode);
    hal_cmd->DataMode           = map_qspi_mode(cmd->data_mode);
    hal_cmd->AlternateByteMode  = map_qspi_mode(cmd->alt_mode);
    hal_cmd->AddressSize        = cmd->addr_size;
    hal_cmd->DummyCycles        = cmd->dummy_cycles;
    hal_cmd->AlternateBytes     = cmd->alt_bytes;
    hal_cmd->NbData             = 0;
    hal_cmd->DdrMode            = QSPI_DDR_MODE_DISABLE;
    hal_cmd->DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;
    hal_cmd->SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
}

/* ---- Platform 接口实现 ---- */

pl_qspi_handle_t pl_qspi_get_handle(void)
{
    return (pl_qspi_handle_t)&hqspi;
}

int32_t pl_qspi_read(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, uint8_t *buf, size_t len)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    QSPI_CommandTypeDef hal_cmd = {0};
    build_hal_command(&hal_cmd, cmd);
    hal_cmd.NbData = (uint32_t)len;

    /* HAL_QSPI_Command 发送指令+地址+空周期阶段 */
    if (HAL_QSPI_Command(h, &hal_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    /* HAL_QSPI_Receive 接收数据阶段 */
    if (HAL_QSPI_Receive(h, buf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return (int32_t)len;
}

int32_t pl_qspi_write(pl_qspi_handle_t handle, pl_qspi_command_t *cmd, const uint8_t *buf, size_t len)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    QSPI_CommandTypeDef hal_cmd = {0};
    build_hal_command(&hal_cmd, cmd);
    hal_cmd.NbData = (uint32_t)len;

    if (HAL_QSPI_Command(h, &hal_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    if (HAL_QSPI_Transmit(h, (uint8_t *)buf, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;

    return (int32_t)len;
}

int32_t pl_qspi_send_cmd(pl_qspi_handle_t handle, pl_qspi_command_t *cmd)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    QSPI_CommandTypeDef hal_cmd = {0};
    build_hal_command(&hal_cmd, cmd);
    hal_cmd.NbData = 0;
    if (HAL_QSPI_Command(h, &hal_cmd, HAL_QSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
        return -1;
    return 0;
}

int32_t pl_qspi_memory_mapped(pl_qspi_handle_t handle, const pl_qspi_mmap_cfg_t *cfg)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    QSPI_CommandTypeDef hal_cmd = {0};
    QSPI_MemoryMappedTypeDef mem_cfg = {0};

    if (!cfg)
        return -1;

    hal_cmd.Instruction     = cfg->instruction;
    hal_cmd.InstructionMode = map_qspi_mode(cfg->instr_mode);
    hal_cmd.AddressMode     = map_qspi_mode(cfg->addr_mode);
    hal_cmd.DataMode        = map_qspi_mode(cfg->data_mode);
    hal_cmd.AddressSize     = cfg->addr_size;
    hal_cmd.DummyCycles     = cfg->dummy_cycles;
    hal_cmd.DdrMode         = QSPI_DDR_MODE_DISABLE;

    mem_cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;
    if (HAL_QSPI_MemoryMapped(h, &hal_cmd, &mem_cfg) != HAL_OK)
        return -1;
    return 0;
}

int32_t pl_qspi_get_status(pl_qspi_handle_t handle)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    return (int32_t)HAL_QSPI_GetState(h);
}

void pl_qspi_abort(pl_qspi_handle_t handle)
{
    QSPI_HandleTypeDef *h = (QSPI_HandleTypeDef *)handle;
    HAL_QSPI_Abort(h);
}

/* ---- QSPI 初始化（initcall 注册）---- */

void pl_qspi_init(void)
{
    hqspi.Instance                = QUADSPI;
    hqspi.Init.ClockPrescaler     = 0;
    hqspi.Init.FifoThreshold      = 4;
    hqspi.Init.SampleShifting     = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
    hqspi.Init.FlashSize          = 23;    /* 2^23 = 8MB */
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
    hqspi.Init.ClockMode          = QSPI_CLOCK_MODE_0;
    hqspi.Init.FlashID            = QSPI_FLASH_ID_1;
    hqspi.Init.DualFlash          = QSPI_DUALFLASH_DISABLE;
    if (HAL_QSPI_Init(&hqspi) != HAL_OK)
        Error_Handler();
}

device_initcall(pl_qspi_init);

/* ---- HAL MSP 回调（由 HAL_QSPI_Init 内部触发）---- */

void HAL_QSPI_MspInit(QSPI_HandleTypeDef *qspiHandle)
{
    if (qspiHandle->Instance != QUADSPI)
        return;

    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;

    /* PE2  → QSPI_BK1_IO2 (AF9) */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    /* PB2  → QSPI_CLK (AF9) */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PD11 → QSPI_BK1_IO0 (AF9), PD12 → BK1_IO1, PD13 → BK1_IO3 */
    GPIO_InitStruct.Pin       = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* PB6  → QSPI_BK1_NCS (AF10) */
    GPIO_InitStruct.Pin       = GPIO_PIN_6;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef *qspiHandle)
{
    if (qspiHandle->Instance != QUADSPI)
        return;

    __HAL_RCC_QSPI_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOE, GPIO_PIN_2);
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_2 | GPIO_PIN_6);
    HAL_GPIO_DeInit(GPIOD, GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13);
}
