/* File: main/mode_manager.c */
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "mode_manager.h"
#include "button_events.h"
#include "esp_log.h"

static const char * TAG = "MODE_MGR";

/** Convert app_mode_t → string */
static const char * mode_to_string(app_mode_t mode) {
    switch (mode) {
    case MODE_CLOCK:
        return "MODE_CLOCK";
    case MODE_CLOCK_SET:
        return "MODE_CLOCK_SET";
    case MODE_ALARM:
        return "MODE_ALARM";
    case MODE_ALARM_SET:
        return "MODE_ALARM_SET";
    case MODE_CHRONO:
        return "MODE_CHRONO";
    case MODE_ALARM_RING:
        return "MODE_ALARM_RING";
    default:
        return "UNKNOWN_MODE";
    }
}

/** Convert alarm_set_seq_t → string */
static const char * alarm_seq_to_string(alarm_set_seq_t seq) {
    switch (seq) {
    case ALARM_SEQ_IDLE:
        return "ALRM_SEQ_IDLE";
    case ALARM_SEQ_MIN:
        return "ALRM_SEQ_MIN";
    case ALARM_SEQ_HR:
        return "ALRM_SEQ_HR";
    case ALARM_SEQ_EN:
        return "ALRM_SEQ_EN";
    default:
        return "UNKNOWN_ALRM_SEQ";
    }
}

/** Definitions of the globals declared in the header */
app_mode_t current_mode = MODE_CLOCK;
clk_set_seq_t clk_set_sequence = CLK_SEQ_IDLE;
alarm_set_seq_t alarm_set_sequence = ALARM_SEQ_IDLE;

void mode_manager_init(void) {
    current_mode = MODE_CLOCK;
    clk_set_sequence = CLK_SEQ_IDLE;
    alarm_set_sequence = ALARM_SEQ_IDLE;
    ESP_LOGI(TAG, "Mode manager initialized, starting in %s", mode_to_string(current_mode));
}

void mode_manager_task(void * pvParameters) {
    for (;;) {
        /* Espera PB3 (cambia modo) y PB2 (avanza secuenciador) */
        EventBits_t bits = xEventGroupWaitBits(xButtonEventGroup,
                                               EV_BIT_FUNC_CHANGE  /* PB3 */
                                                   | EV_BIT_RESET, /* PB2 */
                                               pdFALSE,            /* limpia los bits al salir */
                                               pdFALSE, portMAX_DELAY);

        /* --- Cambio de modo principal (PB3) --- */
        if (bits & EV_BIT_FUNC_CHANGE) {
            xEventGroupClearBits(xButtonEventGroup, EV_BIT_FUNC_CHANGE);
            ESP_LOGI(TAG, "PB3 pressed, current_mode = %s", mode_to_string(current_mode));
            switch (current_mode) {
            case MODE_CLOCK:
                current_mode = MODE_CLOCK_SET;
                break;
            case MODE_CLOCK_SET:
                current_mode = MODE_ALARM;
                break;
            case MODE_ALARM:
                current_mode = MODE_ALARM_SET;
                break;
            case MODE_ALARM_SET:
                current_mode = MODE_CHRONO;
                break;
            case MODE_CHRONO:
                current_mode = MODE_CLOCK;
                break;
            case MODE_ALARM_RING: /* permanece hasta snooze/stop */
                break;
            default:
                current_mode = MODE_CLOCK;
                break;
            }
            /* Al entrar en SET-modos, reiniciar siempre el secuenciador */
            if (current_mode == MODE_CLOCK_SET) {
                clk_set_sequence = CLK_SEQ_IDLE;
                ESP_LOGI(TAG, "CLK_SET sequencer reset to IDLE");
            }
            if (current_mode == MODE_ALARM_SET) {
                alarm_set_sequence = ALARM_SEQ_IDLE;
                ESP_LOGI(TAG, "ALRM_SET sequencer reset to IDLE");
            }
            ESP_LOGI(TAG, "Switched to %s", mode_to_string(current_mode));
        }

        /* --- Avanza secuenciador secundario (PB2) sólo en SET-modos --- */
        if (bits & EV_BIT_RESET) {
            xEventGroupClearBits(xButtonEventGroup, EV_BIT_RESET);
            if (current_mode == MODE_CLOCK_SET) {
                clk_set_sequence = (clk_set_sequence + 1) % 3;
                ESP_LOGI(TAG, "CLK_SET sequencer -> %d", clk_set_sequence);

            } else if (current_mode == MODE_ALARM_SET) {
                alarm_set_sequence = (alarm_set_sequence + 1) % 4;
                ESP_LOGI(TAG, "ALRM_SET sequencer -> %s", alarm_seq_to_string(alarm_set_sequence));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
