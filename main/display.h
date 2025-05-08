/* File: display.h */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "mode_manager.h"    /* for mode_t, current_mode, MODE_* */

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/**
 * @brief   Initialize the display hardware, create digit panels and the mode-queue.
 */
void display_init(void);

/**
 * @brief   Trigger a screen redraw with the current_mode.
 */
void display_request_update(void);

/**
 * @brief   The FreeRTOS task that waits for mode updates and redraws the LCD.
 */
void display_task(void *pvParameters);

#endif /* DISPLAY_H */
