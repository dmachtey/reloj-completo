/* File: mode_manager.c */
#include "mode_manager.h"
#include "button_events.h"
#include "display.h"

mode_t current_mode = MODE_CLOCK;
static mode_t prev_mode = MODE_CLOCK;

void mode_manager_init(void) {}

void mode_manager_task(void *pvParameters)
{
    for (;;) {
        xEventGroupWaitBits(
            xButtonEventGroup,
            EV_BIT_FUNC_CHANGE,
            pdTRUE,
            pdFALSE,
            portMAX_DELAY
        );
        prev_mode = current_mode;
        switch (current_mode) {
        case MODE_CLOCK:
            current_mode = MODE_CLOCK_SET; break;
        case MODE_CLOCK_SET:
            current_mode = MODE_ALARM; break;
        case MODE_ALARM:
            current_mode = MODE_ALARM_SET; break;
        case MODE_ALARM_SET:
            current_mode = MODE_CHRONO; break;
        case MODE_CHRONO:
            current_mode = MODE_CLOCK; break;
        case MODE_ALARM_RING:
            /* stay until stopped */
            current_mode = current_mode;
            break;
        }
        display_request_update();
    }
}
