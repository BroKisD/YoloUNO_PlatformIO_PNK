#ifndef ___MAIN_SERVER__
#define ___MAIN_SERVER__
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_NeoPixel.h>
#include "global.h"

#define LED1_PIN 48
#define LED2_PIN 41
#define BOOT_PIN 0
#define NEO_PIN 45
#define NUMPIXELS 1

// Neo LED control modes
enum NeoMode {
    HUMIDITY_MODE,
    POLICE_MODE,
    TRAFFIC_MODE
};

extern Adafruit_NeoPixel neoPixel;
void updateNeoLED();
extern WebServer server;
extern bool isAPMode;
extern NeoMode currentNeoMode;
extern bool neoControlEnabled;

String mainPage();
String settingsPage();
void handleNeoMode();

void startAP();
void setupServer();
void connectToWiFi();

void main_server_task(void *pvParameters);

#endif