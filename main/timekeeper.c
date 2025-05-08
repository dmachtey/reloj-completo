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
uint32_t decimas = 0;


// Clock time variables
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
  static uint32_t thents = 0;
  TickType_t last_wake = xTaskGetTickCount();
  uint32_t acc_ms = 0;

  for (;;)
    {
      // Read current event bits
      EventBits_t bits = xEventGroupGetBits(xButtonEventGroup);
      bool running = (bits & EV_STATE_RUNNING) != 0;
      bool lap = (bits & EV_STATE_LAP) != 0;


      // MODE_CHORONO
      if (current_mode == MODE_CHRONO)
        {
          // Refresh local state
          bits = xEventGroupGetBits(xButtonEventGroup);
          running = (bits & EV_STATE_RUNNING) != 0;
          lap = (bits & EV_STATE_LAP) != 0;

          // Handle PB1: cycle STOPPED->RUNNING->LAP->RUNNING
          if (bits & EV_BIT_START_STOP)
            {
              if (!running && !lap)
                {
                  // STOPPED -> RUNNING
                  xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
                }
              else if (running)
                {
                  // RUNNING -> LAP
                  xEventGroupClearBits(xButtonEventGroup, EV_STATE_RUNNING);
                  xEventGroupSetBits(xButtonEventGroup, EV_STATE_LAP);
                  }
              else if (lap)
                {
                  // LAP -> RUNNING
                  xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP);
                  xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
                }
              xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
              display_request_update();

            }

          if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
            {
              decimas = thents;
              xSemaphoreGive(sem_decimas);
            }

          // Update display continuously when running
          if (running)
            {
              xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
              display_request_update();
            }
          if (running || lap)
            {
              // increase the counter
              thents++;
            }

          // Reset on PB2 only when in LAP
          if ((bits & EV_BIT_RESET) && lap)
            {
              if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
                {
                  thents = 0;
                  decimas = 0;
                  xSemaphoreGive(sem_decimas);
                }
              // Go to STOPPED state
              xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP);
              xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
              display_request_update();
            }
        }
      // End MODE_CHRONO


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
