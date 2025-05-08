/* File: timekeeper.c */
#include "timekeeper.h"
#include "display.h"
#include "mode_manager.h"
#include "button_events.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#ifndef EV_STATE_LAP
#define EV_STATE_LAP        (1U << 4)
#endif

// Semaphore and counter for tenths of seconds
SemaphoreHandle_t sem_decimas = NULL;
SemaphoreHandle_t sem_laps = NULL;
uint32_t decimas = 0;
uint32_t laps[4];

// Clock time variables
uint8_t clk_h = 0, clk_m = 0, clk_s = 0;

void timekeeper_init(void)
{
  sem_decimas = xSemaphoreCreateMutex();
  sem_laps = xSemaphoreCreateMutex();
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
  static uint32_t thents = 0;
  static bool laps_updated = false;
  TickType_t last_wake = xTaskGetTickCount();
  uint32_t acc_ms = 0;

  for (;;)
    {
      // Read current event bits
      EventBits_t bits = xEventGroupGetBits(xButtonEventGroup);
      bool running = (bits & EV_STATE_RUNNING) != 0;
      bool lap = (bits & EV_STATE_LAP) != 0;

      // MODE_CHRONO
      if (current_mode == MODE_CHRONO) {
        bits = xEventGroupGetBits(xButtonEventGroup);
        running = bits & EV_STATE_RUNNING;
        lap = bits & EV_STATE_LAP;

        // PB1: Cycle STOPPED->RUNNING->LAP->RUNNING
        if (bits & EV_BIT_START_STOP) {
          if (!running && !lap) {
            xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
            laps_updated = false;
          } else if (running) {
            xEventGroupClearBits(xButtonEventGroup, EV_STATE_RUNNING);
            xEventGroupSetBits(xButtonEventGroup, EV_STATE_LAP);
            laps_updated = false;
          } else if (lap) {
            xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP);
            xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);

            if (!laps_updated) {
              if (xSemaphoreTake(sem_laps, portMAX_DELAY) == pdTRUE) {
                laps[3] = laps[2];
                laps[2] = laps[1];
                laps[1] = laps[0];
                laps[0] = decimas;
                xSemaphoreGive(sem_laps);
                laps_updated = true;
              }
            }

          }
          xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
          display_request_update();
        }

        // Update decimas safely
        if (!lap)
          if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
            decimas = thents;
            xSemaphoreGive(sem_decimas);
          }

        if (lap & !laps_updated) {
          if (xSemaphoreTake(sem_laps, portMAX_DELAY) == pdTRUE) {
            laps[3] = laps[2];
            laps[2] = laps[1];
            laps[1] = laps[0];
            laps[0] = decimas;
            xSemaphoreGive(sem_laps);
            laps_updated = true;
          }
        }

        // Reset with PB2 only in LAP state
        if ((bits & EV_BIT_RESET) && lap) {
          if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
            thents = decimas = 0;
            xSemaphoreGive(sem_decimas);
          }

          if (xSemaphoreTake(sem_laps, portMAX_DELAY) == pdTRUE) {
            laps[3] = 0;
            laps[2] = 0;
            laps[1] = 0;
            laps[0] = 0;
            xSemaphoreGive(sem_laps);
          }

          xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP | EV_BIT_RESET);
          display_request_update();
        }
        if (running) {
          xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
          display_request_update();
        }
      }
      // Increment and refresh display
      if (running || lap) {
        thents++;
      }



      // Delay 10 ms and handle clock every 1 sec
      vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
      acc_ms += 10;

      if (acc_ms >= 1000)
        {
          acc_ms -= 1000;
          // Advance clock
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
          // Update display for clock or chrono when running
          if (current_mode != MODE_CHRONO || running)
            {
              display_request_update();
            }
        }
    }
}
