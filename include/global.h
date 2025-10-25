#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#pragma once
#include <Arduino.h>
#include <queue.h>
#include <DHT20.h>

#define SENSOR_PIN 1

typedef struct {
  float temperature;
  float humidity;
} SensorData;


extern DHT20 dht20;

extern QueueHandle_t sensorQueue;

extern SemaphoreHandle_t ledSemaphore;
extern SemaphoreHandle_t neoSemaphore;


extern int glob_var;

#endif