/* File: main/timekeeper.h */
#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H

#include <stdint.h>
#include "display.h"    /* ChronoData_t, ClockData_t, AlarmData_t */
#include "freertos/FreeRTOS.h"

void timekeeper_init(void);
void timekeeper_set_clock(uint8_t hours, uint8_t minutes, uint8_t seconds);
void timekeeper_task(void *pvParameters);
extern QueueHandle_t xAlarmQtoClock;

#endif /* TIMEKEEPER_H */
