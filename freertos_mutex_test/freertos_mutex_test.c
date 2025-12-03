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
#define TASK2_LOWER_PRIORITY      ( tskIDLE_PRIORITY + 2UL )

// Stack sizes of our threads in words (4 bytes)
#define MAIN_TASK_STACK_SIZE configMINIMAL_STACK_SIZE


extern List_t pxReadyTasksLists[  ]; /**< Prioritised ready tasks. */

static void task_test_1(__unused void *params); 
static void task_test_2(__unused void *params);

static TaskHandle_t task1, task2;
SemaphoreHandle_t xSemaphore;
static uint32_t probe1_triggered =0;
static uint32_t method1_flag = 0, method2_flag = 0;
static uint32_t method1_2_flag = 0, method2_2_flag = 0;
static UBaseType_t method1_top_priority = tskIDLE_PRIORITY, method2_top_priority = tskIDLE_PRIORITY; 
static UBaseType_t method1_2_top_priority = tskIDLE_PRIORITY, method2_2_top_priority = tskIDLE_PRIORITY; 
static UBaseType_t current_top_priority = tskIDLE_PRIORITY;

void probe1(UBaseType_t uxTopReadyPriority)
{
    probe1_triggered =1;
    current_top_priority = uxTopReadyPriority;
}


/*
 * should be called inside a critical section only.
 * This function is going to manipulate Ready lists.
 */
void custom_priority_reset_method1( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority )
{
    method1_flag = 1;

    if(uxPriority >= uxTopReadyPriority)
    {
        /* The highest priority Ready list is empty now.
         * Scan all Ready lists. */
        UBaseType_t x;
        for( x = configMAX_PRIORITIES; x > tskIDLE_PRIORITY; x-- )
        {
            if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ x ] ) ) == pdFALSE )
            {
               method1_top_priority = x; 
            }
        }
    }
    else
    {
        /* do nothing. 
         * Because there are still tasks in the higher priority Ready list. */
        method1_top_priority = uxTopReadyPriority;
    }
}

void custom_priority_reset_method1_2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority )
{
    method1_2_flag = 1;

    if(uxPriority >= uxTopReadyPriority)
    {
        /* The highest priority Ready list is empty now.
         * Scan all Ready lists. */
        UBaseType_t x;
        for( x = configMAX_PRIORITIES; x > tskIDLE_PRIORITY; x-- )
        {
            if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ x ] ) ) == pdFALSE )
            {
               method1_2_top_priority = x; 
            }
        }
    }
    else
    {
        /* do nothing. 
         * Because there are still tasks in the higher priority Ready list. */
        method1_2_top_priority = uxTopReadyPriority;
    }
}


/*
 * should be called inside a critical section only.
 * This function is going to manipulate Ready lists.
 */
void custom_priority_reset_method2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority )
{
    method2_flag = 1;

   /* scan all Ready list anyway. */
   UBaseType_t x;
   for( x = configMAX_PRIORITIES; x > tskIDLE_PRIORITY; x-- )
   {
       if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ x ] ) ) == pdFALSE )
       {
          method2_top_priority = x; 
       }
   }
}

void custom_priority_reset_method2_2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority )
{
    method2_2_flag = 1;

   /* scan all Ready list anyway. */
   UBaseType_t x;
   for( x = configMAX_PRIORITIES; x > tskIDLE_PRIORITY; x-- )
   {
       if( listLIST_IS_EMPTY( &( pxReadyTasksLists[ x ] ) ) == pdFALSE )
       {
          method2_2_top_priority = x; 
       }
   }
}

void task_test_1(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE1;
    UBaseType_t pri = tskIDLE_PRIORITY, pri_base = tskIDLE_PRIORITY;

    xSemaphore = xSemaphoreCreateMutex();

    if( xSemaphore != NULL )
    {
        // take the mutex, then never give it back.
        if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
        {
            xTaskCreate(task_test_2, "Thread2_core", MAIN_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &task2);
            vTaskCoreAffinitySet(task2, 1 << 1);
            printf("task1 took the mutex and created task2.\n");
        }
    }

    while(true) {

        // if task2 tried to take mutex, then task1 will inherit the priority from task1.
        if(pri != pri_base)
        {
            // lower down the task2 priority.
            vTaskPrioritySet( task2, TASK2_LOWER_PRIORITY );

            /*
             * keep busying, do not go into block state!
             * 
             * until the priority disinherit finished.
             */

            if(method1_flag)
            {
                printf("method1 triggered.\n");
                pri = uxTaskPriorityGet(task1); 
                pri_base = uxTaskBasePriorityGet(task1);
            }

        }
        else
        {
            count++;
            core_id = portGET_CORE_ID();
            pri = uxTaskPriorityGet(task1); 
            pri_base = uxTaskBasePriorityGet(task1);

            printf("task1 on core %d, pri=%d, base_pri=%d, ct=%d\n", core_id, pri, pri_base, count);

            if(probe1_triggered)
            {
                printf("probe1 triggered. current top pri=%d\n", current_top_priority);
            }

            if(method1_flag)
            {
                printf("method1 triggered. top pri=%d\n", method1_top_priority);
            }

            if(method2_flag)
            {
                printf("method2 triggered. top pri=%d\n", method2_top_priority);
            }

            if(method1_2_flag)
            {
                printf("method1_2 triggered. top pri=%d\n", method1_2_top_priority);
            }

            if(method2_2_flag)
            {
                printf("method2_2 triggered. top pri=%d\n", method2_2_top_priority);
            }

            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }

    }
}

void task_test_2(__unused void *params) {

    uint32_t count = 0, core_id = 0xDEADBEE2;
    UBaseType_t pri = tskIDLE_PRIORITY, pri_base = tskIDLE_PRIORITY;

    // delay 500ms first, so the execution timing of task1 and task2 will interleave with each other.
    vTaskDelay(500 / portTICK_PERIOD_MS);

    while(true) {

        count++;
        core_id = portGET_CORE_ID();
        pri = uxTaskPriorityGet(task2); 
        pri_base = uxTaskBasePriorityGet(task2);

        printf("task2 on core %d, pri=%d, base_pri=%d, ct=%d\n", core_id, pri, pri_base, count);

        if(count > 10) 
        {
            printf("task2 try to take the mutex.\n");
            // take mutext and block 5s to wait. 
            if( xSemaphoreTake( xSemaphore, ( TickType_t ) 5000 / portTICK_PERIOD_MS ) == pdTRUE)
            {
                printf("task2 took the mutex successfully.\n");
            }
            else
            {
                printf("task2 couldn't take the mutex.\n");
            }
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
#endif

    xTaskCreate(task_test_1, "Thread1_core", MAIN_TASK_STACK_SIZE, NULL, TASK1_PRIORITY, &task1);
    //xTaskCreate(task_test_2, "Thread2_core", MAIN_TASK_STACK_SIZE, NULL, TASK2_PRIORITY, &task2);

#if configUSE_CORE_AFFINITY && configNUMBER_OF_CORES > 1
    // task 1 on core 0, task 2 on core 1
    vTaskCoreAffinitySet(task1, 1 << 0);
    //vTaskCoreAffinitySet(task2, 1 << 1);
#endif

    vTaskStartScheduler();

    return 0;
}
