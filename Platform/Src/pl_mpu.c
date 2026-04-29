#include "pl_mpu.h"
#include "stm32h7xx_hal.h"

void MPU_Config(void)
{
    MPU_Region_InitTypeDef MPU_InitStruct = {0};

    HAL_MPU_Disable();

    /* Region 0: 背景区域 — 默认拒绝整个 4GB 地址空间
       SubRegionDisable = 0x87 (10000111) 禁用子区域 0,1,2,7 */
    MPU_InitStruct.Enable            = MPU_REGION_ENABLE;
    MPU_InitStruct.Number            = MPU_REGION_NUMBER0;
    MPU_InitStruct.BaseAddress       = 0x0;
    MPU_InitStruct.Size              = MPU_REGION_SIZE_4GB;
    MPU_InitStruct.SubRegionDisable  = 0x87;
    MPU_InitStruct.TypeExtField      = MPU_TEX_LEVEL0;
    MPU_InitStruct.AccessPermission  = MPU_REGION_NO_ACCESS;
    MPU_InitStruct.DisableExec       = MPU_INSTRUCTION_ACCESS_DISABLE;
    MPU_InitStruct.IsShareable       = MPU_ACCESS_NOT_SHAREABLE;
    MPU_InitStruct.IsCacheable       = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable      = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Region 1: QSPI Flash 映射区域 0x90000000, 8MB
       完全访问、可执行代码、可缓存 */
    MPU_InitStruct.Number            = MPU_REGION_NUMBER1;
    MPU_InitStruct.BaseAddress       = 0x90000000;
    MPU_InitStruct.Size              = MPU_REGION_SIZE_8MB;
    MPU_InitStruct.SubRegionDisable  = 0x0;
    MPU_InitStruct.AccessPermission  = MPU_REGION_FULL_ACCESS;
    MPU_InitStruct.DisableExec       = MPU_INSTRUCTION_ACCESS_ENABLE;
    MPU_InitStruct.IsCacheable       = MPU_ACCESS_CACHEABLE;
    MPU_InitStruct.IsBufferable      = MPU_ACCESS_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    /* Region 2: DTCM SRAM 区域 0x20000000, 1MB
       完全访问、禁用缓存（DTCM 是紧耦合内存） */
    MPU_InitStruct.Number            = MPU_REGION_NUMBER2;
    MPU_InitStruct.BaseAddress       = 0x20000000;
    MPU_InitStruct.Size              = MPU_REGION_SIZE_1MB;
    MPU_InitStruct.IsCacheable       = MPU_ACCESS_NOT_CACHEABLE;
    MPU_InitStruct.IsBufferable      = MPU_ACCESS_NOT_BUFFERABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);

    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
