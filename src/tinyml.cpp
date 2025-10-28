#include "dht_anomaly_model.h"
#include "global.h"
#include <DHT20.h>

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// === Globals for TensorFlow Lite Micro ===
namespace {
  tflite::ErrorReporter* error_reporter = nullptr;
  const tflite::Model* model = nullptr;
  tflite::MicroInterpreter* interpreter = nullptr;
  TfLiteTensor* input = nullptr;
  TfLiteTensor* output = nullptr;

  constexpr int kTensorArenaSize = 8 * 1024;
  uint8_t tensor_arena[kTensorArenaSize];
}

// === Helper: Map index to season name ===
const char* getSeasonName(int index) {
  switch (index) {
    case 0: return "Spring";
    case 1: return "Summer";
    case 2: return "Autumn";
    case 3: return "Winter";
    default: return "Unknown";
  }
}

// === FreeRTOS Task for TinyML inference ===
void tiny_ml_task(void *pvParameters) {
  Serial.println("[TinyML] Initializing TensorFlow Lite Micro...");

  // Initialize TFLite Micro
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  model = tflite::GetModel(dht_anomaly_model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    error_reporter->Report("Model schema version %d != supported version %d",
                           model->version(), TFLITE_SCHEMA_VERSION);
    vTaskDelete(NULL);
  }

  static tflite::AllOpsResolver resolver;
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    error_reporter->Report("AllocateTensors() failed");
    vTaskDelete(NULL);
  }

  input = interpreter->input(0);
  output = interpreter->output(0);

  Serial.println("[TinyML] Initialization successful.");

  SensorData data;

  while (true) {
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      data = latestData;
      xSemaphoreGive(dataMutex);
    } else {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    float temp = data.temperature;
    float hum = data.humidity;

    // === Prepare Input ===
    input->data.f[0] = temp;
    input->data.f[1] = hum;

    // === Run Inference ===
    if (interpreter->Invoke() != kTfLiteOk) {
      error_reporter->Report("Invoke failed!");
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    // === Get Output (4-class probabilities) ===
    float best_score = 0.0f;
    int best_index = -1;

    for (int i = 0; i < 4; i++) {
      float score = output->data.f[i];
      if (score > best_score) {
        best_score = score;
        best_index = i;
      }
    }

    Serial.println("-----------------------------");
    Serial.printf("[TinyML] Predicted Season: %s\n", getSeasonName(best_index));
    Serial.printf("[TinyML] Confidence: %.2f%%\n", best_score);
    Serial.println("-----------------------------\n");

    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        currentSeason = String(getSeasonName(best_index));
        currentConfidence = best_score;
        xSemaphoreGive(dataMutex);
}

    vTaskDelay(pdMS_TO_TICKS(5000)); 
  }
}
