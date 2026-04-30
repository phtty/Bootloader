#include "pl_crc.h"
#include "stm32h7xx_hal.h"
#include "initcall.h"
#include "main.h"

static CRC_HandleTypeDef hcrc;

/* Feed bytes to CRC unit via 8-bit accesses to DR */
static uint32_t crc_feed(const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        *(volatile uint8_t *)&hcrc.Instance->DR = data[i];
    }
    return hcrc.Instance->DR;
}

void pl_crc_init(void)
{
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    if (HAL_CRC_Init(&hcrc) != HAL_OK)
        Error_Handler();
}

device_initcall(pl_crc_init);

void HAL_CRC_MspInit(CRC_HandleTypeDef *crcHandle)
{
    if (crcHandle->Instance == CRC)
        __HAL_RCC_CRC_CLK_ENABLE();
}

void HAL_CRC_MspDeInit(CRC_HandleTypeDef *crcHandle)
{
    if (crcHandle->Instance == CRC)
        __HAL_RCC_CRC_CLK_DISABLE();
}

uint16_t pl_crc16_calculate(const uint8_t *data, size_t len)
{
    /* 保存当前 CRC32 累加器状态（供固件校验流式使用） */
    uint32_t saved_dr   = CRC->DR;
    uint32_t saved_init = CRC->INIT;
    uint32_t saved_pol  = CRC->POL;
    uint32_t saved_cr   = CRC->CR & ~CRC_CR_RESET;

    /* 切换至 16-bit 模式: CRC-16-CCITT (poly=0x1021, init=0xFFFF) */
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
    hcrc.Init.GeneratingPolynomial = 0x1021;
    hcrc.Init.CRCLength           = CRC_POLYLENGTH_16B;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_DISABLE;
    hcrc.Init.InitValue           = 0xFFFF;
    hcrc.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);

    uint16_t result = (uint16_t)crc_feed(data, len);

    /* 恢复 CRC32 配置与累加器状态 */
    CRC->POL  = saved_pol;
    CRC->INIT = saved_init;
    CRC->CR   = saved_cr;
    CRC->DR   = saved_dr;

    return result;
}

void pl_crc32_reset(void)
{
    /* 确保是 32-bit 默认配置 */
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;
    HAL_CRC_Init(&hcrc);
    __HAL_CRC_DR_RESET(&hcrc);
}

void pl_crc32_feed(const uint8_t *data, size_t len)
{
    crc_feed(data, len);
}

uint32_t pl_crc32_get(void)
{
    return hcrc.Instance->DR;
}
