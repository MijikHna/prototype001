#include "AnimationEngine.hpp"
#include "WlanMeshEngine.hpp"

#include "definitions.hpp"

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <cppQueue.h>

// #define DEBUG 1
// #define DEBUG_STATE 1
// #define DEBUG_JSON 1
// #define DEBUG_CORE 1

// global variables
extern const PROGMEM uint8_t _dimLUT[];
uint8_t R[LED_COUNT], G[LED_COUNT], B[LED_COUNT];

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
Scheduler scheduler1;
Scheduler scheduler2;

WlanMeshEngine wlanMeshEngine;
AnimationEngine animationEngine;

#ifdef DEBUG
extern String states[];
#endif

void setup()
{
#ifdef DEBUG_CORE
    Serial.println("Core: " + String(xPortGetCoreID(), DEC));
#endif
    Serial.begin(115200);

    for (int i = 0; i < LED_COUNT; i++) {
        R[i] = 0;
        G[i] = 0;
        B[i] = 0;
    }

    strip.begin();
    strip.show();
    strip.setBrightness(MAX_BRIGTHNESS);

    wlanMeshEngine.setDebugMsgTypes(DEBUG | STARTUP);
    wlanMeshEngine.init(MESH_PREFIX, MESH_PASSWORD, &scheduler1, MESH_PORT);

    scheduler2.addTask(animationEngine.FXtask);

    animationEngine.FXtask.enableDelayed(ANIMATION_CYCLE_LENGTH - (wlanMeshEngine.getNodeTime() % (ANIMATION_CYCLE_LENGTH * 1000)) / 1000);

    randomSeed(analogRead(A0));

#ifdef DEBUG
    scheduler1.addTask(taskSendMessage);
    taskSendMessage.enable();
#endif

    scheduler2.startNow();

    TaskHandle_t animationEngineTask;
    xTaskCreatePinnedToCore(
        AnimationEngine::run,
        "classNr1Task",
        10000,
        &scheduler2,
        0,
        &animationEngineTask,
        0);
}

void loop()
{
    wlanMeshEngine.update();
    // scheduler2.execute();

    for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(pgm_read_word_near(_dimLUT + R[i]), pgm_read_word_near(_dimLUT + G[i]), pgm_read_word_near(_dimLUT + B[i])));
    }
    strip.show();
}