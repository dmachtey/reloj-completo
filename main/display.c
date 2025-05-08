/* File: display.c */

#include "display.h"
#include "display_config.h"
#include "ili9341.h"
#include "digitos.h"
#include "fonts.h"
#include "timekeeper.h"
#include "alarm.h"
#include "button_events.h"
#include "mode_manager.h"

#include <stdio.h>
#include <stdint.h>
#include "freertos/semphr.h"
#include "freertos/queue.h"

/*---------------------------------------------------------------------------*/
/* Static panels for minutes, seconds, and tenths (2 digits each)           */
/*---------------------------------------------------------------------------*/
static panel_t panel_minutes;
static panel_t panel_seconds;
static panel_t panel_tenths;

/*---------------------------------------------------------------------------*/
/* Queue to receive mode changes (mode_t)                                    */
/*---------------------------------------------------------------------------*/
static QueueHandle_t displayModeQueue = NULL;

/*---------------------------------------------------------------------------*/
/* Initialize the display: start the ILI9341, create digit panels, and kick  */
/* off the first draw                                                        */
/*---------------------------------------------------------------------------*/
void display_init(void)
{
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    /* create three 2-digit panels */
    panel_minutes = CrearPanel(
        30,  60, 2,
        DIGITO_ALTO, DIGITO_ANCHO,
        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    panel_seconds = CrearPanel(
       170,  60, 2,
        DIGITO_ALTO, DIGITO_ANCHO,
        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    panel_tenths = CrearPanel(
       310,  60, 2,
        DIGITO_ALTO, DIGITO_ANCHO,
        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    /* create a 1-item queue carrying mode_t values */
    displayModeQueue = xQueueCreate(1, sizeof(mode_t));

    /* request the very first draw */
    display_request_update();
}

/*---------------------------------------------------------------------------*/
/* Ask the display task to redraw using the current_mode global              */
/*---------------------------------------------------------------------------*/
void display_request_update(void)
{
    if (displayModeQueue) {
        mode_t m = current_mode;
        xQueueOverwrite(displayModeQueue, &m);
    }
}

static mode_t old_m = MODE_CLOCK;
/*---------------------------------------------------------------------------*/
/* The display task: waits for mode updates and redraws the screen           */
/*---------------------------------------------------------------------------*/
void display_task(void *pvParameters)
{
    mode_t m;

    char   buf[32];

    for (;;) {
        if (displayModeQueue &&
            xQueueReceive(displayModeQueue, &m, portMAX_DELAY) == pdTRUE)
        {
          /* 1) clear the entire screen if mode has changed */
          if (m != old_m)
                   ILI9341Fill(DIGITO_FONDO);
          old_m = m;

          /* 2) draw the mode indicator in the top-right corner */
            const char *mode_str = "";
            switch (m) {
            case MODE_CLOCK:      mode_str = "CLOCK";    break;
            case MODE_CLOCK_SET:  mode_str = "CLK-SET";  break;
            case MODE_ALARM:      mode_str = "ALARM";    break;
            case MODE_ALARM_SET:  mode_str = "AL-SET";   break;
            case MODE_CHRONO:     mode_str = "CHRONO";   break;
            case MODE_ALARM_RING: mode_str = "!RING!";   break;
            }

            ILI9341DrawString(
                350, 10,
                (char*)mode_str, &font_7x10,
                ILI9341_WHITE, DIGITO_FONDO);

            /* 3) draw the main area depending on mode */
            if (m == MODE_CLOCK || m == MODE_CLOCK_SET) {
                /* HH:MM:SS */
                snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                         clk_h, clk_m, clk_s);
                ILI9341DrawString(
                    80, 200,
                    buf, &font_16x26,
                    ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_ALARM || m == MODE_ALARM_SET) {
                /* HH:MM */
                snprintf(buf, sizeof(buf), "%02u:%02u",
                         al_h, al_m);
                ILI9341DrawString(
                    100, 200,
                    buf, &font_16x26,
                    ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_CHRONO) {
                uint32_t tot;

                /* safely grab the current tenths count */
                xSemaphoreTake(sem_decimas, portMAX_DELAY);
                  tot = decimas;
                xSemaphoreGive(sem_decimas);

                uint32_t mins = tot / 6000;            /* 6000 tenths per minute */
                uint32_t secs = (tot / 100) % 60;      /* 100 tenths per second */
                uint32_t tenth = tot % 100;            /* residual tenths */

                /* draw minutes */
                DibujarDigito(panel_minutes, 0, mins / 10);
                DibujarDigito(panel_minutes, 1, mins % 10);

                /* draw colon separator as two filled circles */
                {
                    uint16_t circleColor = DIGITO_ENCENDIDO;
                    ILI9341DrawFilledCircle(150, 110, 5, circleColor);
                    ILI9341DrawFilledCircle(150, 150, 5, circleColor);
                }

                /* draw seconds */
                DibujarDigito(panel_seconds, 0, secs / 10);
                DibujarDigito(panel_seconds, 1, secs % 10);

                /* draw colon separator between seconds and tenths */
                {
                    uint16_t circleColor = DIGITO_ENCENDIDO;
                    ILI9341DrawFilledCircle(290, 110, 5, circleColor);
                    ILI9341DrawFilledCircle(290, 150, 5, circleColor);
                }

                /* draw tenths */
                DibujarDigito(panel_tenths, 0, tenth / 10);
                DibujarDigito(panel_tenths, 1, tenth % 10);

            } else if (m == MODE_ALARM_RING) {
                /* Alarm ringing message */
                ILI9341DrawString(
                    100, 200,
                    "ALARM!",
                    &font_16x26, ILI9341_RED, DIGITO_FONDO);
            }

            /* 4) always show bottom button legends (left→right: MODE, RESET, START) */
            ILI9341DrawString( 10, 300, "PB3: MODE",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140, 300, "PB2: RESET",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280, 300, "PB1: START",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
        }
    }
}
