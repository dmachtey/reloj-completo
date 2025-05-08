/* File: display.c */

#include "display.h"
#include "display_config.h"
#include "ili9341.h"
#include "digitos.h"
#include "fonts.h"
#include "timekeeper.h"
#include "alarm.h"
#include "button_events.h"
#include "mode_manager.h"      /* <— ensures mode_t, current_mode, MODE_* */

#include "freertos/semphr.h"
#include "freertos/event_groups.h"

#include <stdio.h>
#include <stdint.h>

/* Static panels — allocated once, not on the task stack */
static panel_t p_min;
static panel_t p_sec;
static panel_t p_dec;

/* Queue handle for receiving mode_t updates */
static QueueHandle_t xDisplayQueue = NULL;

void display_init(void)
{
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    /* Create the big-digit panels only once */
    p_min = CrearPanel( 30, 60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    p_sec = CrearPanel(170, 60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    p_dec = CrearPanel(310, 60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    /* Create a length-1 queue to pass mode changes */
    xDisplayQueue = xQueueCreate(1, sizeof(mode_t));
    /* Kick-off the first draw */
    display_request_update();
}

void display_request_update(void)
{
    if (xDisplayQueue != NULL) {
        mode_t m = current_mode;
        xQueueOverwrite(xDisplayQueue, &m);
    }
}

void display_task(void *pvParameters)
{
    mode_t m;
    char   buf[32];

    for (;;) {
        /* Block until a new mode arrives */
        if (xDisplayQueue != NULL &&
            xQueueReceive(xDisplayQueue, &m, portMAX_DELAY) == pdTRUE)
        {
            /* 1) Clear the screen */
            ILI9341Fill(DIGITO_FONDO);

            /* 2) Draw the mode label at top-right */
            const char *mode_str = "";
            switch (m) {
            case MODE_CLOCK:      mode_str = "CLOCK";    break;
            case MODE_CLOCK_SET:  mode_str = "CLK-SET";  break;
            case MODE_ALARM:      mode_str = " ALARM";   break;
            case MODE_ALARM_SET:  mode_str = "AL-SET";   break;
            case MODE_CHRONO:     mode_str = "CHRONO";   break;
            case MODE_ALARM_RING: mode_str = "!RING!";   break;
            }
            ILI9341DrawString(350, 10,
                              (char*)mode_str, &font_7x10,
                              ILI9341_WHITE, DIGITO_FONDO);

            /* 3) Main display content based on mode */
            if (m == MODE_CLOCK || m == MODE_CLOCK_SET) {
                /* Real-time clock HH:MM:SS */
                snprintf(buf, sizeof(buf), "%02u:%02u:%02u",
                         clk_h, clk_m, clk_s);
                ILI9341DrawString(80, 200,
                                  buf, &font_16x26,
                                  ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_ALARM || m == MODE_ALARM_SET) {
                /* Alarm time HH:MM */
                snprintf(buf, sizeof(buf), "%02u:%02u",
                         al_h, al_m);
                ILI9341DrawString(100, 200,
                                  buf, &font_16x26,
                                  ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_CHRONO) {
                /* Stopwatch: draw minutes, seconds, tenths as big digits */
                uint32_t tot;
                xSemaphoreTake(sem_decimas, portMAX_DELAY);
                  tot = decimas;
                xSemaphoreGive(sem_decimas);

                uint32_t mi = tot / 6000;
                uint32_t se = (tot / 100) % 60;
                uint32_t de = tot % 100;

                DibujarDigito(p_min, 0, mi / 10);
                DibujarDigito(p_min, 1, mi % 10);
                DibujarDigito(p_sec, 0, se / 10);
                DibujarDigito(p_sec, 1, se % 10);
                DibujarDigito(p_dec, 0, de / 10);
                DibujarDigito(p_dec, 1, de % 10);

            } else if (m == MODE_ALARM_RING) {
                /* Alarm ringing message */
                ILI9341DrawString(100, 200,
                                  "ALARM!",
                                  &font_16x26, ILI9341_RED, DIGITO_FONDO);
            }

            /* 4) Bottom fixed legends */
            ILI9341DrawString( 10, 300, "PB3: MODE",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140, 300, "PB2:INC",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280, 300, "PB1:OK/SEL",
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
        }
    }
}
