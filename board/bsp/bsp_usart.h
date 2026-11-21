#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdbool.h>
#include "stm32f7xx_hal.h"

HAL_StatusTypeDef bsp_usart_init(void);

/* Use from one FreeRTOS task. Reading blocks until data arrives.
 * A false result reports lost input. Buffered bytes are discarded in that case. */
bool bsp_usart_read(uint8_t *byte);
HAL_StatusTypeDef bsp_usart_write(const uint8_t *data, uint16_t length);

#endif
