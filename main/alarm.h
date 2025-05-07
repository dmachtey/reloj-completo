/* File: alarm.h */
#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include <stdbool.h>


extern uint8_t al_h, al_m;

bool alarm_enabled(void);
void alarm_set(uint8_t hours, uint8_t minutes);
void alarm_task(void *pvParameters);

#endif /* ALARM_H */