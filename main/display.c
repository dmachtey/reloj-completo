/* File: main/display.c */
#include "display.h"
#include "display_config.h"
#include "ili9341.h"
#include "digitos.h"
#include "fonts.h"
#include "mode_manager.h"
#include <stdio.h>
#include "esp_log.h"


static const char *TAG = "DISPLAY.C";

/* Paneles de dígitos */
static panel_t panel_minutes;
static panel_t panel_seconds;
static panel_t panel_tenths;

/* Colas y conjunto de colas */
QueueHandle_t    xChronoQueue     = NULL;
QueueHandle_t    xClockQueue      = NULL;
QueueHandle_t    xAlarmQueue      = NULL;
//QueueSetHandle_t xDisplayQueueSet = NULL;

/**
 * @brief  Dibuja las leyendas estáticas (título y PBx) según el modo actual.
 */
static void draw_static_legend(app_mode_t mode)
{
  const char *title;
  const char *pb2;
  const char *pb1;

  ILI9341Fill(DIGITO_FONDO);

  switch (mode)
    {
    case MODE_CLOCK:
    case MODE_CLOCK_SET:
      title = (mode == MODE_CLOCK) ? "CLOCK" : "CLK-SET";
      pb2   = "PB2: SET";
      pb1   = "PB1: INC";
      break;

    case MODE_CHRONO:
      title = "CHRONO";
      pb2   = "PB2: RESET";
      pb1   = "PB1: START";
      break;

    case MODE_ALARM:
    case MODE_ALARM_SET:
    case MODE_ALARM_RING:
      title = (mode == MODE_ALARM)     ? "ALARM"
        : (mode == MODE_ALARM_SET) ? "AL-SET"
        : "!RING!";
      pb2   = "PB2: SNOOZE";
      pb1   = "PB1: STOP";
      break;

    default:
      return;
    }

  ILI9341DrawString(350, 10, (char*)title, &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
  ILI9341DrawString(10, 300,  (char*)"PB3: MODE", &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
  ILI9341DrawString(140,300,  (char*)pb2,       &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
  ILI9341DrawString(280,300,  (char*)pb1,       &font_7x10, ILI9341_WHITE, DIGITO_FONDO);
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

  xChronoQueue     = xQueueCreate(1, sizeof(ChronoData_t));
  xClockQueue      = xQueueCreate(1, sizeof(ClockData_t));
  xAlarmQueue      = xQueueCreate(1, sizeof(AlarmData_t));

  // xDisplayQueueSet = xQueueCreateSet(3);
  // xQueueAddToSet(xChronoQueue, xDisplayQueueSet);
  // xQueueAddToSet(xClockQueue,  xDisplayQueueSet);
  // xQueueAddToSet(xAlarmQueue,  xDisplayQueueSet);

  ESP_LOGI(TAG, "display task INIT");
}


/** @brief Helper to convert app_mode_t to a human-readable string */
static const char* mode_to_string(app_mode_t mode)
{
  switch (mode) {
  case MODE_CLOCK:      return "MODE_CLOCK";
  case MODE_CLOCK_SET:  return "MODE_CLOCK_SET";
  case MODE_ALARM:      return "MODE_ALARM";
  case MODE_ALARM_SET:  return "MODE_ALARM_SET";
  case MODE_CHRONO:     return "MODE_CHRONO";
  case MODE_ALARM_RING: return "MODE_ALARM_RING";
  default:              return "UNKNOWN_MODE";
  }
}


void display_task(void *pvParameters)
{
  app_mode_t old_mode = (app_mode_t)-1;

  for (;;)
    {
      //QueueHandle_t h = xQueueSelectFromSet(xDisplayQueueSet, portMAX_DELAY);
      if (old_mode != current_mode){
        draw_static_legend(current_mode);
        old_mode = current_mode;
        ESP_LOGI(TAG, "display task %s", mode_to_string(current_mode));
      }

      // /* --- MODO CRONÓMETRO --- */
      // if (h == xChronoQueue)
      // {

      ChronoData_t d;
      if (xQueueReceive(xChronoQueue, &d, 2) == pdTRUE)
        {
          ESP_LOGI(TAG, "Recibimos por la cola de chrono");
          /* Desechamos si no es realmente modo CHRONO */
          if (d.mode != MODE_CHRONO) {
            continue;
          }
          // /* Si cambia el modo, redibujamos leyendas estáticas */
          // if (d.mode != old_mode) {
          //   old_mode = d.mode;
          //   draw_static_legend(d.mode);
          // }
          /* Luego dibujamos el tiempo y vueltas */
          uint32_t mins  = (d.thents / 6000) % 100;
          uint32_t secs  = (d.thents / 100)  % 60;
          uint32_t tenth =  d.thents         % 100;
          DibujarDigito(panel_minutes, 0, mins / 10);
          DibujarDigito(panel_minutes, 1, mins % 10);
          DibujarDigito(panel_seconds, 0, secs / 10);
          DibujarDigito(panel_seconds, 1, secs % 10);
          DibujarDigito(panel_tenths,  0, tenth / 10);
          DibujarDigito(panel_tenths,  1, tenth % 10);

          char buf[16];
          for (int i = 0; i < 4; i++)
            {
              uint32_t pmin = (d.laps[i] / 6000) % 100;
              uint32_t psec = (d.laps[i] / 100)  % 60;
              uint32_t pde  =  d.laps[i]         % 100;
              snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", pmin, psec, pde);
              ILI9341DrawString(30, 170 + 24 * i, buf, &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
            }
        }
      // }
      // /* --- MODO RELOJ --- */
      // else if (h == xClockQueue)
      // {
      //     ClockData_t c;
      //     if (xQueueReceive(xClockQueue, &c, 0) == pdTRUE)
      //     {
      //         /* Desechamos si no es CLOCK ni CLOCK_SET */
      //         if (c.mode != MODE_CLOCK && c.mode != MODE_CLOCK_SET) {
      //             continue;
      //         }
      //         if (c.mode != old_mode) {
      //             old_mode = c.mode;
      //             draw_static_legend(c.mode);
      //         }
      //         char buf[32];
      //         snprintf(buf, sizeof(buf), "%02u:%02u:%02u", c.clk_h, c.clk_m, c.clk_s);
      //         ILI9341DrawString(80, 200, buf, &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
      //     }
      // }
      // /* --- MODO ALARMA --- */
      // else if (h == xAlarmQueue)
      // {
      //     AlarmData_t a;
      //     if (xQueueReceive(xAlarmQueue, &a, 0) == pdTRUE)
      //     {
      //         /* Desechamos si no es ALARM, ALARM_SET ni ALARM_RING */
      //         if (a.mode != MODE_ALARM && a.mode != MODE_ALARM_SET && a.mode != MODE_ALARM_RING) {
      //             continue;
      //         }
      //         if (a.mode != old_mode) {
      //             old_mode = a.mode;
      //             draw_static_legend(a.mode);
      //         }
      //         if (a.mode == MODE_ALARM_RING)
      //         {
      //             ILI9341DrawString(80, 200, (char*)"!!! ALARM !!!", &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
      //         }
      //         else
      //         {
      //             char buf2[16];
      //             snprintf(buf2, sizeof(buf2), "%02u:%02u", a.al_h, a.al_m);
      //             ILI9341DrawString(120,200, buf2, &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
      //         }
      //     }
      // }
      vTaskDelay(pdMS_TO_TICKS(2));
    }
}
