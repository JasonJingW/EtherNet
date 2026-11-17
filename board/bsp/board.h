#ifndef BOARD_H
#define BOARD_H
#include "stm32f7xx_hal.h"
void Board_Init(void);
void Board_ClockInit(void);
void Board_LEDWrite(unsigned led, unsigned on);
unsigned Board_LEDRead(unsigned led);
void Board_LEDToggle(unsigned led);
void Board_Fatal(const char *reason);
#endif
