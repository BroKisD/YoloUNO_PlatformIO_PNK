#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#pragma once
#include <Arduino.h>
#include <queue.h>

struct SensorData {
  float temperature;
};

extern QueueHandle_t sensorQueue;


extern int glob_var;

#endif