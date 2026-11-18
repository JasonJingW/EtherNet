#ifndef BSP_GPIO_H
#define BSP_GPIO_H

typedef enum
{
    MCU_LED,
    MCU_LED1,
} bsp_gpio_t;

void bsp_gpio_init(void);

/* Set/get use logical active state. Set and toggle apply only to outputs. */
void bsp_gpio_set(bsp_gpio_t id, unsigned active);
unsigned bsp_gpio_get(bsp_gpio_t id);
void bsp_gpio_toggle(bsp_gpio_t id);

#endif
