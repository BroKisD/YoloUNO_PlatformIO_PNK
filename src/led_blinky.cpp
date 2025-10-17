#include "led_blinky.h"
#include "global.h"

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
    if (xQueueReceive(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
      temp = data.temperature;
      Serial.printf("[LED] Received temperature: %.2f°C\n", temp);

      int delayTime;

      if (temp < 25.0) {
        delayTime = 1000;  
        Serial.println("[LED] Mode: COOL (Slow blink)");
      } else if (temp < 30.0) {
        delayTime = 500;   
        Serial.println("[LED] Mode: WARM (Medium blink)");
      } else {
        delayTime = 200;  
        Serial.println("[LED] Mode: HOT (Fast blink)");
      }

       for (int i = 0; i < 3; i++){
      digitalWrite(LED_GPIO, HIGH);
      vTaskDelay(pdMS_TO_TICKS(delayTime));
      digitalWrite(LED_GPIO, LOW);
      vTaskDelay(pdMS_TO_TICKS(delayTime));
       }
    }
  }
}
