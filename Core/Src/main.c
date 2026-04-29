/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */

#include "main.h"
#include "initcall.h"
#include "app_bootloader.h"

int main(void)
{
    /* 阶段 1+2: 内核 + 所有外设/设备驱动初始化（通过 initcall 自动发现） */
    board_init();

    /* 阶段 3: 进入 Bootloader 业务逻辑 */
    app_bootloader_run();

    for (;;) {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();
    for (;;) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
    for (;;) {
    }
}
#endif /* USE_FULL_ASSERT */
