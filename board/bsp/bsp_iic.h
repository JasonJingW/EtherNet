#ifndef BSP_IIC_H
#define BSP_IIC_H

#include "stm32f7xx_hal.h"

/* Initialize I2C2 once after hardware_init(). Concurrent transfers return HAL_BUSY. */
HAL_StatusTypeDef bsp_iic_init(void);

/* Device addresses are unshifted 7-bit values. */
HAL_StatusTypeDef bsp_iic_read_reg(uint8_t address, uint8_t reg, uint8_t *value);
HAL_StatusTypeDef bsp_iic_write_reg(uint8_t address, uint8_t reg, uint8_t value);
HAL_StatusTypeDef bsp_iic_write_byte(uint8_t address, uint8_t value);

#endif
