#include "pl_dma.h"
#include "stm32h7xx_hal.h"
#include "initcall.h"

void pl_dma_init(void)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

subsys_initcall(pl_dma_init);
