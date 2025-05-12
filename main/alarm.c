/* File: main/alarm.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "alarm.h"
#include "display.h"
#include "button_events.h"
#include "mode_manager.h"
#include "timekeeper.h"
#include "esp_log.h"

extern QueueHandle_t xAlarmQueue; // defined in display.c
extern QueueHandle_t xAlarmQtoClock;  // new queue for timekeeper module


static const char * TAG = "ALARM.C";

/* Alarm state */
bool enabled = false;
uint8_t al_h = 0;
uint8_t al_m = 0;

/**
 * @brief Initialize alarm module.
 */
void alarm_init(void) {
    enabled = false;
    al_h = 0;
    al_m = 0;
    alarm_set_sequence = ALARM_SEQ_IDLE;

    /* Ensure alarm queue exists */
    if (xAlarmQueue == NULL) {
        xAlarmQueue = xQueueCreate(1, sizeof(AlarmData_t));
    }

    /* Ensure clock queue exists */
    if (!xAlarmQtoClock) {
        xAlarmQtoClock = xQueueCreate(1, sizeof(AlarmData_t));
    }
    /* Publish initial alarm settings */
    AlarmData_t a = {.mode = MODE_ALARM_SET, .al_h = al_h, .al_m = al_m, .enable = enabled};
    if (xAlarmQueue) {
        xQueueOverwrite(xAlarmQueue, &a);
    }
    if (xAlarmQtoClock) {
        xQueueOverwrite(xAlarmQtoClock, &a);
    }
    ESP_LOGI(TAG, "Alarm init: %02d:%02d, enable=%d", al_h, al_m, enabled);
}

/**
 * @brief Update alarm parameters and publish to display.
 */
static void alarm_set_params(uint8_t hours, uint8_t minutes, bool en) {
    al_h = hours;
    al_m = minutes;
    enabled = en;

    AlarmData_t a = {.mode = current_mode, .al_h = al_h, .al_m = al_m, .enable = enabled};
    if (xAlarmQueue) {
        xQueueOverwrite(xAlarmQueue, &a);
    }
     /* Publish to clock queue */
    if (xAlarmQtoClock) {
        xQueueOverwrite(xAlarmQtoClock, &a);
    }
    ESP_LOGI(TAG, "Alarm set -> %02d:%02d, enable=%d", al_h, al_m, enabled);
}

/**
 * @brief Alarm service task: handles setting and normal alarm operation.
 */
void alarm_task(void * pvParameters) {
    EventBits_t bits;

    for (;;) {
        bits = xEventGroupGetBits(xButtonEventGroup);

        /* --- MODE_ALARM_SET: adjust hours, minutes, enable flag --- */
        if (current_mode == MODE_ALARM_SET) {
            /* Start/Stop button: increment or toggle enable */
            if (bits & EV_BIT_START_STOP) {
                switch (alarm_set_sequence) {
                case ALARM_SEQ_MIN:
                    al_m = (al_m + 1) % 60;
                    break;
                case ALARM_SEQ_HR:
                    al_h = (al_h + 1) % 24;
                    break;
                case ALARM_SEQ_EN:
                    enabled = !enabled;
                    break;
                default:
                    break;
                }
                alarm_set_params(al_h, al_m, enabled);
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_START_STOP);
            }

            /* Reset button: advance sequencer */
            if (bits & EV_BIT_RESET) {
                alarm_set_sequence = (alarm_set_sequence + 1) % 4;
                xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
            }

            /* Debounce / pacing */
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        /* --- MODE_ALARM: publish current alarm settings --- */
        if (current_mode == MODE_ALARM) {
            AlarmData_t a = {.mode = MODE_ALARM, .al_h = al_h, .al_m = al_m, .enable = enabled};
            if (xAlarmQueue) {
                xQueueOverwrite(xAlarmQueue, &a);
            }
        }

        /* --- MODE_ALARM_RING: handle ringing, snooze/stop elsewhere --- */
        if ((current_mode == MODE_ALARM_RING)
            && (bits & EV_BIT_START_STOP)) {
            enabled = false;
            AlarmData_t a = {.mode = MODE_ALARM, .al_h = al_h, .al_m = al_m, .enable = enabled};
            if (xAlarmQueue) {
              xQueueOverwrite(xAlarmQueue, &a);
            }
            current_mode = MODE_CLOCK;

        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
