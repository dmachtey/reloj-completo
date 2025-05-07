/* File: alarm.c */

#include "alarm.h"
#include "timekeeper.h"
#include "button_events.h"
#include "mode_manager.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

/* public alarm time */
uint8_t al_h = 0;
uint8_t al_m = 0;

/* internal state */
static bool enabled = false;
static bool ringing = false;

void alarm_init(void)
{
    enabled = false;
    ringing = false;
    display_request_update();
}

bool alarm_enabled(void)
{
    return enabled;
}

void alarm_set(uint8_t hours, uint8_t minutes)
{
    al_h = hours % 24;
    al_m = minutes % 60;
    enabled = true;
    display_request_update();
}

void alarm_task(void *pvParameters)
{
    const TickType_t delay = pdMS_TO_TICKS(100);

    for (;;) {
        vTaskDelay(delay);

        /* trigger at HH:MM:00 */
        if (!ringing && enabled &&
            clk_h == al_h && clk_m == al_m && clk_s == 0)
        {
            ringing = true;
            current_mode = MODE_ALARM_RING;
            display_request_update();
        }

        if (ringing) {
            EventBits_t bits = xEventGroupGetBits(xButtonEventGroup);
            if (bits & EV_BIT_START_STOP) {
                enabled = false; ringing = false;
                current_mode = MODE_CLOCK;
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
                display_request_update();
            } else if (bits & EV_BIT_RESET) {
                int total = al_h * 60 + al_m + 5;
                al_h = (total / 60) % 24;
                al_m = total % 60;
                ringing = false;
                current_mode = MODE_CLOCK;
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
                display_request_update();
            }
        }
    }
}
