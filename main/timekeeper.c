/* File: timekeeper.c */
#include "timekeeper.h"
#include "display.h"
#include "mode_manager.h"
#include "button_events.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

SemaphoreHandle_t sem_decimas = NULL;
uint32_t decimas = 0;
uint8_t clk_h = 0, clk_m = 0, clk_s = 0;

void timekeeper_init(void)
{
    sem_decimas = xSemaphoreCreateMutex();
}

void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
    {
        decimas = 0;
        clk_h = hours;
        clk_m = minutes;
        clk_s = seconds;
        xSemaphoreGive(sem_decimas);
        display_request_update();
    }
}

void timekeeper_task(void *pvParameters)
{
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t acc = 0;

    for (;;)
    {
        /* Handle start/stop on PB1 (EV_BIT_START_STOP) */
        EventBits_t bits = xEventGroupGetBits(xButtonEventGroup);
        if (bits & EV_BIT_START_STOP)
        {
            if (bits & EV_STATE_RUNNING)
                xEventGroupClearBits(xButtonEventGroup, EV_STATE_RUNNING);
            else
                xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
            xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
            display_request_update();
        }

        /* Handle reset on PB2: only in chrono mode when paused */
        if (current_mode == MODE_CHRONO &&
            (bits & EV_BIT_RESET) &&
            !(bits & EV_STATE_RUNNING))
        {
            if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
            {
                decimas = 0;
                xSemaphoreGive(sem_decimas);
            }
            xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
            display_request_update();
        }

        /* Chrono: increment decimas every 10 ms only if running */
        if (current_mode == MODE_CHRONO &&
            (bits & EV_STATE_RUNNING))
        {
            if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
            {
                decimas++;
                xSemaphoreGive(sem_decimas);
                display_request_update();
            }
        }

        /* Maintain 1-second ticks for clock and update */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
        acc += 10;
        if (acc >= 1000)
        {
            acc -= 1000;
            clk_s++;
            if (clk_s >= 60)
            {
                clk_s = 0;
                clk_m++;
            }
            if (clk_m >= 60)
            {
                clk_m = 0;
                clk_h = (clk_h + 1) % 24;
            }
            display_request_update();
        }
    }
}
