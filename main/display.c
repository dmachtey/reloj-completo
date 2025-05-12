/* File: main/display.c */
#include "display.h"
#include "display_config.h"
#include "ili9341.h"
#include "digitos.h"
#include "fonts.h"
#include "mode_manager.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "esp_log.h"

static const char *TAG = "DISPLAY.C";

/* Paneles de dígitos */
static panel_t panel_minutes;
static panel_t panel_seconds;
static panel_t panel_tenths;

/* Colas */
QueueHandle_t xChronoQueue = NULL;
QueueHandle_t xClockQueue  = NULL;
QueueHandle_t xAlarmQueue  = NULL;  // Cola con AlarmData_t

/* Dibuja título y leyendas según modo */
static void draw_static_legend(app_mode_t mode)
{
    ILI9341Fill(DIGITO_FONDO);

    switch (mode) {
        case MODE_CLOCK:
            ILI9341DrawString(350, 10, "CLOCK",     &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString( 10,300, "PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            break;

        case MODE_CLOCK_SET:
            ILI9341DrawString(350, 10, "CLK-SET",   &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString( 10,300, "PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140,300, "PB2: SET",  &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280,300, "PB1: INC",  &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            break;

        case MODE_CHRONO:
            ILI9341DrawString(350, 10, "CHRONO",    &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString( 10,300, "PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140,300, "PB2: RESET",&font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280,300, "PB1: START/LAP",&font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            break;

        case MODE_ALARM:
            ILI9341DrawString(350, 10, "ALARM",     &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString( 10,300, "PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140,300, "PB2: SNOOZE",&font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280,300, "PB1: STOP", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            break;

        case MODE_ALARM_SET:
            ILI9341DrawString(350, 10, "AL-SET",    &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString( 10,300, "PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(140,300, "PB2: SET",  &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            ILI9341DrawString(280,300, "PB1: INC",  &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
            break;

        default:
            return;
    }
}

void display_init(void)
{
    ILI9341Init();
    ILI9341Rotate(ILI9341_Landscape_1);

    panel_minutes = CrearPanel( 30,  60, 2,
                                DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_seconds = CrearPanel(170,  60, 2,
                                DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);
    panel_tenths  = CrearPanel(310,  60, 2,
                                DIGITO_ALTO, DIGITO_ANCHO,
                                DIGITO_ENCENDIDO, DIGITO_APAGADO, DIGITO_FONDO);

    xChronoQueue = xQueueCreate(1, sizeof(ChronoData_t));
    xClockQueue  = xQueueCreate(1, sizeof(ClockData_t));
    xAlarmQueue  = xQueueCreate(1, sizeof(AlarmData_t));

    ESP_LOGI(TAG, "display task INIT");
}

void display_task(void *pvParameters)
{
    app_mode_t    old_mode     = (app_mode_t)-1;
    bool          mode_changed = true, first_cycle = true;
    uint32_t      old_mins     = 0xFFFFFFFF;
    uint32_t      old_secs     = 0xFFFFFFFF;
    uint32_t      old_tenth    = 0xFFFFFFFF;
    uint32_t      old_laps     = 0xFFFFFFFF;
    uint16_t      circleColor  = DIGITO_ENCENDIDO;
    uint8_t       circleChange = 0;
    ChronoData_t  d            = { .mode = MODE_CHRONO, .thents = 0,       .laps = {0} };
    ClockData_t   c            = { .mode = MODE_CLOCK,  .clk_h = 0, .clk_m = 0, .clk_s = 0 };
    AlarmData_t   a            = { .mode = MODE_ALARM,  .al_h = 0, .al_m = 0, .enable = false };

    for (;;) {
        /* al cambiar de modo, redibuja leyenda y resetea viejos */
        if ((mode_changed = ((old_mode != current_mode) | first_cycle))) {
            draw_static_legend(current_mode);
            old_mode  = current_mode;
            old_mins  = old_secs = old_tenth = 0xFFFFFFFF;
            old_laps  = 0xFFFFFFFF;
        }

        /* --- MODO CHRONO (sin cambios) --- */
        if ( current_mode == MODE_CHRONO
          && ((xQueueReceive(xChronoQueue, &d, pdMS_TO_TICKS(2)) == pdTRUE)
           |  mode_changed) )
        {
            if (d.mode != MODE_CHRONO) continue;
            uint32_t mins  = (d.thents / 6000) % 100;
            uint32_t secs  = (d.thents / 100)  % 60;
            uint32_t tenth =  d.thents         % 100;

            if (old_mins  != mins)  { DibujarDigito(panel_minutes,0, mins/10);  DibujarDigito(panel_minutes,1, mins%10);  old_mins  = mins; }
            if (old_secs  != secs)  { DibujarDigito(panel_seconds,0, secs/10);  DibujarDigito(panel_seconds,1, secs%10);  old_secs  = secs; }
            if (old_tenth != tenth) {
                DibujarDigito(panel_tenths,0, tenth/10);
                DibujarDigito(panel_tenths,1, tenth%10);
                old_tenth = tenth;
                /* separador parpadeante */
                circleChange++;
                if ((circleChange > 10) | mode_changed) {
                    circleChange = 0;
                    circleColor  = (circleColor == DIGITO_ENCENDIDO) ? DIGITO_APAGADO : DIGITO_ENCENDIDO;
                    ILI9341DrawFilledCircle(160,100,5,circleColor);
                    ILI9341DrawFilledCircle(160,140,5,circleColor);
                    ILI9341DrawFilledCircle(300,100,5,circleColor);
                    ILI9341DrawFilledCircle(300,140,5,circleColor);
                }
            }
            /* laps igual que antes */
            if ((old_laps != d.laps[0]) | mode_changed) {
                char buf[16];
                for (int i = 0; i < 4; i++) {
                    uint32_t pmin = (d.laps[i] / 6000) % 100;
                    uint32_t psec = (d.laps[i] / 100)  % 60;
                    uint32_t pde  =  d.laps[i]         % 100;
                    snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", pmin,psec,pde);
                    ILI9341DrawString(30,170+24*i, buf, &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
                }
                old_laps = d.laps[0];
            }
        }
        /* --- MODO CLOCK / CLOCK_SET --- */
        else if ((current_mode == MODE_CLOCK || current_mode == MODE_CLOCK_SET)
              && ((xQueueReceive(xClockQueue, &c, pdMS_TO_TICKS(2)) == pdTRUE)
               |  mode_changed) )
        {
            if (c.mode != MODE_CLOCK && c.mode != MODE_CLOCK_SET) continue;
            if (mode_changed) {
                /* círculos fijos encendidos */
                ILI9341DrawFilledCircle(160,100,5,DIGITO_ENCENDIDO);
                ILI9341DrawFilledCircle(160,140,5,DIGITO_ENCENDIDO);
                ILI9341DrawFilledCircle(300,100,5,DIGITO_ENCENDIDO);
                ILI9341DrawFilledCircle(300,140,5,DIGITO_ENCENDIDO);
            }
            uint32_t mins  = c.clk_h;  // horas
            uint32_t secs  = c.clk_m;  // minutos
            uint32_t tenth = c.clk_s;  // segundos

            if (old_mins  != mins)  { DibujarDigito(panel_minutes,0, mins/10);  DibujarDigito(panel_minutes,1, mins%10);  old_mins  = mins; }
            if (old_secs  != secs)  { DibujarDigito(panel_seconds,0, secs/10);  DibujarDigito(panel_seconds,1, secs%10);  old_secs  = secs; }
            if (old_tenth != tenth) { DibujarDigito(panel_tenths,0, tenth/10);  DibujarDigito(panel_tenths,1, tenth%10);  old_tenth = tenth; }
        }
        /* --- MODO ALARM / ALARM_SET --- */
        else if ((current_mode == MODE_ALARM || current_mode == MODE_ALARM_SET)
              && ((xQueueReceive(xAlarmQueue, &a, pdMS_TO_TICKS(2)) == pdTRUE)
               |  mode_changed) )
        {
            if (a.mode != MODE_ALARM && a.mode != MODE_ALARM_SET) continue;
            uint32_t mins = a.al_h;  // horas de alarma
            uint32_t secs = a.al_m;  // minutos de alarma

            if (old_mins != mins) { DibujarDigito(panel_minutes,0, mins/10);  DibujarDigito(panel_minutes,1, mins%10);  old_mins = mins; }
            if (old_secs != secs) { DibujarDigito(panel_seconds,0, secs/10);  DibujarDigito(panel_seconds,1, secs%10);  old_secs = secs; }
            ILI9341DrawFilledCircle(160,100,5,DIGITO_ENCENDIDO);
            ILI9341DrawFilledCircle(160,140,5,DIGITO_ENCENDIDO);
            ILI9341DrawString(350, 30,
                              a.enable ? "ENABLE" : "DISABLE",
                              &font_7x10,
                              ILI9341_WHITE,
                              DIGITO_FONDO);

        }


        mode_changed = first_cycle = false;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
