/* File: main/mode_manager.h */
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stdint.h>

/**
 * @brief  Application modes (avoids sys/types mode_t conflict).
 */
typedef enum {
    MODE_CLOCK,
    MODE_CLOCK_SET,
    MODE_ALARM,
    MODE_ALARM_SET,
    MODE_CHRONO,
    MODE_ALARM_RING
} app_mode_t;

/** Global current mode (defined in mode_manager.c) */
extern app_mode_t current_mode;

void mode_manager_init(void);
void mode_manager_task(void *pvParameters);

#endif /* MODE_MANAGER_H */
