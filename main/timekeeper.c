/* File: main/timekeeper.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "timekeeper.h"
#include "button_events.h"
#include "display.h"
#include "mode_manager.h"
#include <string.h>
#include "esp_log.h"


static const char *TAG = "TIMEKEEPER.C";


SemaphoreHandle_t sem_decimas = NULL;
SemaphoreHandle_t sem_laps    = NULL;
uint32_t          decimas     = 0;
uint32_t          laps[4]     = {0};

uint8_t  clk_h = 0, clk_m = 0, clk_s = 0;

void timekeeper_init(void)
{
  sem_decimas = xSemaphoreCreateMutex();
  sem_laps    = xSemaphoreCreateMutex();
}

void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
  if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE)
    {
      decimas = 0;
      clk_h   = hours;
      clk_m   = minutes;
      clk_s   = seconds;
      xSemaphoreGive(sem_decimas);

      if (current_mode == MODE_CLOCK || current_mode == MODE_CLOCK_SET) {
        ClockData_t c = { .mode=current_mode, .clk_h=clk_h, .clk_m=clk_m, .clk_s=clk_s };
        xQueueOverwrite(xClockQueue, &c);
      } else {
        ClockData_t dummy;
        xQueueReceive(xClockQueue, &dummy, 0);
      }
    }
}

void timekeeper_task(void *pvParameters)
{
  uint32_t    thents    = 0;
  TickType_t  last_wake = xTaskGetTickCount();
  uint32_t    acc_ms    = 0;
  ChronoData_t d;
  bool laps_updated = false;


  for (;;) {
    EventBits_t bits    = xEventGroupGetBits(xButtonEventGroup);
    bool        running = (bits & EV_STATE_RUNNING) != 0;
    bool        lap     = (bits & EV_STATE_LAP)     != 0;


    if (bits & EV_BIT_START_STOP){
      running = true;
      ESP_LOGI(TAG, "RUNNING.....");
    }



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
          ESP_LOGI(TAG, "Running");
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
        // fill the queue and update
        d.thents = thents;
        memcpy(d.laps, laps, sizeof(laps));
        d.mode=MODE_CHRONO;
        xQueueOverwrite(xChronoQueue, &d);

        ESP_LOGI(TAG, "Encolado de LAPS");
        //        display_request_update();
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
         // fill the queue and update
        d.thents = thents;
        memcpy(d.laps, laps, sizeof(laps));
        d.mode=MODE_CHRONO;
        xQueueOverwrite(xChronoQueue, &d);

        //        display_request_update();
      }
      if (running) {
        xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
        // fill the queue and update
        d.thents = thents;
        memcpy(d.laps, laps, sizeof(laps));
        d.mode=MODE_CHRONO;
        xQueueOverwrite(xChronoQueue, &d);

        //display_request_update();
      }
    }
    // Increment and refresh display
    if (running || lap) {
      thents++;
    }






    //  if (current_mode == MODE_CHRONO) {
    //     if (bits & EV_BIT_START_STOP) {
    //         if (!running && !lap) {
    //             xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
    //         } else if (running) {
    //             xEventGroupClearBits(xButtonEventGroup, EV_STATE_RUNNING);
    //             xEventGroupSetBits(xButtonEventGroup, EV_STATE_LAP);
    //         } else {
    //             xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP);
    //             xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
    //         }

    //         ChronoData_t d = { .mode=MODE_CHRONO };
    //         if (xSemaphoreTake(sem_decimas, portMAX_DELAY)==pdTRUE) {
    //             d.thents = thents;
    //             xSemaphoreGive(sem_decimas);
    //         }
    //         if (xSemaphoreTake(sem_laps, portMAX_DELAY)==pdTRUE) {
    //             memcpy(d.laps, laps, sizeof(laps));
    //             xSemaphoreGive(sem_laps);
    //         }
    //         xQueueOverwrite(xChronoQueue, &d);
    //     }

    //     if ((bits & EV_BIT_RESET) && lap) {
    //         if (xSemaphoreTake(sem_decimas, portMAX_DELAY)==pdTRUE) {
    //             thents=decimas=0;
    //             xSemaphoreGive(sem_decimas);
    //         }
    //         if (xSemaphoreTake(sem_laps, portMAX_DELAY)==pdTRUE) {
    //             laps[0]=laps[1]=laps[2]=laps[3]=0;
    //             xSemaphoreGive(sem_laps);
    //         }
    //         xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP | EV_BIT_RESET);

    //         ChronoData_t d = { .mode=MODE_CHRONO };
    //         if (xSemaphoreTake(sem_decimas, portMAX_DELAY)==pdTRUE) {
    //             d.thents=thents;
    //             xSemaphoreGive(sem_decimas);
    //         }
    //         if (xSemaphoreTake(sem_laps, portMAX_DELAY)==pdTRUE) {
    //             memcpy(d.laps,laps,sizeof(laps));
    //             xSemaphoreGive(sem_laps);
    //         }
    //         xQueueOverwrite(xChronoQueue, &d);
    //     }

    //     if (running) {
    //         xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
    //         ChronoData_t d = { .mode=MODE_CHRONO };
    //         if (xSemaphoreTake(sem_decimas, portMAX_DELAY)==pdTRUE) {
    //             d.thents=thents;
    //             xSemaphoreGive(sem_decimas);
    //         }
    //         if (xSemaphoreTake(sem_laps, portMAX_DELAY)==pdTRUE) {
    //             memcpy(d.laps,laps,sizeof(laps));
    //             xSemaphoreGive(sem_laps);
    //         }
    //         xQueueOverwrite(xChronoQueue, &d);
    //     }
    // }

    // if (running || lap) {
    //     thents++;
    // }






    // acc_ms += portTICK_PERIOD_MS;
    // if (acc_ms >= 10) {
    //     acc_ms = 0;
    //     last_wake += pdMS_TO_TICKS(10);

    //     clk_s++;
    //     if (clk_s>=60) { clk_s=0; clk_m++; }
    //     if (clk_m>=60) { clk_m=0; clk_h=(clk_h+1)%24; }

    //     // if (current_mode==MODE_CHRONO && running) {
    //     //     ChronoData_t d={.mode=MODE_CHRONO};
    //     //     if(xSemaphoreTake(sem_decimas,portMAX_DELAY)==pdTRUE){ d.thents=thents; xSemaphoreGive(sem_decimas); }
    //     //     if(xSemaphoreTake(sem_laps,portMAX_DELAY)==pdTRUE){ memcpy(d.laps,laps,sizeof(laps)); xSemaphoreGive(sem_laps); }
    // //             xQueueOverwrite(xChronoQueue,&d);
    // //   } else
    // if (current_mode==MODE_CLOCK||current_mode==MODE_CLOCK_SET) {
    //   ClockData_t c={.mode=current_mode,.clk_h=clk_h,.clk_m=clk_m,.clk_s=clk_s};
    //   if (!(clk_s%100))
    //     xQueueOverwrite(xClockQueue,&c);
    // }

    // }
    vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
  }
}
