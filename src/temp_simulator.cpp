#include "temp_simulator.h"
#include "global.h"

void temp_simulator(void *pvParameters) {
  SensorData data;

  while (true) {
    // Giả lập dữ liệu cảm biến (dao động 20–40°C)
    data.temperature = 20.0f + (float)(rand() % 2000) / 100.0f;

    // Chờ đến khi queue được tạo (tránh lỗi null)
    if (sensorQueue != NULL) {
      if (xQueueSend(sensorQueue, &data, portMAX_DELAY) == pdPASS) {
        Serial.printf("[Sim] Sent temp: %.2f°C\n", data.temperature);
      }
    } else {
      Serial.println("[Sim] Waiting for queue...");
    }

    vTaskDelay(pdMS_TO_TICKS(1000));  // Gửi mỗi giây
  }
}
