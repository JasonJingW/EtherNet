#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

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
#include "SEGGER_RTT.h"

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
