#include "neo_blinky.h"
#include "global.h"
#include <Adafruit_NeoPixel.h>

uint32_t humidityToColor(Adafruit_NeoPixel &strip, float humidity) {
    uint8_t r = 0, g = 0, b = 0;

    if (humidity < 40) {
        //red
        float t = humidity / 40.0;
        r = 255;
        g = (uint8_t)(255 * t);
        b = 0;
    } 
    else if (humidity < 70) {
        //green
        float t = (humidity - 40) / 30.0;
        r = (uint8_t)(255 * (1 - t));
        g = 255;
        b = 0;
    } 
    else {
        //blue
        float t = (humidity - 70) / 30.0;
        r = 0;
        g = (uint8_t)(255 * (1 - t));
        b = (uint8_t)(255 * t);
    }

    return strip.Color(r, g, b);
}


void neo_blinky(void *pvParameters) {
  Adafruit_NeoPixel strip(LED_COUNT, NEO_PIN, NEO_GRB + NEO_KHZ800);
  strip.begin();
  strip.clear();
  strip.show();

  SensorData data;

  while (true) {
    // Wait for semaphore 
    if (xSemaphoreTake(neoSemaphore, portMAX_DELAY) == pdTRUE) {
      // Only process if we're in HUMIDITY_MODE and neoControlEnabled is false
      if (!neoControlEnabled && xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
        float humi = data.humidity;
        uint32_t color = humidityToColor(strip, humi);

        strip.setPixelColor(0, color);
        strip.show();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
