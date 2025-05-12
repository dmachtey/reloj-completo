/* File: button_events.c */
#include "button_events.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define AUTO_REPEAT_INITIAL_DELAY   pdMS_TO_TICKS(500)  // medio segundo antes de empezar
#define AUTO_REPEAT_PERIOD          pdMS_TO_TICKS(200)  // repite cada 200 ms

static const char *TAG = "BTN_EVT";

EventGroupHandle_t xButtonEventGroup = NULL;

void button_events_init(void)
{
    xButtonEventGroup = xEventGroupCreate();
    if (xButtonEventGroup == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
    }

    gpio_set_direction(BUTTON_PIN_START_STOP, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_START_STOP);

    gpio_set_direction(BUTTON_PIN_RESET, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_RESET);

    gpio_set_direction(BUTTON_PIN_FUNC, GPIO_MODE_INPUT);
    gpio_pullup_en(BUTTON_PIN_FUNC);

    ESP_LOGI(TAG, "Button event group initialized");
}


void button_events_task(void *pvParameters)
{
    TickType_t lastWake = xTaskGetTickCount();
    bool      pb1_prev = true, pb2_prev = true;
    TickType_t pressTime1 = 0, pressTime2 = 0;
    int  prev_func = 1;

    for (;;) {
        // Lee el GPIO (activo bajo)
        bool pb1 = (gpio_get_level(BUTTON_PIN_START_STOP) == 0);
        bool pb2 = (gpio_get_level(BUTTON_PIN_RESET) == 0);
        int cur_func  = gpio_get_level(BUTTON_PIN_FUNC);

        // PB1: START/STOP
        if (pb1 && !pb1_prev) {
            // Nuevo flanco de bajada: envío único
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_START_STOP);
            pressTime1 = lastWake;
        }
        else if (pb1 && pb1_prev) {
            // Si lo dejamos pulsado y superó el initial delay, repite
            if ((lastWake - pressTime1) >= AUTO_REPEAT_INITIAL_DELAY) {
                // ¿Es momento de repetir?
                static TickType_t nextRepeat1 = 0;
                if (lastWake >= nextRepeat1) {
                    xEventGroupSetBits(xButtonEventGroup, EV_BIT_START_STOP);
                    nextRepeat1 = lastWake + AUTO_REPEAT_PERIOD;
                }
            }
        }

        // PB2: RESET
        if (pb2 && !pb2_prev) {
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_RESET);
            pressTime2 = lastWake;
        }
        else if (pb2 && pb2_prev) {
            if ((lastWake - pressTime2) >= AUTO_REPEAT_INITIAL_DELAY) {
                static TickType_t nextRepeat2 = 0;
                if (lastWake >= nextRepeat2) {
                    xEventGroupSetBits(xButtonEventGroup, EV_BIT_RESET);
                    nextRepeat2 = lastWake + AUTO_REPEAT_PERIOD;
                }
            }
        }


        if (cur_func == 0 && prev_func == 1) {
          ESP_LOGI(TAG, "MODE (FUNC) button pressed");
          xEventGroupSetBits(xButtonEventGroup, EV_BIT_FUNC_CHANGE);
        }


        pb1_prev = pb1;
        pb2_prev = pb2;
        prev_func = cur_func;

        // Mantén el muestreo periódico
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(10));
    }
}
