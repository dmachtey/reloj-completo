/* File: alarm.h */
#ifndef ALARM_H
#define ALARM_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize alarm internal state */
void alarm_init(void);
/* Returns true if alarm is enabled */
bool alarm_enabled(void);
/* Set alarm time HH:MM */
void alarm_set(uint8_t hours, uint8_t minutes);
/* Task that checks & handles ring/snooze/stop */
void alarm_task(void *pvParameters);

/* Public alarm time */
extern uint8_t al_h;
extern uint8_t al_m;

#endif /* ALARM_H */
