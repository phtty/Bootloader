#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef void *pl_uart_handle_t;

typedef struct {
    uint32_t baudrate;
    uint8_t  data_bits;
    uint8_t  stop_bits;
    char     parity;       /* 'N', 'E', 'O' */
} pl_uart_config_t;

typedef void (*pl_uart_rx_callback_t)(const uint8_t *data, size_t len);

void      pl_uart_init(void);
pl_uart_handle_t pl_uart_get_handle(void);
int32_t   pl_uart_send(pl_uart_handle_t handle, const uint8_t *buf, size_t len, uint32_t timeout_ms);
int32_t   pl_uart_recv(pl_uart_handle_t handle, uint8_t *buf, size_t len, uint32_t timeout_ms);
void      pl_uart_set_rx_callback(pl_uart_handle_t handle, pl_uart_rx_callback_t cb);
bool      pl_uart_is_busy(pl_uart_handle_t handle);
void      pl_uart_flush(pl_uart_handle_t handle);
int32_t   pl_uart_start_dma_rx(pl_uart_handle_t handle, uint8_t *buf, size_t len);
