#include "dev_console.h"
#include "pl_uart.h"
#include "initcall.h"

static dev_console_rx_callback_t g_console_rx_cb;

/* 将 Platform 层 UART RX 回调转发到 Console 层回调 */
static void uart_to_console_cb(const uint8_t *data, size_t len)
{
    if (g_console_rx_cb)
        g_console_rx_cb(data, len);
}

int32_t dev_console_init(void)
{
    pl_uart_set_rx_callback(pl_uart_get_handle(), uart_to_console_cb);
    return 0;
}

int32_t dev_console_putc(char c)
{
    return pl_uart_send(pl_uart_get_handle(), (const uint8_t *)&c, 1, 100);
}

int32_t dev_console_puts(const char *s)
{
    size_t len = 0;
    while (s[len]) len++;
    return pl_uart_send(pl_uart_get_handle(), (const uint8_t *)s, len, 1000);
}

int32_t dev_console_getc(uint32_t timeout_ms)
{
    uint8_t c;
    if (pl_uart_recv(pl_uart_get_handle(), &c, 1, timeout_ms) != 1)
        return -1;
    return (int32_t)c;
}

int32_t dev_console_write(const uint8_t *data, size_t len)
{
    return pl_uart_send(pl_uart_get_handle(), data, len, 1000);
}

int32_t dev_console_read(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    return pl_uart_recv(pl_uart_get_handle(), buf, len, timeout_ms);
}

void dev_console_set_rx_callback(dev_console_rx_callback_t cb)
{
    g_console_rx_cb = cb;
}

int32_t dev_console_start_rx(uint8_t *buf, size_t len)
{
    return pl_uart_start_dma_rx(pl_uart_get_handle(), buf, len);
}

driver_initcall(dev_console_init);
