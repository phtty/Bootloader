#include "pl_rtc.h"
#include "stm32h7xx_hal.h"
#include "initcall.h"
#include "main.h"

static RTC_HandleTypeDef hrtc;

pl_rtc_handle_t pl_rtc_get_handle(void)
{
    return (pl_rtc_handle_t)&hrtc;
}

void pl_rtc_init(void)
{
    hrtc.Instance = RTC;
    hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv = 127;
    hrtc.Init.SynchPrediv = 255;
    hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK)
        Error_Handler();
}

device_initcall(pl_rtc_init);

void HAL_RTC_MspInit(RTC_HandleTypeDef *rtcHandle)
{
    if (rtcHandle->Instance != RTC)
        return;

    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
        Error_Handler();

    __HAL_RCC_RTC_ENABLE();
}

void HAL_RTC_MspDeInit(RTC_HandleTypeDef *rtcHandle)
{
    if (rtcHandle->Instance != RTC)
        return;
    __HAL_RCC_RTC_DISABLE();
}
