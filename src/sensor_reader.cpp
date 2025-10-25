#include <Wire.h>
#include <DHT20.h>
#include "global.h"

void sensor_reader(void *pvParameters) {
  Wire.begin(11, 12);
  dht20.begin();

  SensorData data;

  while (true) {
    if (dht20.read() == 0) {
      data.temperature = dht20.getTemperature();
      data.humidity = dht20.getHumidity();

      Serial.printf("[SENSOR] Temp: %.2f°C | Humi: %.2f%%\n",
                    data.temperature, data.humidity);

      if (xQueueSend(sensorQueue, &data, 0) != pdPASS) {
        Serial.println("[SENSOR] Queue full, skipping data...");
      }
    } else {
      Serial.println("[SENSOR] DHT20 read error!");
    }

    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}
