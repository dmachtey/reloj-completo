/* File: main/timekeeper.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "timekeeper.h"
#include "button_events.h"
#include "display.h"
#include "mode_manager.h"
#include <stdint.h>
#include <string.h>
#include "esp_log.h"

static const char *TAG = "TIMEKEEPER.C";

SemaphoreHandle_t sem_decimas = NULL;
SemaphoreHandle_t sem_laps    = NULL;
uint32_t          decimas     = 0;
uint32_t          laps[4]     = {0};

/* Variables de reloj */
uint8_t clk_h = 0, clk_m = 0, clk_s = 0;

/* Latest alarm settings queue */
QueueHandle_t    xAlarmQtoClock = NULL;  // new queue from alarm module
AlarmData_t a_set = {.al_h = 0, .al_m =0, .enable = false};

void timekeeper_init(void)
{
    sem_decimas = xSemaphoreCreateMutex();
    sem_laps    = xSemaphoreCreateMutex();


    /* Create alarm-to-clock queue */
    xAlarmQtoClock = xQueueCreate(1, sizeof(AlarmData_t));
    configASSERT(xAlarmQtoClock);
}

void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
        decimas = 0;
        clk_h   = hours;
        clk_m   = minutes;
        clk_s   = seconds;
        xSemaphoreGive(sem_decimas);

        /* Al cambiar la hora manualmente, actualiza la cola del reloj */
        if (current_mode == MODE_CLOCK || current_mode == MODE_CLOCK_SET) {
            ClockData_t c = { .mode = current_mode,
                              .clk_h = clk_h,
                              .clk_m = clk_m,
                              .clk_s = clk_s };
            xQueueOverwrite(xClockQueue, &c);
        } else {
            /* Limpia datos viejos si no es modo reloj */
            ClockData_t dummy;
            xQueueReceive(xClockQueue, &dummy, 0);
        }
    }
}

void timekeeper_task(void *pvParameters)
{
    uint32_t     thents       = 0;
    TickType_t   last_wake    = xTaskGetTickCount();
    uint32_t     acc_ms       = 0;
    ChronoData_t d;
    bool         laps_updated = false;

    for (;;) {
        EventBits_t bits    = xEventGroupGetBits(xButtonEventGroup);
        bool        running = (bits & EV_STATE_RUNNING) != 0;
        bool        lap     = (bits & EV_STATE_LAP)     != 0;


        bits    = xEventGroupGetBits(xButtonEventGroup);
        if (bits & EV_BIT_RESET)
          ESP_LOGI(TAG, "RESET PRESSED");


        if (current_mode == MODE_CLOCK_SET) {
            if (bits & EV_BIT_START_STOP) {
                /* Incrementar campo actual */
                if (clk_set_sequence == CLK_SEQ_HR) {
                    clk_h = (clk_h + 1) % 24;
                } else if (clk_set_sequence == CLK_SEQ_MIN) {
                    clk_m = (clk_m + 1) % 60;
                }
                /* Aplicar cambio */
                timekeeper_set_clock(clk_h, clk_m, clk_s);
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
            }
            if (bits & EV_BIT_RESET) {
                 ESP_LOGI(TAG, "RESET PRESSED Alternar secuenciador");
                /* Alternar secuenciador de edición */
                clk_set_sequence = (clk_set_sequence == CLK_SEQ_HR)
                                   ? CLK_SEQ_MIN : CLK_SEQ_HR;
                ESP_LOGI(TAG, "CLK_SET -> %d", clk_set_sequence);
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        // if (bits & EV_BIT_START_STOP) {
        //     running = true;
        //     ESP_LOGI(TAG, "RUNNING.....");
        // }

        /* --- MODO CRONÓMETRO (sin cambios) --- */
        if (current_mode == MODE_CHRONO) {
            bits    = xEventGroupGetBits(xButtonEventGroup);
            running = (bits & EV_STATE_RUNNING) != 0;
            lap     = (bits & EV_STATE_LAP)     != 0;

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
                d.thents = thents;
                memcpy(d.laps, laps, sizeof(laps));
                d.mode   = MODE_CHRONO;
                xQueueOverwrite(xChronoQueue, &d);
            }

            if (!lap) {
                if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
                    decimas = thents;
                    xSemaphoreGive(sem_decimas);
                }
            }

            if (lap && !laps_updated) {
                if (xSemaphoreTake(sem_laps, portMAX_DELAY) == pdTRUE) {
                    laps[3] = laps[2];
                    laps[2] = laps[1];
                    laps[1] = laps[0];
                    laps[0] = decimas;
                    xSemaphoreGive(sem_laps);
                    laps_updated = true;
                }
            }

            if ((bits & EV_BIT_RESET) && lap) {
                if (xSemaphoreTake(sem_decimas, portMAX_DELAY) == pdTRUE) {
                    thents = decimas = 0;
                    xSemaphoreGive(sem_decimas);
                }
                if (xSemaphoreTake(sem_laps, portMAX_DELAY) == pdTRUE) {
                    laps[0] = laps[1] = laps[2] = laps[3] = 0;
                    xSemaphoreGive(sem_laps);
                }
                ESP_LOGI(TAG, "Chrono reset");
                xEventGroupClearBits(xButtonEventGroup, EV_STATE_LAP | EV_BIT_RESET);
                d.thents = thents;
                memcpy(d.laps, laps, sizeof(laps));
                d.mode   = MODE_CHRONO;
                xQueueOverwrite(xChronoQueue, &d);
            }

            if (running) {
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
                d.thents = thents;
                memcpy(d.laps, laps, sizeof(laps));
                d.mode   = MODE_CHRONO;
                xQueueOverwrite(xChronoQueue, &d);
            }
        }

        /* Incrementa el contador de décimas sólo si corre o hay lap */
        if (running || lap) {
            thents++;
        }

        /* --- MODO RELOJ: cada 1000 ms actualiza hh:mm:ss y encola --- */
        acc_ms += 10;  // este bucle corre cada 10 ms con vTaskDelayUntil
        if (acc_ms >= 1000) {
            acc_ms -= 1000;
            clk_s++;
            if (clk_s >= 60) {
                clk_s = 0;
                clk_m++;
            }
            if (clk_m >= 60) {
                clk_m = 0;
                clk_h = (clk_h + 1) % 24;
            }
            if (current_mode == MODE_CLOCK || current_mode == MODE_CLOCK_SET) {
                ClockData_t c = { .mode = current_mode,
                                  .clk_h = clk_h,
                                  .clk_m = clk_m,
                                  .clk_s = clk_s };
                xQueueOverwrite(xClockQueue, &c);
            } else {
                /* Limpia valores si no estamos en reloj */
                ClockData_t dummy;
                xQueueReceive(xClockQueue, &dummy, 0);
            }
        }


        /* Read alarm settings if updated */
        if (xQueueReceive(xAlarmQtoClock, &a_set, 0) == pdTRUE) {
           ESP_LOGI(TAG, "Alarm updated: %02d:%02d enable=%d", a_set.al_h, a_set.al_m, a_set.enable);
        }

        /* Alarm Snooze 5 min */
        if ((current_mode == MODE_ALARM_RING) && (bits & EV_BIT_RESET)) {
          xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
          // Add 5 minutes to the current time (in total minutes)
          uint32_t total_min = a_set.al_h * 60 + a_set.al_m + 5;

          // Wrap around within a 24-hour period (24*60 = 1440 minutes)
          total_min %= 24 * 60;

          // Convert back to hours and minutes
          a_set.al_h = total_min / 60;    // yields a value in 0–23
          a_set.al_m = total_min % 60;    // yields a value in 0–59
          current_mode = MODE_CLOCK;
          ESP_LOGI(TAG, "Snooze pressed");
        }

          /* Check alarm trigger: if current time >= alarm time */
        if ((a_set.enable) &&
            (clk_h == a_set.al_h && clk_m == a_set.al_m) )
          current_mode = MODE_ALARM_RING;



        /* Mantiene periodo de 10 ms */
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));
    }
}
