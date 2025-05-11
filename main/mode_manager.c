/* File: main/mode_manager.c */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mode_manager.h"
#include "button_events.h"
#include "esp_log.h"

static const char *TAG = "MODE_MGR";

/** @brief Helper to convert app_mode_t to a human-readable string */
static const char* mode_to_string(app_mode_t mode)
{
    switch (mode) {
        case MODE_CLOCK:      return "MODE_CLOCK";
        case MODE_CLOCK_SET:  return "MODE_CLOCK_SET";
        case MODE_ALARM:      return "MODE_ALARM";
        case MODE_ALARM_SET:  return "MODE_ALARM_SET";
        case MODE_CHRONO:     return "MODE_CHRONO";
        case MODE_ALARM_RING: return "MODE_ALARM_RING";
        default:              return "UNKNOWN_MODE";
    }
}

app_mode_t current_mode = MODE_CLOCK;

void mode_manager_init(void)
{
    current_mode = MODE_CLOCK;
    ESP_LOGI(TAG, "Mode manager initialized, starting in %s", mode_to_string(current_mode));
}

void mode_manager_task(void *pvParameters)
{
    for (;;)
    {
        /* Espera a que se pulse PB3 (modo) */
        EventBits_t bits = xEventGroupWaitBits(
            xButtonEventGroup,
            EV_BIT_FUNC_CHANGE,
            pdTRUE,    // limpia el bit al salir
            pdFALSE,
            portMAX_DELAY
        );

        if (bits & EV_BIT_FUNC_CHANGE) {
            ESP_LOGI(TAG, "BT3 (MODE) pressed, current_mode = %s", mode_to_string(current_mode));

            /* Cicla el modo */
            switch (current_mode)
            {
                case MODE_CLOCK:      current_mode = MODE_CLOCK_SET; break;
                case MODE_CLOCK_SET:  current_mode = MODE_ALARM;     break;
                case MODE_ALARM:      current_mode = MODE_ALARM_SET; break;
                case MODE_ALARM_SET:  current_mode = MODE_CHRONO;    break;
                case MODE_CHRONO:     current_mode = MODE_CLOCK;     break;
                case MODE_ALARM_RING: /* Permanece hasta que se “stop” o “snooze” */ break;
                default:              current_mode = MODE_CLOCK;     break;
            }

            ESP_LOGI(TAG, "Switched to %s", mode_to_string(current_mode));
        }

        /* Pequeña demora para ceder CPU */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
