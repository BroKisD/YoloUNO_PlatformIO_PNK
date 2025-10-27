#include "led_blinky.h"
#include "global.h"
#include <algorithm>

void led_blinky(void *pvParameters) {
  pinMode(LED_GPIO, OUTPUT);

  SensorData data;

  while (true) {
    if (xSemaphoreTake(ledSemaphore, portMAX_DELAY) == pdTRUE) {

      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        data = latestData;
        xSemaphoreGive(dataMutex);
      }

      float temp = data.temperature;

      int delayTime = 1000
                    - 100 * std::max(0.0f, std::min(temp - 25.0f, 5.0f))
                    - 60  * std::max(0.0f, temp - 30.0f);

      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_GPIO, HIGH);
        vTaskDelay(pdMS_TO_TICKS(delayTime));
        digitalWrite(LED_GPIO, LOW);
        vTaskDelay(pdMS_TO_TICKS(delayTime));
      }
    }
  }
}
