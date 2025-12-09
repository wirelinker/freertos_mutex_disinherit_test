# freertos_mutex_disinherit_test
Test for freertos task priority disinherit when mutex taking failed by a higher priority task.

The test code was modified from pico example : hello freertos.

The code is used to perform the test with a modified FreeRTOS kernel.

`https://github.com/wirelinker/FreeRTOS-Kernel-for-test.git` on branch
`mutex_disinherit_test`

Use `freertos_mutex_test.h` to inject the test functions into tasks.c.

The specific test case and test functions are placed in `freertos_mutex_test.c`.

# The test case is described below:
- only use single core.

- task1, pri=3
    - create and hold a mutex.
    - create task 2.

- task2, pri=4,
    - acquire mutex and make task1 inhirent pri=4.
    - block for a time period.

- task1, pri=4
    - change task2 pri=5.
    - Keep busying, so task1 can stay in Ready list.

- task2, pri=5
    - block timeout and wake up.
    - disinherit task1 priority back to '3' and remove task1 from pxReadyTasksLists[4].
    - the pxReadyTasksLists[4] is empty now.

    - portRESET_READY_PRIORITY() will clear priority '3' bit from uxTopReadyPriority.

    - At this moment, task1 hasn't been added back into Ready list.
    - So the valid highest priority of pxReadyTasksLists is only '5'.

    - Add task1 back to pxReadyTasksLists. The priority bit '3' be added back to uxTopReadyPriority.

The result would be:The pxReadyTasksLists[4] is empty. And the priority bit '4' will remain in uxTopReadyPriority. The next time, the scheduler may select a next task from the xListEnd.


