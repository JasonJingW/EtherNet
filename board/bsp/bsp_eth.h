#ifndef BSP_ETH_H
#define BSP_ETH_H

#include "stm32f7xx_hal.h"

#define BSP_ETH_MAC_SIZE (6U)
#define BSP_ETH_FRAME_SIZE (1514U)

/* Call from one task. The RX frame remains valid until the next receive call. */
HAL_StatusTypeDef bsp_eth_init(uint8_t mac[BSP_ETH_MAC_SIZE]);
uint32_t bsp_eth_poll_link(void); /* Current speed in Mbit/s, or zero if down. */
const uint8_t *bsp_eth_receive(uint16_t *length);
HAL_StatusTypeDef bsp_eth_send(const uint8_t *frame, uint16_t length);

#endif
