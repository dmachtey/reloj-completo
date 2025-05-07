/* File: main.c */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button_events.h"
#include "mode_manager.h"
#include "timekeeper.h"
#include "alarm.h"
#include "display.h"

/* Stack sizes in words */
#define BTN_STACK_SIZE    2048
#define MODE_STACK_SIZE   2048
#define TIME_STACK_SIZE   2048
#define ALARM_STACK_SIZE  2048
#define DISP_STACK_SIZE   4096

static StaticTask_t xBtnTCB;   static StackType_t xBtnStack[BTN_STACK_SIZE];
static StaticTask_t xModeTCB;  static StackType_t xModeStack[MODE_STACK_SIZE];
static StaticTask_t xTimeTCB;  static StackType_t xTimeStack[TIME_STACK_SIZE];
static StaticTask_t xAlarmTCB; static StackType_t xAlarmStack[ALARM_STACK_SIZE];
static StaticTask_t xDispTCB;  static StackType_t xDispStack[DISP_STACK_SIZE];

void app_main(void)
{
    /* Initialize display first so its queue is ready */
    display_init();

    /* Now initialize other modules that may request updates */
    button_events_init();
    timekeeper_init();
    mode_manager_init();
    alarm_init();

    /* Create all tasks statically */
    xTaskCreateStatic(button_events_task, "BtnEvt", BTN_STACK_SIZE, NULL, 2, xBtnStack,   &xBtnTCB);
    xTaskCreateStatic(mode_manager_task,  "Mode",  MODE_STACK_SIZE, NULL, 3, xModeStack, &xModeTCB);
    xTaskCreateStatic(timekeeper_task,     "Time",  TIME_STACK_SIZE, NULL, 4, xTimeStack, &xTimeTCB);
    xTaskCreateStatic(alarm_task,          "Alarm", ALARM_STACK_SIZE,NULL, 3, xAlarmStack,&xAlarmTCB);
    xTaskCreateStatic(display_task,        "Disp",  DISP_STACK_SIZE, NULL, 2, xDispStack, &xDispTCB);
}
