/* File: alarm.c */
#include "alarm.h"
#include "timekeeper.h"
#include "button_events.h"
#include "mode_manager.h"
#include "display.h"

uint8_t al_h = 0, al_m = 0;
static bool enabled = false, ringing = false;

bool alarm_enabled(void)
{
    return enabled;
}

void alarm_set(uint8_t hours, uint8_t minutes)
{
    al_h = hours;
    al_m = minutes;
    enabled = true;
    display_request_update();
}

void alarm_task(void *pvParameters)
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if (!ringing && enabled && clk_h == al_h && clk_m == al_m && clk_s == 0) {
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
                ringing = false; current_mode = MODE_CLOCK;
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
                display_request_update();
            }
        }
    }
}
