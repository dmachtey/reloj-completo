/* File: timekeeper.c */
#include "timekeeper.h"
#include "display.h"
#include "mode_manager.h"
#include "freertos/task.h"

SemaphoreHandle_t sem_decimas = NULL;
uint32_t decimas = 0;
uint8_t clk_h = 0, clk_m = 0, clk_s = 0;

void timekeeper_init(void)
{
    sem_decimas = xSemaphoreCreateMutex();
}

void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    clk_h = hours;
    clk_m = minutes;
    clk_s = seconds;
    display_request_update();
}

void timekeeper_task(void *pvParameters)
{
    TickType_t last_wake = xTaskGetTickCount();
    int64_t acc = 0;
    for (;;) {
        /* Chrono: 10 ms ticks */
        if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
            if (current_mode == MODE_CHRONO) decimas++;
            xSemaphoreGive(sem_decimas);
        }
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
        acc += 10;
        if (acc >= 1000) {
            acc -= 1000;
            clk_s++;
            if (clk_s >= 60) { clk_s = 0; clk_m++; }
            if (clk_m >= 60) { clk_m = 0; clk_h = (clk_h + 1) % 24; }
            display_request_update();
        }
    }
}
