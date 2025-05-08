/* File: timekeeper.h */
#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "button_events.h"

extern SemaphoreHandle_t sem_decimas;    /* ← protegen decimas  */
extern SemaphoreHandle_t sem_laps;  /* ← protegen parciales */

extern uint32_t decimas;                 /* ← decenas de segundo      */
extern uint32_t laps[4];            /* ← históricos de laps      */

extern uint8_t clk_h;    /* hora actual          */
extern uint8_t clk_m;    /* minuto actual        */
extern uint8_t clk_s;    /* segundo actual       */

void timekeeper_init(void);
void timekeeper_task(void *pvParameters);
void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds);

#endif /* TIMEKEEPER_H */
