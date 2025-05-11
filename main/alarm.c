/* File: main/alarm.c */
#include "alarm.h"
#include "mode_manager.h"
#include "button_events.h"
#include "display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

uint8_t al_h = 0, al_m = 0;
static bool enabled = false, ringing = false;

void alarm_init(void)
{
    enabled = false;
    ringing = false;
}

bool alarm_enabled(void)
{
    return enabled;
}

void alarm_set(uint8_t hours, uint8_t minutes)
{
    al_h = hours;
    al_m = minutes;
    if (current_mode == MODE_ALARM || current_mode == MODE_ALARM_SET)
    {
        AlarmData_t a = { .mode = current_mode, .al_h = al_h, .al_m = al_m };
        xQueueOverwrite(xAlarmQueue, &a);
    }
    else
    {
        AlarmData_t d;
        xQueueReceive(xAlarmQueue, &d, 0);
    }
}

void alarm_task(void *pvParameters)
{
    for (;;)
    {
        // EventBits_t bits = xEventGroupWaitBits(
        //     xButtonEventGroup,
        //     EV_BIT_START_STOP | EV_BIT_RESET,
        //     pdTRUE,
        //     pdFALSE,
        //     portMAX_DELAY
        // );
        // if (bits & EV_BIT_START_STOP)
        // {
        //     enabled = !enabled;
        //     ringing = false;
        //     if (!enabled && current_mode == MODE_ALARM_RING)
        //         current_mode = MODE_CLOCK;
        //     AlarmData_t a = { .mode = current_mode, .al_h = al_h, .al_m = al_m };
        //     xQueueOverwrite(xAlarmQueue, &a);
        // }
        // else if (bits & EV_BIT_RESET)
        // {
        //     int total = al_h * 60 + al_m + 5;
        //     al_h = (total / 60) % 24;
        //     al_m = total % 60;
        //     ringing = false;
        //     current_mode = MODE_CLOCK;
        //     AlarmData_t a = { .mode = current_mode, .al_h = al_h, .al_m = al_m };
        //     xQueueOverwrite(xAlarmQueue, &a);
        // }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
