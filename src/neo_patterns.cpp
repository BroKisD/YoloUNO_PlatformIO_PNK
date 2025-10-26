#include "mainserver.h"

// Police mode colors
const uint32_t BLUE = 0x0000FF;
const uint32_t RED = 0xFF0000;

// Traffic light colors
const uint32_t GREEN = 0x00FF00;
const uint32_t YELLOW = 0xFFFF00;

unsigned long lastUpdate = 0;
bool policeState = false;
int trafficState = 0;

void updateNeoLED() {
    if (!neoControlEnabled) {
        return;  // Let humidity mode control the LED
    }

    unsigned long currentMillis = millis();

    switch (currentNeoMode) {
        case POLICE_MODE:
            if (currentMillis - lastUpdate >= 500) {  // Toggle every 500ms
                policeState = !policeState;
                neoPixel.setPixelColor(0, policeState ? RED : BLUE);
                neoPixel.show();
                lastUpdate = currentMillis;
            }
            break;

        case TRAFFIC_MODE:
            if (currentMillis - lastUpdate >= 3000) {  // Change every 3 seconds
                trafficState = (trafficState + 1) % 3;
                switch (trafficState) {
                    case 0:  // Green
                        neoPixel.setPixelColor(0, GREEN);
                        break;
                    case 1:  // Yellow
                        neoPixel.setPixelColor(0, YELLOW);
                        break;
                    case 2:  // Red
                        neoPixel.setPixelColor(0, RED);
                        break;
                }
                neoPixel.show();
                lastUpdate = currentMillis;
            }
            break;

        default:
            break;
    }
}