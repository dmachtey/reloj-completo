/* File: button_events.c */
#include "button_events.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"

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
    int prev_start = 1, prev_reset = 1, prev_func = 1;

    for (;;)
    {
        int cur_start = gpio_get_level(BUTTON_PIN_START_STOP);
        int cur_reset = gpio_get_level(BUTTON_PIN_RESET);
        int cur_func  = gpio_get_level(BUTTON_PIN_FUNC);

        if (cur_start == 0 && prev_start == 1) {
            ESP_LOGI(TAG, "START/STOP button pressed");
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_START_STOP);
        }

        if (cur_reset == 0 && prev_reset == 1) {
            ESP_LOGI(TAG, "RESET button pressed");
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_RESET);
        }

        if (cur_func == 0 && prev_func == 1) {
            ESP_LOGI(TAG, "MODE (FUNC) button pressed");
            xEventGroupSetBits(xButtonEventGroup, EV_BIT_FUNC_CHANGE);
        }

        prev_start = cur_start;
        prev_reset = cur_reset;
        prev_func  = cur_func;

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
