#include "bsp_iic.h"

/* RM0410 table 216: Standard-mode timing with a 16 MHz I2C kernel clock. */
#define BSP_IIC_TIMING (0x30420F13U)
#define BSP_IIC_TIMEOUT_MS (20U)

static I2C_HandleTypeDef iic2;

HAL_StatusTypeDef bsp_iic_init(void)
{
    GPIO_InitTypeDef pins = {
        .Pin = GPIO_PIN_4 | GPIO_PIN_5,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF4_I2C2,
    };

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
    {
        return HAL_ERROR;
    }
    __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_HSI);
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();
    HAL_GPIO_Init(GPIOH, &pins);
    iic2.Instance = I2C2;
    iic2.Init.Timing = BSP_IIC_TIMING;
    iic2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    return HAL_I2C_Init(&iic2);
}

HAL_StatusTypeDef bsp_iic_read_reg(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (address > 0x7FU || value == NULL)
    {
        return HAL_ERROR;
    }
    return HAL_I2C_Mem_Read(&iic2, (uint16_t)address << 1U, reg, I2C_MEMADD_SIZE_8BIT, value, 1U,
                            BSP_IIC_TIMEOUT_MS);
}

HAL_StatusTypeDef bsp_iic_write_reg(uint8_t address, uint8_t reg, uint8_t value)
{
    if (address > 0x7FU)
    {
        return HAL_ERROR;
    }
    return HAL_I2C_Mem_Write(&iic2, (uint16_t)address << 1U, reg, I2C_MEMADD_SIZE_8BIT, &value, 1U,
                             BSP_IIC_TIMEOUT_MS);
}
