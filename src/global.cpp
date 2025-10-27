#include "global.h"


SemaphoreHandle_t ledSemaphore = NULL;
SemaphoreHandle_t neoSemaphore = NULL;
SemaphoreHandle_t dataMutex = NULL;

QueueHandle_t sensorQueue = NULL;

SensorData latestData = {0.0, 0.0}; 