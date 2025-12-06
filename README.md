# freertos_mutex_disinherit_test
Test for freertos task priority disinherit when mutex taking failed by a higher priority task.

The test code was modified from pico example : hello freertos.

The code is used to perform the test with a modified FreeRTOS kernel.

`https://github.com/wirelinker/FreeRTOS-Kernel-for-test.git` on branch
`mutex_disinherit_test`

Use `freertos_mutex_test.h` to inject the test functions into tasks.c in 
`vTaskPriorityDisinheritAfterTimeout()` function.

The specific test case and test function are placed in `freertos_mutex_test.c`.

The test case is described below:

- task1, pri=3
    - create and hold a mutex.
    - create task 2.
- task2, pri=4
    - acquire mutex and make task1 inhirent pri=4.
    - block for a time period.
- task1, pri=4
    - change task2 pri=2.
    - Keep busying, so task1 can stay in Ready list.
- task2, pri=2
    - block timeout and wake up.
    - disinhirent task1 pri and remove task1 from ReadyLists[4].
    - the pri=4 ready list is empty now.
    - At this moment, task1 hasn't been added back into Ready list.
    - So the valid highest priority of Ready lists in fact is only '2'.
- task1, pri=3
    - Now, task1 is the highest priority task.


