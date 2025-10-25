#include "led_blinky.h"
#include "global.h"
#include <algorithm>

QueueHandle_t sensorQueue = NULL;

void led_blinky(void *pvParameters) {
  pinMode(LED_GPIO, OUTPUT);

  sensorQueue = xQueueCreate(10, sizeof(SensorData));
  if (sensorQueue == NULL) {
    Serial.println("[LED] Failed to create queue!");
    vTaskDelete(NULL);
  }

  Serial.println("[LED] Queue created successfully.");

  SensorData data;
  float temp = 0.0;

  while (true) {
    // Wait for semaphore 
    if (xSemaphoreTake(ledSemaphore, portMAX_DELAY) == pdTRUE) {
      if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
        temp = data.temperature;

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
}
