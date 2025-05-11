/* File: button_events.h */
#ifndef BUTTON_EVENTS_H
#define BUTTON_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define BUTTON_PIN_START_STOP   GPIO_NUM_35  /* PB1 */
#define BUTTON_PIN_RESET        GPIO_NUM_22  /* PB2 */
#define BUTTON_PIN_FUNC         GPIO_NUM_21  /* PB3 */

#define EV_BIT_START_STOP       (1U << 0)
#define EV_BIT_RESET            (1U << 1)
#define EV_BIT_FUNC_CHANGE      (1U << 2)
#define EV_STATE_RUNNING        (1U << 3)
#define EV_STATE_LAP            (1U << 4)

#ifdef __cplusplus
extern "C" {
#endif

extern EventGroupHandle_t xButtonEventGroup;

void button_events_init(void);
void button_events_task(void *pvParameters);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_EVENTS_H */
