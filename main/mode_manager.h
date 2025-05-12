/* File: main/mode_manager.h */
#ifndef MODE_MANAGER_H
#define MODE_MANAGER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/**
 * @brief Application modes.
 */
typedef enum {
    MODE_CLOCK,
    MODE_CLOCK_SET,
    MODE_ALARM,
    MODE_ALARM_SET,
    MODE_CHRONO,
    MODE_ALARM_RING
} app_mode_t;

/**
 * @brief Sequencer for MODE_CLOCK_SET: IDLE → MIN → HR.
 */
typedef enum {
    CLK_SEQ_IDLE = 0,
    CLK_SEQ_MIN,
    CLK_SEQ_HR
} clk_set_seq_t;

/**
 * @brief Sequencer for MODE_ALARM_SET: IDLE → MIN → HR → EN.
 */
typedef enum {
    ALARM_SEQ_IDLE = 0,
    ALARM_SEQ_MIN,
    ALARM_SEQ_HR,
    ALARM_SEQ_EN
} alarm_set_seq_t;

/** Global current mode (defined in mode_manager.c) */
extern app_mode_t      current_mode;
/** Sequencer state for MODE_CLOCK_SET */
extern clk_set_seq_t   clk_set_sequence;
/** Sequencer state for MODE_ALARM_SET */
extern alarm_set_seq_t alarm_set_sequence;

void mode_manager_init(void);
void mode_manager_task(void *pvParameters);

#endif /* MODE_MANAGER_H */
