/* File: mode_manager.h */
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include "freertos/FreeRTOS.h"

typedef enum {
    MODE_CLOCK,
    MODE_CLOCK_SET,
    MODE_ALARM,
    MODE_ALARM_SET,
    MODE_CHRONO,
    MODE_ALARM_RING
} mode_t;

extern mode_t current_mode;

void mode_manager_init(void);
void mode_manager_task(void *pvParameters);

#endif /* MODE_MANAGER_H */