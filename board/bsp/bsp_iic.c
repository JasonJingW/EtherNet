#include "bsp_iic.h"
#include <stdbool.h>

/* RM0410 table 216: Standard-mode timing with a 16 MHz I2C kernel clock. */
#define BSP_IIC_TIMING (0x30420F13U)
#define BSP_IIC_TIMEOUT_MS (20U)

static I2C_HandleTypeDef iic2;
static bool iic_ready;
static bool iic_busy;

static bool bsp_iic_acquire(void)
{
    const uint32_t primask = __get_PRIMASK();
    __disable_irq();
    const bool available = !iic_busy;
    if (available)
    {
        iic_busy = true;
    }
    __set_PRIMASK(primask);
    return available;
}

static HAL_StatusTypeDef bsp_iic_release(HAL_StatusTypeDef status)
{
    __DMB();
    iic_busy = false;
    return status;
}

HAL_StatusTypeDef bsp_iic_init(void)
{
    if (!bsp_iic_acquire())
    {
        return HAL_BUSY;
    }
    if (iic_ready)
    {
        return bsp_iic_release(HAL_OK);
    }

    GPIO_InitTypeDef pins = {
        .Pin = GPIO_PIN_4 | GPIO_PIN_5,
        .Mode = GPIO_MODE_AF_OD,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_LOW,
        .Alternate = GPIO_AF4_I2C2,
    };

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_HSIRDY) == RESET)
    {
        return bsp_iic_release(HAL_ERROR);
    }
    __HAL_RCC_I2C2_CONFIG(RCC_I2C2CLKSOURCE_HSI);
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_I2C2_CLK_ENABLE();
    HAL_GPIO_Init(GPIOH, &pins);
    iic2.Instance = I2C2;
    iic2.Init.Timing = BSP_IIC_TIMING;
    iic2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    const HAL_StatusTypeDef status = HAL_I2C_Init(&iic2);
    iic_ready = status == HAL_OK;
    return bsp_iic_release(status);
}

static HAL_StatusTypeDef bsp_iic_begin(uint8_t address)
{
    if (address > 0x7FU)
    {
        return HAL_ERROR;
    }
    if (!bsp_iic_acquire())
    {
        return HAL_BUSY;
    }
    return iic_ready ? HAL_OK : bsp_iic_release(HAL_ERROR);
}

HAL_StatusTypeDef bsp_iic_read_reg(uint8_t address, uint8_t reg, uint8_t *value)
{
    if (value == NULL)
    {
        return HAL_ERROR;
    }
    const HAL_StatusTypeDef status = bsp_iic_begin(address);
    if (status != HAL_OK)
    {
        return status;
    }
    return bsp_iic_release(HAL_I2C_Mem_Read(&iic2, (uint16_t)address << 1U, reg,
                                            I2C_MEMADD_SIZE_8BIT, value, 1U, BSP_IIC_TIMEOUT_MS));
}

HAL_StatusTypeDef bsp_iic_write_reg(uint8_t address, uint8_t reg, uint8_t value)
{
    const HAL_StatusTypeDef status = bsp_iic_begin(address);
    if (status != HAL_OK)
    {
        return status;
    }
    return bsp_iic_release(HAL_I2C_Mem_Write(&iic2, (uint16_t)address << 1U, reg,
                                             I2C_MEMADD_SIZE_8BIT, &value, 1U, BSP_IIC_TIMEOUT_MS));
}

HAL_StatusTypeDef bsp_iic_write_byte(uint8_t address, uint8_t value)
{
    const HAL_StatusTypeDef status = bsp_iic_begin(address);
    if (status != HAL_OK)
    {
        return status;
    }
    return bsp_iic_release(
        HAL_I2C_Master_Transmit(&iic2, (uint16_t)address << 1U, &value, 1U, BSP_IIC_TIMEOUT_MS));
}
