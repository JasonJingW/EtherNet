#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "stm32f7xx.h"

/* This project uses STM32F767xGx. Set to 0 to use the defaults below only. */
#ifndef FREERTOS_USE_CHIP_CONFIG
#define FREERTOS_USE_CHIP_CONFIG (1)
#endif
#if FREERTOS_USE_CHIP_CONFIG
#include "stm32f767xgx_config.h"
#endif

/* HAL and FreeRTOS share a 1 ms SysTick; use the configured board clock. */
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       (1000U)
#define configTICK_TYPE_WIDTH_IN_BITS            TICK_TYPE_WIDTH_32_BITS
#define configUSE_PREEMPTION                     (1)
#define configUSE_TIME_SLICING                   (1)
#define configUSE_TICKLESS_IDLE                  (0)
#if defined(CHIP_RTOS_MAX_PRIORITIES)
#define configMAX_PRIORITIES                     CHIP_RTOS_MAX_PRIORITIES
#else
#define configMAX_PRIORITIES                     (8U)
#endif
#if defined(CHIP_RTOS_MINIMAL_STACK_SIZE)
#define configMINIMAL_STACK_SIZE                 CHIP_RTOS_MINIMAL_STACK_SIZE
#else
#define configMINIMAL_STACK_SIZE                 (128U)
#endif
#if defined(CHIP_RTOS_MAX_TASK_NAME_LEN)
#define configMAX_TASK_NAME_LEN                  CHIP_RTOS_MAX_TASK_NAME_LEN
#else
#define configMAX_TASK_NAME_LEN                  (16U)
#endif

/* heap_4.c owns this heap; sizes are bytes, task stacks are 32-bit words. */
#define configSUPPORT_DYNAMIC_ALLOCATION         (1)
#define configSUPPORT_STATIC_ALLOCATION          (0)
#if defined(CHIP_RTOS_TOTAL_HEAP_SIZE)
#define configTOTAL_HEAP_SIZE                    CHIP_RTOS_TOTAL_HEAP_SIZE
#else
#define configTOTAL_HEAP_SIZE                    (32U * 1024U)
#endif
#define configAPPLICATION_ALLOCATED_HEAP          (0)

#define configUSE_MUTEXES                        (1)
#define configUSE_RECURSIVE_MUTEXES              (1)
#define configUSE_COUNTING_SEMAPHORES            (1)
#define configUSE_TIMERS                         PROJECT_USE_SOFTWARE_TIMERS
#if defined(CHIP_RTOS_TIMER_TASK_PRIORITY)
#define configTIMER_TASK_PRIORITY                CHIP_RTOS_TIMER_TASK_PRIORITY
#else
#define configTIMER_TASK_PRIORITY                (2U)
#endif
#if defined(CHIP_RTOS_TIMER_QUEUE_LENGTH)
#define configTIMER_QUEUE_LENGTH                 CHIP_RTOS_TIMER_QUEUE_LENGTH
#else
#define configTIMER_QUEUE_LENGTH                 (10U)
#endif
#if defined(CHIP_RTOS_TIMER_TASK_STACK_DEPTH)
#define configTIMER_TASK_STACK_DEPTH             CHIP_RTOS_TIMER_TASK_STACK_DEPTH
#else
#define configTIMER_TASK_STACK_DEPTH             (256U)
#endif
#define configUSE_CO_ROUTINES                    (0)

#define configUSE_IDLE_HOOK                      (0)
#define configUSE_TICK_HOOK                      (0)
#define configUSE_MALLOC_FAILED_HOOK             (1)
#define configCHECK_FOR_STACK_OVERFLOW           (2)
#define INCLUDE_vTaskDelay                       (1)
#define INCLUDE_xTaskDelayUntil                  (1)
#define INCLUDE_xTaskGetSchedulerState           (1)

/* Priorities 5..15 may call FromISR APIs; numerically smaller is more urgent. */
#define configPRIO_BITS                          __NVIC_PRIO_BITS
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY   (15U)
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5U)
#define configKERNEL_INTERRUPT_PRIORITY          \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

/* Match the chip startup vector names; SysTick uses system_init.c's wrapper. */
#define vPortSVCHandler                          SVC_Handler
#define xPortPendSVHandler                       PendSV_Handler
#define configCHECK_HANDLER_INSTALLATION         (1)

#define configASSERT(condition)                 \
    do {                                        \
        if (!(condition)) {                     \
            __disable_irq();                    \
            for (;;) { }                        \
        }                                       \
    } while (0)

#endif
