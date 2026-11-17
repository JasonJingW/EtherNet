#include "board.h"
#include "project_config.h"

/* Apollo V2+ F767 hardware manual: PB1/DS0 and PB0/DS1, active low. */
static const uint16_t led_pins[] = {GPIO_PIN_1, GPIO_PIN_0};

void Board_LEDWrite(unsigned led, unsigned on)
{
    if (led < 2) HAL_GPIO_WritePin(GPIOB, led_pins[led], on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}
unsigned Board_LEDRead(unsigned led)
{
    return led < 2 && (GPIOB->ODR & led_pins[led]) == 0;
}
void Board_LEDToggle(unsigned led)
{
    if (led < 2) HAL_GPIO_TogglePin(GPIOB, led_pins[led]);
}
void Board_Init(void)
{
    GPIO_InitTypeDef io = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_SET);
    io.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    io.Mode = GPIO_MODE_OUTPUT_PP;
    io.Pull = GPIO_NOPULL;
    io.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &io);
}
void Board_Fatal(const char *reason)
{
    (void)reason; /* The log level can compile error output out. */
    RLOG_ERR("FATAL: %s", reason);
    while (1) { Board_LEDToggle(0); HAL_Delay(100); }
}
void Board_ClockInit(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 25;
    osc.PLL.PLLN = 432;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Board_Fatal("25MHz HSE/PLL");
    if (HAL_PWREx_EnableOverDrive() != HAL_OK) Board_Fatal("overdrive");
    clk.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_7) != HAL_OK) Board_Fatal("clock switch");
}
