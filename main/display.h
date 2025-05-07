/* File: display.h */
#ifndef DISPLAY_H
#define DISPLAY_H

#include "mode_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

void display_init(void);
void display_request_update(void);
void display_task(void *pvParameters);

#endif /* DISPLAY_H */