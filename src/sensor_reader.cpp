#include <Wire.h>
#include <DHT20.h>
#include "global.h"

void sensor_reader(void *pvParameters) {
  Wire.begin(11, 12);
  dht20.begin();

  while (true) {
    if (dht20.read() == 0) {
      float temp = dht20.getTemperature();
      float humi = dht20.getHumidity();

      Serial.printf("[SENSOR] Temp: %.2f°C | Humi: %.2f%%\n", temp, humi);

      if (xSemaphoreTake(dataMutex, portMAX_DELAY) == pdTRUE) {
        latestData.temperature = temp;
        latestData.humidity = humi;
        xSemaphoreGive(dataMutex);
      }

      xSemaphoreGive(ledSemaphore);
      xSemaphoreGive(neoSemaphore);
    } else {
      Serial.println("[SENSOR] DHT20 read error!");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
