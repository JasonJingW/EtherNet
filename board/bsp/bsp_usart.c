#include "bsp_usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"

#define BSP_USART_BAUD_RATE (115200U)
#define BSP_USART_RX_CAPACITY (256U)
#define BSP_USART_TX_TIMEOUT_MS (100U)
#define BSP_USART_RX_ERRORS (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE | USART_ISR_PE)
#define BSP_USART_RX_CLEAR (USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF | USART_ICR_PECF)

static UART_HandleTypeDef usart1;
static StreamBufferHandle_t usart_rx;
static volatile bool usart_rx_lost;

HAL_StatusTypeDef bsp_usart_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK2);
    __HAL_RCC_USART1_CLK_ENABLE();

    GPIO_InitTypeDef pins = {
        .Pin = GPIO_PIN_9 | GPIO_PIN_10,
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_PULLUP,
        .Speed = GPIO_SPEED_FREQ_HIGH,
        .Alternate = GPIO_AF7_USART1,
    };
    HAL_GPIO_Init(GPIOA, &pins);

    usart1 = (UART_HandleTypeDef){
        .Instance = USART1,
        .Init =
            {
                .BaudRate = BSP_USART_BAUD_RATE,
                .WordLength = UART_WORDLENGTH_8B,
                .StopBits = UART_STOPBITS_1,
                .Parity = UART_PARITY_NONE,
                .Mode = UART_MODE_TX_RX,
                .HwFlowCtl = UART_HWCONTROL_NONE,
                .OverSampling = UART_OVERSAMPLING_16,
            },
    };
    const HAL_StatusTypeDef status = HAL_UART_Init(&usart1);
    if (status != HAL_OK)
    {
        return status;
    }

    usart_rx = xStreamBufferCreate(BSP_USART_RX_CAPACITY, 1U);
    if (usart_rx == NULL)
    {
        return HAL_ERROR;
    }

    /* HAL handles configuration and TX. RX stays armed and feeds the task's stream buffer. */
    USART1->RQR = USART_RQR_RXFRQ;
    USART1->ICR = BSP_USART_RX_CLEAR;
    HAL_NVIC_SetPriority(USART1_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0U);
    HAL_NVIC_ClearPendingIRQ(USART1_IRQn);
    SET_BIT(USART1->CR3, USART_CR3_EIE);
    SET_BIT(USART1->CR1, USART_CR1_RXNEIE);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    return HAL_OK;
}

bool bsp_usart_read(uint8_t *byte)
{
    const size_t received = xStreamBufferReceive(usart_rx, byte, 1U, portMAX_DELAY);
    taskENTER_CRITICAL();
    const bool lost = usart_rx_lost;
    if (lost)
    {
        usart_rx_lost = false;
        xStreamBufferReset(usart_rx);
    }
    taskEXIT_CRITICAL();
    return received == 1U && !lost;
}

HAL_StatusTypeDef bsp_usart_write(const uint8_t *data, uint16_t length)
{
    return HAL_UART_Transmit(&usart1, data, length, BSP_USART_TX_TIMEOUT_MS);
}

void USART1_IRQHandler(void)
{
    const uint32_t status = USART1->ISR;
    if ((status & BSP_USART_RX_ERRORS) != 0U)
    {
        USART1->ICR = BSP_USART_RX_CLEAR;
        usart_rx_lost = true;
        if ((status & USART_ISR_RXNE) != 0U)
        {
            (void)USART1->RDR;
        }
        return;
    }

    if ((status & USART_ISR_RXNE) != 0U)
    {
        const uint8_t byte = (uint8_t)USART1->RDR;
        BaseType_t task_woken = pdFALSE;
        if (xStreamBufferSendFromISR(usart_rx, &byte, 1U, &task_woken) != 1U)
        {
            usart_rx_lost = true;
        }
        portYIELD_FROM_ISR(task_woken);
    }
}
