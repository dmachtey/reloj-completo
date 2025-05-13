/* File: main/main.c */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "button_events.h"
#include "display.h"
#include "timekeeper.h"
#include "mode_manager.h"
#include "alarm.h"

// Definiciones de pila y TCB para cada tarea
#define STACK_SIZE_DISPLAY    4096
#define STACK_SIZE_TIMEKEEPER 4096
#define STACK_SIZE_MODE       2048
#define STACK_SIZE_ALARM      2048
#define STACK_SIZE_BTN_EVTS   2048

static StaticTask_t tcb_display;
static StackType_t stack_display[STACK_SIZE_DISPLAY];

static StaticTask_t tcb_timekeeper;
static StackType_t stack_timekeeper[STACK_SIZE_TIMEKEEPER];

static StaticTask_t tcb_mode;
static StackType_t stack_mode[STACK_SIZE_MODE];

static StaticTask_t tcb_alarm;
static StackType_t stack_alarm[STACK_SIZE_ALARM];

static StaticTask_t tcb_btn;
static StackType_t stack_btn[STACK_SIZE_BTN_EVTS];

void app_main(void) {
    // 1) Inicializar el event-group de botones y arrancar su tarea
    button_events_init();
    xTaskCreateStatic(button_events_task, "btn_evts", STACK_SIZE_BTN_EVTS, NULL, 2, stack_btn, &tcb_btn);

    // 2) Inicializar módulos
    mode_manager_init();
    timekeeper_init();
    alarm_init();
    display_init();

    // 3) Crear tareas estáticas en el orden deseado
    xTaskCreateStatic(display_task, "display", STACK_SIZE_DISPLAY, NULL, 1, stack_display, &tcb_display);

    xTaskCreateStatic(timekeeper_task, "timekeeper", STACK_SIZE_TIMEKEEPER, NULL, 2, stack_timekeeper, &tcb_timekeeper);

    xTaskCreateStatic(mode_manager_task, "mode_manager", STACK_SIZE_MODE, NULL, 1, stack_mode, &tcb_mode);

    xTaskCreateStatic(alarm_task, "alarm", STACK_SIZE_ALARM, NULL, 1, stack_alarm, &tcb_alarm);
}
