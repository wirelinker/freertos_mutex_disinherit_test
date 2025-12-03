
#ifndef INC_FREERTOS_MUTEX_TEST_H
#define INC_FREERTOS_MUTEX_TEST_H

void probe1(UBaseType_t uxTopReadyPriority);
void custom_priority_reset_method1( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority );
void custom_priority_reset_method2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority );
void custom_priority_reset_method1_2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority );
void custom_priority_reset_method2_2( UBaseType_t uxPriority, UBaseType_t uxTopReadyPriority );

/* for test only */
#define portREMOVE_STATIC_QUALIFIER

#endif /* INC_FREERTOS_H */

