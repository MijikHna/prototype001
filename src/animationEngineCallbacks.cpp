#include "AnimationEngine.hpp"
#include "WlanMeshEngine.hpp"
#include "definitions.hpp"

// #define DEBUG_CORE 1
// #define DEBUG_TASK 1

extern AnimationEngine animationEngine;
extern WlanMeshEngine wlanMeshEngine;

extern uint8_t R[LED_COUNT], G[LED_COUNT], B[LED_COUNT];

void taskCallback()
{
#ifdef DEBUG_CORE
    Serial.println("Core: " + String(xPortGetCoreID(), DEC));
#endif
    static uint32_t animationStep = 0;
    if (animationEngine.FXtask.isFirstIteration()) {
        for (int i = 0; i < LED_COUNT; i++) {
            R[i] = 0;
            G[i] = 0;
            B[i] = 0;
        }

        if (!animationEngine.q.isEmpty()) {
            animationEngine.q.pop(&animationEngine.currentAnimation);
        }
        animationStep = animationEngine.currentAnimation.cycleMod;

        if (wlanMeshEngine.getCurrentState() == WLAN_MESH_STATE::STANDALONE || wlanMeshEngine.getCurrentState() == WLAN_MESH_STATE::MASTER) {
            if (random(0, 4) == 3) {
                effect tempFx = { (uint8_t)random(101, 104),
                    random(0, 255) * 1.0,
                    random(0, 255) * 1.0,
                    random(0, 255) * 1.0,
                    (uint8_t)random(1, 8) };
                animationEngine.addNextAnimation(tempFx);
            }
        }
#ifdef DEBUG
        Serial.println("First Iteration");
#endif
    }

    switch (animationEngine.currentAnimation.fxId) {
    case 101: // Pulsing-Animation
        if (animationStep <= (ANIMATION_STEPS_HALF)) {
            for (int i = 0; i < LED_COUNT; i++) {
                R[i] = animationStep * (animationEngine.currentAnimation.red / (ANIMATION_STEPS_HALF));
                G[i] = animationStep * (animationEngine.currentAnimation.green / (ANIMATION_STEPS_HALF));
                B[i] = animationStep * (animationEngine.currentAnimation.blue / (ANIMATION_STEPS_HALF));
            }
        } else {
            for (int i = 0; i < LED_COUNT; i++) {
                R[i] = (ANIMATION_STEPS / 2) * (animationEngine.currentAnimation.red / (ANIMATION_STEPS_HALF)) - (animationStep - (ANIMATION_STEPS / 2)) * (animationEngine.currentAnimation.red / (ANIMATION_STEPS_HALF));
                G[i] = (ANIMATION_STEPS / 2) * (animationEngine.currentAnimation.green / (ANIMATION_STEPS_HALF)) - (animationStep - (ANIMATION_STEPS / 2)) * (animationEngine.currentAnimation.green / (ANIMATION_STEPS_HALF));
                B[i] = (ANIMATION_STEPS / 2) * (animationEngine.currentAnimation.blue / (ANIMATION_STEPS_HALF)) - (animationStep - (ANIMATION_STEPS / 2)) * (animationEngine.currentAnimation.blue / (ANIMATION_STEPS_HALF));
            }
        }
        animationStep += 1 * animationEngine.currentAnimation.cycleMod;
        if (animationStep > ANIMATION_STEPS)
            animationStep = animationEngine.currentAnimation.cycleMod;
        break;
    case 102: // Blinking-Animation
        if (animationStep <= (ANIMATION_STEPS / 2)) {
            for (int i = 0; i < LED_COUNT; i++) {
                R[i] = animationEngine.currentAnimation.red;
                G[i] = animationEngine.currentAnimation.green;
                B[i] = animationEngine.currentAnimation.blue;
            }
        } else {
            for (int i = 0; i < LED_COUNT; i++) {
                R[i] = 0;
                G[i] = 0;
                B[i] = 0;
            }
        }
        animationStep += 1 * animationEngine.currentAnimation.cycleMod;
        if (animationStep > ANIMATION_STEPS)
            animationStep = animationEngine.currentAnimation.cycleMod;
        break;
    case 103: //Chase-Effekt
        if (animationStep <= (ANIMATION_STEPS_HALF)) {
            for (int i = 0; i < (animationStep / ((ANIMATION_STEPS / 2) / LED_COUNT)); i++) {
                R[i] = animationEngine.currentAnimation.red;
                G[i] = animationEngine.currentAnimation.green;
                B[i] = animationEngine.currentAnimation.blue;
            }
        } else {
            for (int i = 0; i < ((animationStep - (ANIMATION_STEPS / 2)) / ((ANIMATION_STEPS / 2) / LED_COUNT)); i++) {
                R[i] = 0;
                G[i] = 0;
                B[i] = 0;
            }
        }
        animationStep += 1 * animationEngine.currentAnimation.cycleMod;
        if (animationStep > ANIMATION_STEPS)
            animationStep = animationEngine.currentAnimation.cycleMod;
        break;
    default:
        Serial.printf("Animation not found!");
        break;
    }
    animationEngine.FXtask.delay(ANIMATION_REFRESH_RATE);

    if (animationEngine.FXtask.isLastIteration()) {
        animationStep = animationEngine.currentAnimation.cycleMod;
        animationEngine.FXtask.setIterations(ANIMATION_STEPS);
        // Calculate delay based on current mesh time
#ifdef DEBUG_TASK

        unsigned long delayDebug = (ANIMATION_CYCLE_LENGTH + 200) - (wlanMeshEngine.getNodeTime() % ((ANIMATION_CYCLE_LENGTH + 200) * 1000)) / 1000;
        Serial.println("Delay: " + String(delayDebug, DEC));
#endif
        // 200 MilliSek addieren (ungefähr so lange dauert diese Funktion)
        animationEngine.FXtask.enableDelayed((ANIMATION_CYCLE_LENGTH + 200) - (wlanMeshEngine.getNodeTime() % ((ANIMATION_CYCLE_LENGTH + 200) * 1000)) / 1000);
    }
}