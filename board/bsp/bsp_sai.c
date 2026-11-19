#include "bsp_sai.h"
#include <stdbool.h>
#include <string.h>
#include "stm32f767xgx_config.h"
#include "FreeRTOS.h"
#include "task.h"

/* hardware_init() provides the shared 1 MHz PLL input (25 MHz HSE / 25).
 * PLLI2S Q and the SAI master divider give 47.991 kHz at MCLK = 256 * Fs. */
#define BSP_SAI_PLLI2S_N (344U)
#define BSP_SAI_PLLI2S_Q (7U)
#define BSP_SAI_MCKDIV (2U)
#define BSP_SAI_FRAME_SAMPLES (2U)
#define BSP_SAI_HALF_SAMPLES (512U * BSP_SAI_FRAME_SAMPLES)
#define BSP_SAI_DMA_SAMPLES (2U * BSP_SAI_HALF_SAMPLES)
#define BSP_SAI_TIMEOUT_MS (50U)
#define BSP_SAI_ERROR_FLAGS (SAI_xSR_OVRUDR | SAI_xSR_AFSDET | SAI_xSR_LFSDET)

__DMA_U16_DEFINE(sai_pcm, BSP_SAI_DMA_SAMPLES);
__DMA_U16_DEFINE(sai_silence, BSP_SAI_FRAME_SAMPLES);
static DMA_HandleTypeDef sai_tx_dma;
static DMA_HandleTypeDef sai_rx_dma;

static HAL_StatusTypeDef bsp_sai_dma_init(DMA_HandleTypeDef *dma, DMA_Stream_TypeDef *stream,
                                          uint32_t direction)
{
    dma->Instance = stream;
    dma->Init = (DMA_InitTypeDef){
        .Channel = DMA_CHANNEL_0,
        .Direction = direction,
        .PeriphInc = DMA_PINC_DISABLE,
        .MemInc = DMA_MINC_ENABLE,
        .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD,
        .MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
        .Mode = DMA_CIRCULAR,
        .Priority = DMA_PRIORITY_HIGH,
        .FIFOMode = DMA_FIFOMODE_DISABLE,
    };
    return HAL_DMA_Init(dma);
}

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
    status = HAL_SAI_InitProtocol(rx, SAI_I2S_STANDARD, SAI_PROTOCOL_DATASIZE_16BIT, 2U);
    if (status != HAL_OK)
    {
        return status;
    }

    /* RM0410 DMA2 request mapping: SAI1_A on stream 1, SAI1_B on stream 5. */
    __HAL_RCC_DMA2_CLK_ENABLE();
    status = bsp_sai_dma_init(&sai_tx_dma, DMA2_Stream1, DMA_MEMORY_TO_PERIPH);
    if (status != HAL_OK)
    {
        return status;
    }
    return bsp_sai_dma_init(&sai_rx_dma, DMA2_Stream5, DMA_PERIPH_TO_MEMORY);
}

static HAL_StatusTypeDef bsp_sai_start(SAI_HandleTypeDef *sai, DMA_HandleTypeDef *dma,
                                       uint16_t *samples, uint32_t count)
{
    const bool transmit = dma->Init.Direction == DMA_MEMORY_TO_PERIPH;
    const uint32_t peripheral = (uint32_t)&sai->Instance->DR;
    const uint32_t memory = (uint32_t)samples;
    __DMB();
    const HAL_StatusTypeDef status =
        HAL_DMA_Start(dma, transmit ? memory : peripheral, transmit ? peripheral : memory, count);
    if (status != HAL_OK)
    {
        return status;
    }

    SET_BIT(sai->Instance->CR2, SAI_xCR2_FFLUSH);
    sai->Instance->CLRFR = BSP_SAI_ERROR_FLAGS;
    SET_BIT(sai->Instance->CR1, SAI_xCR1_DMAEN);
    if (transmit)
    {
        /* Prime the TX FIFO before enabling the audio clocks, as in HAL SAI TX. */
        const uint32_t started = HAL_GetTick();
        while ((sai->Instance->SR & SAI_xSR_FLVL) == SAI_FIFOSTATUS_EMPTY)
        {
            if (HAL_GetTick() - started >= BSP_SAI_TIMEOUT_MS)
            {
                return HAL_TIMEOUT;
            }
        }
    }
    __HAL_SAI_ENABLE(sai);
    return HAL_OK;
}

static HAL_StatusTypeDef bsp_sai_stop(SAI_HandleTypeDef *sai, DMA_HandleTypeDef *dma)
{
    if (dma->State != HAL_DMA_STATE_BUSY)
    {
        return HAL_OK;
    }

    __HAL_SAI_DISABLE(sai);
    const uint32_t started = HAL_GetTick();
    while ((sai->Instance->CR1 & SAI_xCR1_SAIEN) != 0U)
    {
        if (HAL_GetTick() - started >= BSP_SAI_TIMEOUT_MS)
        {
            break;
        }
    }
    const bool stopped = (sai->Instance->CR1 & SAI_xCR1_SAIEN) == 0U;
    CLEAR_BIT(sai->Instance->CR1, SAI_xCR1_DMAEN);
    const HAL_StatusTypeDef status = HAL_DMA_Abort(dma);
    SET_BIT(sai->Instance->CR2, SAI_xCR2_FFLUSH);
    sai->Instance->CLRFR = BSP_SAI_ERROR_FLAGS;
    return stopped ? status : HAL_TIMEOUT;
}

static HAL_StatusTypeDef bsp_sai_wait_half(SAI_HandleTypeDef *sai, DMA_HandleTypeDef *dma,
                                           uint32_t complete, uint32_t next)
{
    const uint32_t errors = __HAL_DMA_GET_TE_FLAG_INDEX(dma) | __HAL_DMA_GET_DME_FLAG_INDEX(dma);
    const uint32_t started = HAL_GetTick();
    for (;;)
    {
        if (__HAL_DMA_GET_FLAG(dma, errors | next) != 0U ||
            (sai->Instance->SR & BSP_SAI_ERROR_FLAGS) != 0U)
        {
            return HAL_ERROR;
        }
        if (__HAL_DMA_GET_FLAG(dma, complete) != 0U)
        {
            __HAL_DMA_CLEAR_FLAG(dma, complete);
            __DMB();
            return HAL_OK;
        }
        if (HAL_GetTick() - started >= BSP_SAI_TIMEOUT_MS)
        {
            return HAL_TIMEOUT;
        }
        vTaskDelay(1U);
    }
}

static size_t bsp_sai_fill_half(uint16_t *half, const int16_t *pcm, size_t samples)
{
    const size_t count = samples < BSP_SAI_HALF_SAMPLES ? samples : BSP_SAI_HALF_SAMPLES;
    memcpy(half, pcm, count * sizeof(*pcm));
    memset(half + count, 0, (BSP_SAI_HALF_SAMPLES - count) * sizeof(*half));
    return count;
}

HAL_StatusTypeDef bsp_sai_play(SAI_HandleTypeDef *tx, const int16_t *pcm, size_t frames)
{
    const size_t samples = frames * BSP_SAI_FRAME_SAMPLES;
    const size_t blocks = (samples + BSP_SAI_HALF_SAMPLES - 1U) / BSP_SAI_HALF_SAMPLES;
    size_t queued = bsp_sai_fill_half(sai_pcm, pcm, samples);
    queued += bsp_sai_fill_half(sai_pcm + BSP_SAI_HALF_SAMPLES, pcm + queued, samples - queued);

    DMA_HandleTypeDef *dma = &sai_tx_dma;
    const uint32_t ht = __HAL_DMA_GET_HT_FLAG_INDEX(dma);
    const uint32_t tc = __HAL_DMA_GET_TC_FLAG_INDEX(dma);
    HAL_StatusTypeDef status = bsp_sai_start(tx, dma, sai_pcm, BSP_SAI_DMA_SAMPLES);
    /* The extra silent half lets the final samples leave the TX FIFO. */
    for (size_t block = 0U; status == HAL_OK && block <= blocks; ++block)
    {
        const bool first = (block & 1U) == 0U;
        const uint32_t next = first ? tc : ht;
        status = bsp_sai_wait_half(tx, dma, first ? ht : tc, next);
        if (status == HAL_OK && block < blocks)
        {
            uint16_t *half = sai_pcm + (first ? 0U : BSP_SAI_HALF_SAMPLES);
            queued += bsp_sai_fill_half(half, pcm + queued, samples - queued);
            __DMB();
            if (__HAL_DMA_GET_FLAG(dma, next) != 0U)
            {
                status = HAL_ERROR;
            }
        }
    }
    const HAL_StatusTypeDef stop_status = bsp_sai_stop(tx, dma);
    return status == HAL_OK ? stop_status : status;
}

HAL_StatusTypeDef bsp_sai_record(SAI_HandleTypeDef *tx, SAI_HandleTypeDef *rx, int16_t *pcm,
                                 size_t frames)
{
    size_t remaining = frames * BSP_SAI_FRAME_SAMPLES;
    DMA_HandleTypeDef *dma = &sai_rx_dma;
    const uint32_t ht = __HAL_DMA_GET_HT_FLAG_INDEX(dma);
    const uint32_t tc = __HAL_DMA_GET_TC_FLAG_INDEX(dma);
    HAL_StatusTypeDef status = bsp_sai_start(rx, dma, sai_pcm, BSP_SAI_DMA_SAMPLES);
    if (status == HAL_OK)
    {
        /* The synchronous receiver needs the master's clocks throughout capture. */
        memset(sai_silence, 0, sizeof(sai_silence));
        status = bsp_sai_start(tx, &sai_tx_dma, sai_silence, BSP_SAI_FRAME_SAMPLES);
    }
    for (bool first = true; status == HAL_OK && remaining > 0U; first = !first)
    {
        const uint32_t next = first ? tc : ht;
        status = bsp_sai_wait_half(rx, dma, first ? ht : tc, next);
        if (status == HAL_OK)
        {
            const size_t count =
                remaining < BSP_SAI_HALF_SAMPLES ? remaining : BSP_SAI_HALF_SAMPLES;
            memcpy(pcm, sai_pcm + (first ? 0U : BSP_SAI_HALF_SAMPLES), count * sizeof(*pcm));
            pcm += count;
            remaining -= count;
            __DMB();
            if (__HAL_DMA_GET_FLAG(dma, next) != 0U ||
                (tx->Instance->SR & BSP_SAI_ERROR_FLAGS) != 0U)
            {
                status = HAL_ERROR;
            }
        }
    }
    const HAL_StatusTypeDef rx_stop = bsp_sai_stop(rx, dma);
    const HAL_StatusTypeDef tx_stop = bsp_sai_stop(tx, &sai_tx_dma);
    if (status == HAL_OK)
    {
        status = rx_stop == HAL_OK ? tx_stop : rx_stop;
    }
    return status;
}
