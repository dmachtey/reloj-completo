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
  bool mode_changed = true;
  bool first_cycle = true;
  uint32_t old_mins = 0xFFFFFFFF;
  uint32_t old_secs = 0xFFFFFFFF;
  uint32_t old_tenth = 0xFFFFFFFF;
  uint16_t circleColor = DIGITO_ENCENDIDO;
  uint8_t circleChange = 0;
  uint32_t old_laps = 0xFFFFFFFF;
  ChronoData_t d = {
    .mode   = MODE_CHRONO,
    .thents = 0,
    .laps   = { 0, 0, 0, 0 }
  };


  for (;;)
    {


      if ((mode_changed = ((old_mode != current_mode) | first_cycle))) {
        draw_static_legend(current_mode);
        old_mode = current_mode;
        // Redraw the chrono if mode has changed.
        old_mins = 0xFFFFFFFF;
        old_secs = 0xFFFFFFFF;
        old_tenth = 0xFFFFFFFF;
      }

      // /* --- MODO CRONÓMETRO --- */
      // if (h == xChronoQueue)
      // {



        if ((current_mode == MODE_CHRONO) & ((xQueueReceive(xChronoQueue, &d, 2) == pdTRUE)| mode_changed))
        {
          //ESP_LOGI(TAG, "Recibimos por la cola de chrono");
          /* Desechamos si no es realmente modo CHRONO */
          if (d.mode != MODE_CHRONO) {
            continue;
          }

          uint32_t mins  = (d.thents / 6000) % 100;
          uint32_t secs  = (d.thents / 100)  % 60;
          uint32_t tenth =  d.thents         % 100;
          /* draw minutes */
          if (old_mins != mins){
            DibujarDigito(panel_minutes, 0, mins / 10);
            DibujarDigito(panel_minutes, 1, mins % 10);

            /* draw colon separator as two filled circles */
            old_mins = mins;
          }


          if (old_secs != secs){
            /* draw seconds */
            DibujarDigito(panel_seconds, 0, secs / 10);
            DibujarDigito(panel_seconds, 1, secs % 10);

            /* draw colon separator between seconds and tenths */
            old_secs = secs;
          }

          if (old_tenth != tenth){
            /* draw tenths */
            DibujarDigito(panel_tenths, 0, tenth / 10);
            DibujarDigito(panel_tenths, 1, tenth % 10);
            old_tenth = tenth;

            circleChange++;
            if ((circleChange > 10)| (mode_changed) )
              {
                circleChange = 0;
                if ((circleColor !=  DIGITO_ENCENDIDO) | mode_changed)
                  circleColor = DIGITO_ENCENDIDO;
                else
                  circleColor = DIGITO_APAGADO;

                ILI9341DrawFilledCircle(160, 100, 5, circleColor);
                ILI9341DrawFilledCircle(160, 140, 5, circleColor);
                ILI9341DrawFilledCircle(300, 100, 5, circleColor);
                ILI9341DrawFilledCircle(300, 140, 5, circleColor);
              }
          }

          if ((old_laps != d.laps[0])| mode_changed){
            char buf[16];
            for (int i = 0; i < 4; i++)
              {
                uint32_t pmin = (d.laps[i] / 6000) % 100;
                uint32_t psec = (d.laps[i] / 100)  % 60;
                uint32_t pde  =  d.laps[i]         % 100;
                snprintf(buf, sizeof(buf), "%02lu:%02lu.%02lu", pmin, psec, pde);
                ILI9341DrawString(30, 170 + 24 * i, buf, &font_11x18, ILI9341_WHITE, DIGITO_APAGADO);
                old_laps = d.laps[0];
              }
          }

        }
        mode_changed = false;
        first_cycle = false;
      vTaskDelay(pdMS_TO_TICKS(10));
    }
}
