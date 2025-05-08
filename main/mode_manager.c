/* File: mode_manager.c */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "mode_manager.h"    /* for mode_t, MODE_* */
#include "button_events.h"   /* for EV_BIT_FUNC_CHANGE, xButtonEventGroup */
#include "display.h"         /* for display_request_update() */

/* Start in CLOCK mode */
mode_t current_mode = MODE_CLOCK;

void mode_manager_init(void)
{
    /* Set the default working mode */
    current_mode = MODE_CLOCK;
    display_request_update();
}

void mode_manager_task(void *pvParameters)
{
    const EventBits_t waitBits = EV_BIT_FUNC_CHANGE;

    for (;;)
    {
        /* Wait indefinitely for the MODE button press (PB3) */
        xEventGroupWaitBits(
            xButtonEventGroup,
            waitBits,
            pdTRUE,     /* clear bit on exit */
            pdFALSE,    /* wait for any */
            portMAX_DELAY
        );

        /* Cycle through modes: CLOCK → CLOCK_SET → ALARM → ALARM_SET → CHRONO → CLOCK … */
        switch (current_mode)
        {
            case MODE_CLOCK:
                current_mode = MODE_CLOCK_SET;
                break;
            case MODE_CLOCK_SET:
                current_mode = MODE_ALARM;
                break;
            case MODE_ALARM:
                current_mode = MODE_ALARM_SET;
                break;
            case MODE_ALARM_SET:
                current_mode = MODE_CHRONO;
                break;
            case MODE_CHRONO:
                current_mode = MODE_CLOCK;
                break;
            case MODE_ALARM_RING:
                /* If currently in ALARM_RING, stay until dismissed in alarm_task */
                break;
        }

        /* Request the display to redraw with the new mode */
        display_request_update();
    }
}
