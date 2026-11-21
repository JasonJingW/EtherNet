#include "bsp_eth.h"
#include "bsp_iic.h"
#include "stm32f767xgx_config.h"
#include <stdbool.h>
#include <string.h>

#define PHY_RESET_EXPANDER (0x20U)
#define PHY_RESET_PIN (0x80U)
#define PHY_REG_BMSR (0x01U)
#define PHY_REG_ID1 (2U)
#define PHY_REG_ID2 (3U)
#define PHY_REG_STATUS (0x1FU)
#define PHY_ADDRESS_COUNT (2U)
#define PHY_ID1 (0x0007U)
#define PHY_ID2 (0xC0F0U)
#define PHY_ID2_MASK (0xFFF0U)
#define PHY_LINK_UP (0x0004U)
#define PHY_AUTONEG_DONE (0x0020U)
#define PHY_MODE_MASK (0x001CU)
#define PHY_MODE_10_HALF (0x0004U)
#define PHY_MODE_10_FULL (0x0014U)
#define PHY_MODE_100_HALF (0x0008U)
#define PHY_MODE_100_FULL (0x0018U)
#define ETH_DMA_BUFFER_SIZE (1536U)
#define ETH_TX_TIMEOUT_MS (5U)
#define ETH_HEADER_SIZE (14U)

static ETH_DMADescTypeDef rx_descriptors[ETH_RX_DESC_CNT] __DMA_DATA __ALIGNED(32);
static ETH_DMADescTypeDef tx_descriptors[ETH_TX_DESC_CNT] __DMA_DATA __ALIGNED(32);
static uint8_t rx_buffers[ETH_RX_DESC_CNT][ETH_DMA_BUFFER_SIZE] __DMA_DATA __ALIGNED(32);
static uint8_t tx_buffer[ETH_DMA_BUFFER_SIZE] __DMA_DATA __ALIGNED(32);
static uint8_t rx_frame[BSP_ETH_FRAME_SIZE];
static bool rx_in_use[ETH_RX_DESC_CNT];
static ETH_HandleTypeDef eth;
static uint8_t eth_mac[BSP_ETH_MAC_SIZE];
static uint32_t phy_address;
static uint32_t eth_mode;
static bool eth_needs_init;
static bool eth_tx_failed;

void HAL_ETH_MspInit(ETH_HandleTypeDef *handle)
{
    if (handle->Instance != ETH)
    {
        return;
    }

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_ETH_CLK_ENABLE();

    GPIO_InitTypeDef pins = {
        .Mode = GPIO_MODE_AF_PP,
        .Pull = GPIO_NOPULL,
        .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
        .Alternate = GPIO_AF11_ETH,
    };
    pins.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7; /* REF_CLK, MDIO, CRS_DV */
    HAL_GPIO_Init(GPIOA, &pins);
    pins.Pin = GPIO_PIN_11; /* TX_EN */
    HAL_GPIO_Init(GPIOB, &pins);
    pins.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5; /* MDC, RXD0, RXD1 */
    HAL_GPIO_Init(GPIOC, &pins);
    pins.Pin = GPIO_PIN_13 | GPIO_PIN_14; /* TXD0, TXD1 */
    HAL_GPIO_Init(GPIOG, &pins);
}

static HAL_StatusTypeDef bsp_eth_reset_phy(void)
{
    /* Apollo V1 inverts PCF8574T P7 before the LAN8720A nRST pin. */
    HAL_StatusTypeDef status = bsp_iic_write_byte(PHY_RESET_EXPANDER, UINT8_MAX);
    if (status != HAL_OK)
    {
        return status;
    }
    HAL_Delay(20U);
    status = bsp_iic_write_byte(PHY_RESET_EXPANDER, (uint8_t)~PHY_RESET_PIN);
    if (status == HAL_OK)
    {
        HAL_Delay(100U);
    }
    return status;
}

static HAL_StatusTypeDef bsp_eth_init_mac(void)
{
    memset(rx_in_use, 0, sizeof(rx_in_use));
    eth = (ETH_HandleTypeDef){
        .Instance = ETH,
        .Init =
            {
                .MACAddr = eth_mac,
                .MediaInterface = HAL_ETH_RMII_MODE,
                .TxDesc = tx_descriptors,
                .RxDesc = rx_descriptors,
                .RxBuffLen = ETH_DMA_BUFFER_SIZE,
            },
    };
    const HAL_StatusTypeDef status = HAL_ETH_Init(&eth);
    if (status == HAL_OK)
    {
        HAL_ETH_SetMDIOClockRange(&eth);
    }
    return status;
}

static HAL_StatusTypeDef bsp_eth_detect_phy(void)
{
    const HAL_StatusTypeDef status = bsp_eth_init_mac();
    if (status != HAL_OK)
    {
        return status;
    }

    /* LAN8720A has one address strap. Mask the silicon revision in ID2. */
    for (uint32_t index = 0U; index < PHY_ADDRESS_COUNT; ++index)
    {
        uint32_t id1;
        uint32_t id2;
        if (HAL_ETH_ReadPHYRegister(&eth, index, PHY_REG_ID1, &id1) == HAL_OK &&
            HAL_ETH_ReadPHYRegister(&eth, index, PHY_REG_ID2, &id2) == HAL_OK && id1 == PHY_ID1 &&
            (id2 & PHY_ID2_MASK) == PHY_ID2)
        {
            phy_address = index;
            return HAL_OK;
        }
    }
    return HAL_ERROR;
}

HAL_StatusTypeDef bsp_eth_init(uint8_t mac[BSP_ETH_MAC_SIZE])
{
    HAL_StatusTypeDef status = bsp_iic_init();
    if (status != HAL_OK)
    {
        return status;
    }

    status = bsp_eth_reset_phy();
    if (status != HAL_OK)
    {
        return status;
    }

    const uint32_t id = HAL_GetUIDw0() ^ HAL_GetUIDw2();
    const uint8_t address[BSP_ETH_MAC_SIZE] = {
        0x02U,
        (uint8_t)HAL_GetUIDw1(),
        (uint8_t)(id >> 24U),
        (uint8_t)(id >> 16U),
        (uint8_t)(id >> 8U),
        (uint8_t)id,
    };
    memcpy(eth_mac, address, sizeof(eth_mac));

    status = bsp_eth_detect_phy();
    if (status == HAL_OK)
    {
        memcpy(mac, eth_mac, sizeof(eth_mac));
    }
    return status;
}

static uint32_t bsp_eth_read_mode(void)
{
    uint32_t bmsr;
    uint32_t mode;
    /* Read the latch-low link bit twice to obtain the current link state. */
    if (HAL_ETH_ReadPHYRegister(&eth, phy_address, PHY_REG_BMSR, &bmsr) != HAL_OK ||
        HAL_ETH_ReadPHYRegister(&eth, phy_address, PHY_REG_BMSR, &bmsr) != HAL_OK ||
        (bmsr & (PHY_LINK_UP | PHY_AUTONEG_DONE)) != (PHY_LINK_UP | PHY_AUTONEG_DONE) ||
        HAL_ETH_ReadPHYRegister(&eth, phy_address, PHY_REG_STATUS, &mode) != HAL_OK)
    {
        return 0U;
    }

    mode &= PHY_MODE_MASK;
    switch (mode)
    {
    case PHY_MODE_10_HALF:
    case PHY_MODE_10_FULL:
    case PHY_MODE_100_HALF:
    case PHY_MODE_100_FULL:
        return mode;
    default:
        return 0U;
    }
}

uint32_t bsp_eth_poll_link(void)
{
    const uint32_t mode = bsp_eth_read_mode();
    if (eth_mode != 0U &&
        (eth_mode != mode || eth_tx_failed || (eth.Instance->DMASR & ETH_DMASR_FBES) != 0U))
    {
        HAL_ETH_Stop(&eth);
        eth_mode = 0U;
        eth_needs_init = true;
        return 0U;
    }
    if (mode == 0U)
    {
        return 0U;
    }

    const bool fast = mode == PHY_MODE_100_HALF || mode == PHY_MODE_100_FULL;
    if (eth_mode == 0U)
    {
        if (eth_needs_init && bsp_eth_init_mac() != HAL_OK)
        {
            return 0U;
        }
        ETH_MACConfigTypeDef config;
        HAL_ETH_GetMACConfig(&eth, &config);
        /* lwIP validates IPv4 and transport checksums in software. */
        config.ChecksumOffload = DISABLE;
        config.Speed = fast ? ETH_SPEED_100M : ETH_SPEED_10M;
        config.DuplexMode = (mode == PHY_MODE_100_FULL || mode == PHY_MODE_10_FULL)
                                ? ETH_FULLDUPLEX_MODE
                                : ETH_HALFDUPLEX_MODE;
        if (HAL_ETH_SetMACConfig(&eth, &config) != HAL_OK || HAL_ETH_Start(&eth) != HAL_OK)
        {
            eth_needs_init = true;
            return 0U;
        }
        eth_needs_init = false;
        eth_tx_failed = false;
        eth_mode = mode;
    }
    return fast ? 100U : 10U;
}

void HAL_ETH_RxAllocateCallback(uint8_t **buffer)
{
    *buffer = NULL;
    for (size_t index = 0U; index < ETH_RX_DESC_CNT; ++index)
    {
        if (!rx_in_use[index])
        {
            rx_in_use[index] = true;
            *buffer = rx_buffers[index];
            return;
        }
    }
}

void HAL_ETH_RxLinkCallback(void **start, void **end, uint8_t *buffer, uint16_t length)
{
    /* Standard Ethernet frames fit one descriptor. The caller discards split frames. */
    if (*start == NULL && length <= sizeof(rx_frame))
    {
        __DMB();
        memcpy(rx_frame, buffer, length);
    }
    *start = rx_frame;
    *end = rx_frame;
    const size_t index = ((uintptr_t)buffer - (uintptr_t)rx_buffers) / sizeof(rx_buffers[0]);
    rx_in_use[index] = false;
}

const uint8_t *bsp_eth_receive(uint16_t *length)
{
    if (eth_mode == 0U)
    {
        return NULL;
    }
    for (size_t attempt = 0U; attempt < ETH_RX_DESC_CNT; ++attempt)
    {
        void *packet = NULL;
        const HAL_StatusTypeDef status = HAL_ETH_ReadData(&eth, &packet);
        if ((eth.Instance->DMASR & ETH_DMASR_RBUS) != 0U && eth.RxDescList.RxBuildDescCnt == 0U)
        {
            eth.Instance->DMASR = ETH_DMASR_RBUS;
            eth.Instance->DMARPDR = 0U;
        }
        if (status != HAL_OK)
        {
            break;
        }
        if (eth.RxDescList.RxDescCnt == 1U && eth.RxDescList.RxDataLength >= ETH_HEADER_SIZE &&
            eth.RxDescList.RxDataLength <= sizeof(rx_frame) &&
            (eth.RxDescList.pRxLastRxDesc & ETH_DMARXDESC_ES) == 0U)
        {
            *length = (uint16_t)eth.RxDescList.RxDataLength;
            return packet;
        }
    }
    return NULL;
}

HAL_StatusTypeDef bsp_eth_send(const uint8_t *frame, uint16_t length)
{
    if (eth_mode == 0U || eth_tx_failed || frame == NULL || length < ETH_HEADER_SIZE ||
        length > BSP_ETH_FRAME_SIZE)
    {
        return HAL_ERROR;
    }

    memcpy(tx_buffer, frame, length);
    ETH_BufferTypeDef buffer = {.buffer = tx_buffer, .len = length};
    const ETH_TxPacketConfigTypeDef packet = {
        .Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD | ETH_TX_PACKETS_FEATURES_CSUM,
        .ChecksumCtrl = ETH_CHECKSUM_DISABLE,
        .CRCPadCtrl = ETH_CRC_PAD_INSERT,
        .Length = length,
        .TxBuffer = &buffer,
    };
    const HAL_StatusTypeDef status = HAL_ETH_Transmit(&eth, &packet, ETH_TX_TIMEOUT_MS);
    eth_tx_failed = status != HAL_OK;
    return status;
}
