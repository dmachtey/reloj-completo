/* File: button_events.h */
#ifndef BUTTON_EVENTS_H
#define BUTTON_EVENTS_H

#include "freertos/event_groups.h"
#include <stdint.h>

/* pines de botones definidos en your_pin_definitions.h */
#define BUTTON_PIN_START_STOP  GPIO_NUM_35  /* PB1 */
#define BUTTON_PIN_RESET       GPIO_NUM_22  /* PB2 */
#define BUTTON_PIN_MODE        GPIO_NUM_21  /* PB3 */

/* Event Bits para pulsaciones */
#define EV_BIT_START_STOP      (1 << 0)
#define EV_BIT_RESET           (1 << 1)
#define EV_BIT_FUNC_CHANGE     (1 << 2)

/* Bit persistente de estado de cronómetro corriendo */
#define EV_STATE_RUNNING       (1 << 3)

extern EventGroupHandle_t xButtonEventGroup;

/* Inicialización GPIO + EventGroup */
void ButtonEvents_Init(void);
/* Tarea que detecta flancos e inserta EV_BITs y EV_STATE_RUNNING */
void ButtonEvents_Task(void *pvParameters);

#endif /* BUTTON_EVENTS_H */
