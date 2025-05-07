/* File: main.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button_events.h"
#include "mode_manager.h"
#include "timekeeper.h"
#include "alarm.h"
#include "display.h"

void app_main(void)
{
    button_events_init();
    timekeeper_init();
    mode_manager_init();
    display_init();

    xTaskCreate(button_events_task, "BtnEvt", 1024, NULL, 2, NULL);
    xTaskCreate(mode_manager_task,  "Mode", 1024, NULL, 3, NULL);
    xTaskCreate(timekeeper_task,     "Time", 1024, NULL, 4, NULL);
    xTaskCreate(alarm_task,          "Alarm",1024, NULL, 3, NULL);
    xTaskCreate(display_task,        "Disp", 1024, NULL, 2, NULL);
}
