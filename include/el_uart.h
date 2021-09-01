#ifndef _EL_UART_H_
#define _EL_UART_H_

#include <espressif/esp_common.h>
#include "FreeRTOS.h"
#include "queue.h"

#include "c_types.h"

#define UART0 0

extern QueueHandle_t  uart_rx_queue;
extern volatile uint16_t uart_rx_overruns;
extern volatile uint16_t uart_rx_bytes;

ICACHE_FLASH_ATTR int uart_getchar_ms(int timeout);
#define uart_getchar() uart_getchar_ms(-1)
#define uart_rx_flush() xQueueReset(uart_rx_queue)
ICACHE_FLASH_ATTR int uart_rx_available(void);
ICACHE_FLASH_ATTR void uart_rx_init(void);
ICACHE_FLASH_ATTR void uart_set_baud(int uart, int baud);

#endif
