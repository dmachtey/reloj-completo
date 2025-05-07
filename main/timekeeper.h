/* File: timekeeper.h */
#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t sem_decimas;
extern uint32_t decimas;
extern uint8_t clk_h, clk_m, clk_s;

void timekeeper_init(void);
void timekeeper_task(void *pvParameters);
void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds);

#endif /* TIMEKEEPER_H */