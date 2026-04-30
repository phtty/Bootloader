#include "pl_uart.h"
#include "stm32h7xx_hal.h"
#include "initcall.h"
#include "main.h"

/* ---- HAL 句柄（static，外部不可见）---- */
static UART_HandleTypeDef huart1;
static DMA_HandleTypeDef hdma_usart1_rx;

/* ---- 回调函数指针 ---- */
static pl_uart_rx_callback_t g_uart_rx_cb;

/* ---- Platform 接口实现 ---- */

pl_uart_handle_t pl_uart_get_handle(void)
{
	return (pl_uart_handle_t)&huart1;
}

int32_t pl_uart_send(pl_uart_handle_t handle, const uint8_t *buf, size_t len, uint32_t timeout_ms)
{
	UART_HandleTypeDef *h = (UART_HandleTypeDef *)handle;
	if (HAL_UART_Transmit(h, (uint8_t *)buf, len, timeout_ms) != HAL_OK)
		return -1;
	return (int32_t)len;
}

int32_t pl_uart_recv(pl_uart_handle_t handle, uint8_t *buf, size_t len, uint32_t timeout_ms)
{
	UART_HandleTypeDef *h = (UART_HandleTypeDef *)handle;
	if (HAL_UART_Receive(h, buf, len, timeout_ms) != HAL_OK)
		return -1;
	return (int32_t)len;
}

void pl_uart_set_rx_callback(pl_uart_handle_t handle, pl_uart_rx_callback_t cb)
{
	(void)handle;
	g_uart_rx_cb = cb;
}

bool pl_uart_is_busy(pl_uart_handle_t handle)
{
	UART_HandleTypeDef *h = (UART_HandleTypeDef *)handle;
	return (h->gState != HAL_UART_STATE_READY);
}

void pl_uart_flush(pl_uart_handle_t handle)
{
	UART_HandleTypeDef *h = (UART_HandleTypeDef *)handle;
	__HAL_UART_FLUSH_DRREGISTER(h);
}

/* ---- 上层可调用此函数启动 DMA 空闲中断接收（典型 bootloader 用法）---- */
int32_t pl_uart_start_dma_rx(pl_uart_handle_t handle, uint8_t *buf, size_t len)
{
	UART_HandleTypeDef *h = (UART_HandleTypeDef *)handle;
	if (HAL_UARTEx_ReceiveToIdle_DMA(h, buf, len) != HAL_OK)
		return -1;
	return 0;
}

/* ---- UART 初始化（initcall 注册）---- */

void pl_uart_init(void)
{
	huart1.Instance					   = USART1;
	huart1.Init.BaudRate			   = 115200;
	huart1.Init.WordLength			   = UART_WORDLENGTH_8B;
	huart1.Init.StopBits			   = UART_STOPBITS_1;
	huart1.Init.Parity				   = UART_PARITY_NONE;
	huart1.Init.Mode				   = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl			   = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling		   = UART_OVERSAMPLING_16;
	huart1.Init.OneBitSampling		   = UART_ONE_BIT_SAMPLE_DISABLE;
	huart1.Init.ClockPrescaler		   = UART_PRESCALER_DIV1;
	huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	if (HAL_UART_Init(&huart1) != HAL_OK)
		Error_Handler();

	/* 配置 FIFO 阈值后禁用 FIFO 模式 */
	if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
		Error_Handler();
	if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
		Error_Handler();
	if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
		Error_Handler();
}

device_initcall(pl_uart_init);

/* ---- HAL MSP 回调（由 HAL_UART_Init 内部触发）---- */

void HAL_UART_MspInit(UART_HandleTypeDef *uartHandle)
{
	if (uartHandle->Instance != USART1)
		return;

	__HAL_RCC_USART1_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* PB14 = USART1_TX, PB15 = USART1_RX */
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	GPIO_InitStruct.Pin				 = GPIO_PIN_14 | GPIO_PIN_15;
	GPIO_InitStruct.Mode			 = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull			 = GPIO_NOPULL;
	GPIO_InitStruct.Speed			 = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate		 = GPIO_AF4_USART1;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* DMA1_Stream0 = USART1_RX */
	hdma_usart1_rx.Instance					= DMA1_Stream0;
	hdma_usart1_rx.Init.Request				= DMA_REQUEST_USART1_RX;
	hdma_usart1_rx.Init.Direction			= DMA_PERIPH_TO_MEMORY;
	hdma_usart1_rx.Init.PeriphInc			= DMA_PINC_DISABLE;
	hdma_usart1_rx.Init.MemInc				= DMA_MINC_ENABLE;
	hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
	hdma_usart1_rx.Init.MemDataAlignment	= DMA_MDATAALIGN_BYTE;
	hdma_usart1_rx.Init.Mode				= DMA_NORMAL;
	hdma_usart1_rx.Init.Priority			= DMA_PRIORITY_LOW;
	hdma_usart1_rx.Init.FIFOMode			= DMA_FIFOMODE_DISABLE;
	if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
		Error_Handler();

	__HAL_LINKDMA(uartHandle, hdmarx, hdma_usart1_rx);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *uartHandle)
{
	if (uartHandle->Instance != USART1)
		return;

	__HAL_RCC_USART1_CLK_DISABLE();
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_14 | GPIO_PIN_15);
	HAL_DMA_DeInit(uartHandle->hdmarx);
}

/* ---- HAL DMA 接收完成回调 → 转发到上层 callback ---- */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size)
{
	if (huart->Instance == USART1 && g_uart_rx_cb) {
		g_uart_rx_cb((const uint8_t *)huart->pRxBuffPtr, size);
	}
}

/* ---- ISR: DMA1 Stream0 全局中断 ---- */
void DMA1_Stream0_IRQHandler(void)
{
	HAL_DMA_IRQHandler(&hdma_usart1_rx);
}
