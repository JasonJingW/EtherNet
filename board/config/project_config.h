#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

#include "version.h"

/* Default J-Link target device used by project_flash.py. */
#define PROJECT_CHIP_MODEL "STM32F767IG"

/* Common C standard library interfaces. */
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Selected chip: HAL includes the matching CMSIS device/core headers. */
#include "stm32f7xx_hal.h"
#include "system_init.h"

/* RTT transport and logging macros used with the host RTT Viewer. */
#if PROJECT_USE_RTT
#include "SEGGER_RTT.h"
#else
#define segger_rtt_init() ((void)0)
#define RLOG_PRINT(...) ((void)0)
#define RLOG_INFO(...) ((void)0)
#define RLOG_ERR(...) ((void)0)
#define RLOG_WAR(...) ((void)0)
#endif

/* Operating system interfaces; FreeRTOS.h must precede the API headers. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "event_groups.h"
#include "stream_buffer.h"
#include "message_buffer.h"

#endif
