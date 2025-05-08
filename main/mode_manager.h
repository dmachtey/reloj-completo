/* File: mode_manager.h */
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stdint.h>

/**
 * @brief  Application modes.
 */
typedef enum {
    MODE_CLOCK,
    MODE_CLOCK_SET,
    MODE_ALARM,
    MODE_ALARM_SET,
    MODE_CHRONO,
    MODE_ALARM_RING
} mode_t;

/**
 * @brief  The current global mode. Defined in mode_manager.c.
 */
extern mode_t current_mode;

/**
 * @brief  Initialize the mode manager (set default mode).
 */
void mode_manager_init(void);

/**
 * @brief  FreeRTOS task that listens for MODE (PB3) presses
 *         and cycles through the modes.
 */
void mode_manager_task(void *pvParameters);

#endif /* MODE_MANAGER_H */
