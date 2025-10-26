#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "mainserver.h"

#define NEO_PIN 45
#define LED_COUNT 1

extern bool neoControlEnabled;

void neo_blinky(void *pvParameters);


#endif