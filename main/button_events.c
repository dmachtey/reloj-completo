/* File: button_events.c */
#include "button_events.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

EventGroupHandle_t xButtonEventGroup = NULL;

void ButtonEvents_Init(void)
{
    /* crear el grupo antes de usarlo */
    xButtonEventGroup = xEventGroupCreate();

    /* configurar pines */
    gpio_set_direction(BUTTON_PIN_START_STOP, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_START_STOP);
    gpio_set_direction(BUTTON_PIN_RESET, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_RESET);
    gpio_set_direction(BUTTON_PIN_MODE, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_MODE);
}

void ButtonEvents_Task(void *pvParameters)
{
    int prev1 = 1, prev2 = 1, prev3 = 1;
    int running = 0;

    for (;;) {
        int cur1 = gpio_get_level(BUTTON_PIN_START_STOP);
        int cur2 = gpio_get_level(BUTTON_PIN_RESET);
        int cur3 = gpio_get_level(BUTTON_PIN_MODE);

        /* START/STOP (PB1) */
        if (prev1 == 1 && cur1 == 0) {
            /* informar evento pulsación */
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_START_STOP);
            /* togglear estado running */
            running = !running;
            if (running) {
                xEventGroupSetBits(xButtonEventGroup, EV_STATE_RUNNING);
            } else {
                xEventGroupClearBits(xButtonEventGroup, EV_STATE_RUNNING);
            }
        }
        prev1 = cur1;

        /* RESET (PB2) */
        if (prev2 == 1 && cur2 == 0) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_RESET);
        }
        prev2 = cur2;

        /* MODE (PB3) */
        if (prev3 == 1 && cur3 == 0) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_FUNC_CHANGE);
        }
        prev3 = cur3;

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
