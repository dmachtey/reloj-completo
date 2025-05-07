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
#include <stdbool.h>
#include "freertos/event_groups.h"

static QueueHandle_t xDisplayQueue = NULL;
static panel_t p_min, p_sec, p_dec;

void display_init(void)
{
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    /* inicializo los paneles */
    p_min = CrearPanel( 30,  60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    p_sec = CrearPanel(170,  60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    p_dec = CrearPanel(310,  60, 2,
                        DIGITO_ALTO, DIGITO_ANCHO,
                        DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    xDisplayQueue = xQueueCreate(1, sizeof(mode_t));
    display_request_update();
}

void display_request_update(void)
{
    mode_t m = current_mode;
    xQueueOverwrite(xDisplayQueue, &m);
}

void display_task(void *pvParameters)
{
    mode_t m;
    char buf[32];

    for (;;) {
        if (xQueueReceive(xDisplayQueue, &m, portMAX_DELAY) == pdTRUE) {
            /* limpiar */
            ILI9341Fill(DIGITO_FONDO);

            /* modo superior derecha */
            const char *mode_str = "";
            switch (m) {
                case MODE_CLOCK:      mode_str = "CLOCK";    break;
                case MODE_CLOCK_SET:  mode_str = "CLK-SET";  break;
                case MODE_ALARM:      mode_str = " ALARM";   break;
                case MODE_ALARM_SET:  mode_str = "AL-SET";   break;
                case MODE_CHRONO:     mode_str = "CHRONO";   break;
                case MODE_ALARM_RING: mode_str = "!RING!";   break;
            }
            ILI9341DrawString(350, 10, (char*)mode_str,
                              &font_7x10, ILI9341_WHITE, DIGITO_FONDO);

            /* área principal */
            if (m == MODE_CLOCK || m == MODE_CLOCK_SET) {
                snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
                         (unsigned long)clk_h,
                         (unsigned long)clk_m,
                         (unsigned long)clk_s);
                ILI9341DrawString(80, 200, buf,
                                  &font_16x26, ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_ALARM || m == MODE_ALARM_SET) {
                snprintf(buf, sizeof(buf), "%02lu:%02lu",
                         (unsigned long)al_h,
                         (unsigned long)al_m);
                ILI9341DrawString(100, 200, buf,
                                  &font_16x26, ILI9341_WHITE, DIGITO_FONDO);

            } else if (m == MODE_CHRONO) {
                uint32_t tot;
                xSemaphoreTake(sem_decimas, portMAX_DELAY);
                  tot = decimas;
                xSemaphoreGive(sem_decimas);

                uint32_t mi = tot / 6000;
                uint32_t se = (tot / 100) % 60;
                uint32_t de = tot % 100;

                /* dígitos grandes */
                DibujarDigito(p_min, 0, mi/10);
                DibujarDigito(p_min, 1, mi%10);
                DibujarDigito(p_sec, 0, se/10);
                DibujarDigito(p_sec, 1, se%10);
                DibujarDigito(p_dec, 0, de/10);
                DibujarDigito(p_dec, 1, de%10);

                /* parciales */
                uint32_t loc[3] = {0};
                xSemaphoreTake(sem_parciales, portMAX_DELAY);
                  loc[0] = parciales[0];
                  loc[1] = parciales[1];
                  loc[2] = parciales[2];
                xSemaphoreGive(sem_parciales);

                for (int i = 0; i < 3; i++) {
                    uint32_t pm = (loc[i]/6000)%100;
                    uint32_t ps = (loc[i]/100)%60;
                    uint32_t pd = loc[i]%100;
                    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu",
                             (unsigned long)pm,
                             (unsigned long)ps,
                             (unsigned long)pd);
                    ILI9341DrawString(30, 180 + 36*i, buf,
                                      &font_11x18, ILI9341_WHITE, DIGITO_FONDO);
                }

            } else if (m == MODE_ALARM_RING) {
                ILI9341DrawString(100, 200, "ALARM!",
                                  &font_16x26, ILI9341_RED, DIGITO_FONDO);
            }

            /* leyendas inferiores (BT3, BT2, BT1) */
            EventBits_t st = xEventGroupGetBits(xButtonEventGroup);
            bool running = (st & EV_STATE_RUNNING) != 0;

            const char *leg3 = "MODE";
            const char *leg2 = "-";
            const char *leg1 = "-";

            switch (m) {
                case MODE_CLOCK:
                    leg1="SET";    leg2="-";    break;
                case MODE_CLOCK_SET:
                    leg1="SEL";    leg2="INC";  break;
                case MODE_ALARM:
                    leg1="SET";    leg2="-";    break;
                case MODE_ALARM_SET:
                    leg1="SEL";    leg2="INC";  break;
                case MODE_CHRONO:
                    if (running) {
                        leg1="PAUSE"; leg2="-";
                    } else {
                        leg1="START"; leg2="RESET";
                    }
                    break;
                case MODE_ALARM_RING:
                    leg1="STOP";   leg2="SNOOZE"; break;
                default:
                    leg1="-";     leg2="-";      break;
            }

            ILI9341DrawString( 10, 300, (char*)leg3, &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140, 300, (char*)leg2, &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280, 300, (char*)leg1, &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
        }
    }
}
