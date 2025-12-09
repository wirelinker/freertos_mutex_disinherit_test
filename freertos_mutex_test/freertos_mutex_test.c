/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "freertos_mutex_test.h"


// Priorities of our threads - higher numbers are higher priority
//#define MAIN_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1UL )
#define TASK1_PRIORITY            ( tskIDLE_PRIORITY + 3UL )
#define TASK2_PRIORITY            ( tskIDLE_PRIORITY + 4UL )
#define TASK2_HIGHER_PRIORITY      ( tskIDLE_PRIORITY + 5UL )

// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE

static void task_test_1(__unused void *params);
static void task_test_2(__unused void *params);

static TaskHandle_t task1, task2;
SemaphoreHandle_t xSemaphore;
static uint32_t probe1_triggered =0;
static UBaseType_t dummy_ready_priorities = tskIDLE_PRIORITY, dummy_highest_ready_priority = tskIDLE_PRIORITY;


void probe1(UBaseType_t uxTopReadyPriority)
{
    probe1_triggered =1;
}


/*
 * should be called inside a critical section only.
 * This function manipulate ready list components.
 */
void dummy_priority_record_method1( UBaseType_t uxPriority )
{
    ( dummy_ready_priorities ) |= ( 1UL << ( uxPriority ) );
}
void dummy_priority_reset_method1( UBaseType_t uxPriority )
{
    ( dummy_ready_priorities ) &= ~( 1UL << ( uxPriority ) );
}
void dummy_highest_priority_get_method1(void)
{
    dummy_highest_ready_priority = ( 31 - __builtin_clz( (dummy_ready_priorities) ) );
}


void task_test_1(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE1, task2_priority_changed = 0;
    UBaseType_t pri = tskIDLE_PRIORITY, pri_base = tskIDLE_PRIORITY;

    xSemaphore = xSemaphoreCreateMutex();

    if( xSemaphore != NULL )
    {
        // take the mutex, then never give it back.
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
            xTaskCreate(task_test_2, "Thread2_core", MAIN_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &task2);
            printf("task1 took the mutex and created task2.\n");
        }
    }

    while(true) {


        /* if task2 tried to take mutex, then task1 will inherit the priority from task1. */
        if(pri != pri_base)
        {
            if( !task2_priority_changed )
            {
                /* raise the task2 priority.*/
                vTaskPrioritySet( task2, TASK2_HIGHER_PRIORITY );
                task2_priority_changed = 1;
                printf("task2 priority set to '5' by task1.\n");
            }
            /*
             * keep busying, do not go into block state!
             *
             * until the priority disinherit finished.
             */
        }
        else
        {
            count++;
            core_id = portGET_CORE_ID();

            /*
             * When task2 is running, task1 will not run.
             * Then task2 try to take the mutex and block itself.
             * Task1 will run and find it's own priorities changed.
             * */
            pri = uxTaskPriorityGet(task1);
            pri_base = uxTaskBasePriorityGet(task1);

            printf("task1 on core %d, pri=%d, base_pri=%d, ct=%d\n", core_id, pri, pri_base, count);

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

    }
}

void task_test_2(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE2;
    UBaseType_t task1_pri = tskIDLE_PRIORITY, task1_pri_base = tskIDLE_PRIORITY;
    UBaseType_t task2_pri = tskIDLE_PRIORITY, task2_pri_base = tskIDLE_PRIORITY;

    /* delay 500ms first, so the execution timing of task1 and task2 will interleave with each other. */
    vTaskDelay(500 / portTICK_PERIOD_MS);

    while(true) {

        count++;
        core_id = portGET_CORE_ID();
        task2_pri = uxTaskPriorityGet(task2);
        task2_pri_base = uxTaskBasePriorityGet(task2);

        printf("task2 on core %d, pri=%d, base_pri=%d, ct=%d\n", core_id, task2_pri, task2_pri_base, count);

        if(count > 5)
        {
            printf("task2 try to take the mutex.\n");
            /* take mutext and block 5s to wait. */
            if( xSemaphoreTake( xSemaphore, ( TickType_t ) 5000 / portTICK_PERIOD_MS ) == pdTRUE)
            {
                printf("task2 took the mutex successfully.\n");
            }
            else
            {
                printf("task2 couldn't take the mutex.\n");
            }
        }

        if(probe1_triggered)
        {
            printf("probe1 triggered.\n");
            task1_pri = uxTaskPriorityGet(task1);
            task1_pri_base = uxTaskBasePriorityGet(task1);
            printf("task1 not running, pri=%d, base_pri=%d\n", task1_pri, task1_pri_base);

            printf("dummy ready priorities= 0b%b\n", dummy_ready_priorities);
            printf("dummy highest ready priority= %d\n", dummy_highest_ready_priority);
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

int main( void )
{
    const char *rtos_name;

    stdio_init_all();

#if (configNUMBER_OF_CORES > 1)
    rtos_name = "FreeRTOS SMP";
    printf("Starting %s on both cores:\n", rtos_name);
#else
    (void) rtos_name;
#endif

    xTaskCreate(task_test_1, "Thread1_core", MAIN_TASK_STACK_SIZE, NULL, TASK1_PRIORITY, &task1);
    //xTaskCreate(task_test_2, "Thread2_core", MAIN_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &task2);

#if configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // task 1 on core 0, task 2 on core 1
    //vTaskCoreAffinitySet(task1, 1 << 0);
    //vTaskCoreAffinitySet(task2, 1 << 1);
#endif

    vTaskStartScheduler();

    return 0;
}
