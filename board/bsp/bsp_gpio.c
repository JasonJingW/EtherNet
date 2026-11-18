#include "bsp_gpio.h"
#include "stm32f7xx_hal.h"

typedef struct
{
    const char *name;
    GPIO_TypeDef *port;
    uint16_t pin;
    uint32_t mode;
    uint32_t pull;
    GPIO_PinState initial_level;
    GPIO_PinState active_level;
} bsp_gpio_config_t;

/* Apollo V2+ F767 GPIOs, indexed by the public GPIO API. */
static const bsp_gpio_config_t gpio_configs[] = {
    /* NAME, PORT, PIN, MODE, PULL, INIT, VALID */
    // clang-format off: keep one GPIO per row.
    [MCU_LED] = {"MCU_LED", GPIOB, GPIO_PIN_1, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_PIN_SET, GPIO_PIN_RESET},
    [MCU_LED1] = {"MCU_LED1", GPIOB, GPIO_PIN_0, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_PIN_SET, GPIO_PIN_RESET},
    // clang-format on
};

#define BSP_GPIO_COUNT (sizeof(gpio_configs) / sizeof(gpio_configs[0]))

static void bsp_gpio_enable_port_clock(GPIO_TypeDef *port)
{
    /* On STM32F767, GPIOA..GPIOK addresses and AHB1 enable bits both follow port order. */
    const uintptr_t port_index = ((uintptr_t)port - GPIOA_BASE) / (GPIOB_BASE - GPIOA_BASE);
    const uint32_t clock_mask = RCC_AHB1ENR_GPIOAEN << port_index;
    SET_BIT(RCC->AHB1ENR, clock_mask);
    (void)READ_BIT(RCC->AHB1ENR, clock_mask);
}

static unsigned bsp_gpio_is_output(const bsp_gpio_config_t *config)
{
    return config->mode == GPIO_MODE_OUTPUT_PP || config->mode == GPIO_MODE_OUTPUT_OD;
}

void bsp_gpio_init(void)
{
    for (unsigned id = 0; id < BSP_GPIO_COUNT; ++id)
    {
        const bsp_gpio_config_t *config = &gpio_configs[id];
        GPIO_InitTypeDef io = {
            .Pin = config->pin,
            .Mode = config->mode,
            .Pull = config->pull,
            .Speed = GPIO_SPEED_FREQ_LOW,
        };
        bsp_gpio_enable_port_clock(config->port);
        if (bsp_gpio_is_output(config))
        {
            HAL_GPIO_WritePin(config->port, config->pin, config->initial_level);
        }
        HAL_GPIO_Init(config->port, &io);
    }
}

void bsp_gpio_set(bsp_gpio_t id, unsigned active)
{
    if ((unsigned)id < BSP_GPIO_COUNT && bsp_gpio_is_output(&gpio_configs[id]))
    {
        const bsp_gpio_config_t *config = &gpio_configs[id];
        const GPIO_PinState inactive_level =
            config->active_level == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET;
        HAL_GPIO_WritePin(config->port, config->pin,
                          active ? config->active_level : inactive_level);
    }
}

unsigned bsp_gpio_get(bsp_gpio_t id)
{
    if ((unsigned)id >= BSP_GPIO_COUNT)
    {
        return 0;
    }
    const bsp_gpio_config_t *config = &gpio_configs[id];
    const uint32_t pin_levels = bsp_gpio_is_output(config) ? config->port->ODR : config->port->IDR;
    const GPIO_PinState level = (pin_levels & config->pin) != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET;
    return level == config->active_level;
}

void bsp_gpio_toggle(bsp_gpio_t id)
{
    if ((unsigned)id < BSP_GPIO_COUNT && bsp_gpio_is_output(&gpio_configs[id]))
    {
        const bsp_gpio_config_t *config = &gpio_configs[id];
        HAL_GPIO_TogglePin(config->port, config->pin);
    }
}
