/* File: main/display.h */
#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "mode_manager.h"

/**
 * @brief Message format for Chrono mode.
 */
typedef struct {
  app_mode_t mode;       /**< Current application mode */
  uint32_t thents;       /**< Tenths of seconds count */
  uint32_t laps[4];      /**< Lap times in tenths */
} ChronoData_t;

/**
 * @brief Message format for Clock mode.
 */
typedef struct {
  app_mode_t mode;       /**< Current application mode */
  uint8_t clk_h;         /**< Hours */
  uint8_t clk_m;         /**< Minutes */
  uint8_t clk_s;         /**< Seconds */
} ClockData_t;

/**
 * @brief Message format for Alarm mode.
 */
typedef struct {
  app_mode_t mode;       /**< Current application mode */
  uint8_t al_h;          /**< Alarm hours */
  uint8_t al_m;          /**< Alarm minutes */
  bool enable;          /**< Alarm enabled flag */
} AlarmData_t;

/* Queues used by producer tasks to send data to the display task */
extern QueueHandle_t     xChronoQueue;
extern QueueHandle_t     xClockQueue;
extern QueueHandle_t     xAlarmQueue;

/* Queue set used by the display task to select from multiple queues */
extern QueueSetHandle_t  xDisplayQueueSet;

/**
 * @brief  Initialize display hardware and create display queues.
 */
void display_init(void);

/**
 * @brief  Main display task: waits on queues and updates the screen.
 * @param  pvParameters  Task parameters (unused)
 */
void display_task(void *pvParameters);

#endif /* DISPLAY_H */
