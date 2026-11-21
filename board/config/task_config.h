#ifndef TASK_CONFIG_H
#define TASK_CONFIG_H

#include "project_config.h"

/* Project-wide task priority configuration. */
#define HEARTBEAT_TASK_PRIORITY (tskIDLE_PRIORITY + 1U)
#define AUDIO_TASK_PRIORITY (HEARTBEAT_TASK_PRIORITY + 1U)
#define NETWORK_TASK_PRIORITY HEARTBEAT_TASK_PRIORITY

#endif
