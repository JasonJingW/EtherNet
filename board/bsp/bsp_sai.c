#include "bsp_sai.h"

/* hardware_init() provides the shared 1 MHz PLL input (25 MHz HSE / 25).
 * PLLI2S Q and the SAI master divider give 47.991 kHz at MCLK = 256 * Fs. */
#define BSP_SAI_PLLI2S_N (344U)
#define BSP_SAI_PLLI2S_Q (7U)
#define BSP_SAI_MCKDIV (2U)

HAL_StatusTypeDef bsp_sai_init(SAI_HandleTypeDef *tx, SAI_HandleTypeDef *rx)
{
    RCC_PeriphCLKInitTypeDef clock = {
        .PeriphClockSelection = RCC_PERIPHCLK_SAI1,
        .PLLI2S = {.PLLI2SN = BSP_SAI_PLLI2S_N, .PLLI2SQ = BSP_SAI_PLLI2S_Q},
        .PLLI2SDivQ = 1U,
        .Sai1ClockSelection = RCC_SAI1CLKSOURCE_PLLI2S,
    };
    GPIO_InitTypeDef pins = {
        .Pin = GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF6_SAI1,
    };
    if (tx == NULL || rx == NULL || tx == rx)
    {
        return HAL_ERROR;
    }

    HAL_StatusTypeDef status = HAL_RCCEx_PeriphCLKConfig(&clock);
    if (status != HAL_OK)
    {
        return status;
    }
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_SAI1_CLK_ENABLE();
    HAL_GPIO_Init(GPIOE, &pins);

    *tx = (SAI_HandleTypeDef){
        .Instance = SAI1_Block_A,
        .Init =
            {
                .AudioMode = SAI_MODEMASTER_TX,
                .Synchro = SAI_ASYNCHRONOUS,
                .OutputDrive = SAI_OUTPUTDRIVE_ENABLE,
                .NoDivider = SAI_MASTERDIVIDER_ENABLE,
                .FIFOThreshold = SAI_FIFOTHRESHOLD_1QF,
                .AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV,
                .Mckdiv = BSP_SAI_MCKDIV,
            },
    };
    *rx = (SAI_HandleTypeDef){
        .Instance = SAI1_Block_B,
        .Init =
            {
                .AudioMode = SAI_MODESLAVE_RX,
                .Synchro = SAI_SYNCHRONOUS,
                .FIFOThreshold = SAI_FIFOTHRESHOLD_1QF,
                .AudioFrequency = SAI_AUDIO_FREQUENCY_MCKDIV,
            },
    };

    status = HAL_SAI_InitProtocol(tx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2U);
    if (status != HAL_OK)
    {
        return status;
    }
    return HAL_SAI_InitProtocol(rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2U);
}
