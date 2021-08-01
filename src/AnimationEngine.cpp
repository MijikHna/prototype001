#include "AnimationEngine.hpp"
#include "WlanMeshEngine.hpp"
#include "definitions.hpp"

#include "cppQueue.h"

// #define DEBUG 1
// #define DEBUG_TASK 1
// #define DEBUG_CORE 1

extern uint8_t R[LED_COUNT], G[LED_COUNT], B[LED_COUNT];
extern WlanMeshEngine wlanMeshEngine;

void taskCallback();

AnimationEngine::AnimationEngine()
    : q(sizeof(effect), 10, FIFO)
{
    effect startUp = { 101, 0.0, 255.0, 0.0, 1 };

    q.push(&startUp);

    this->FXtask.set(ANIMATION_CYCLE_LENGTH,
        ANIMATION_STEPS,
        taskCallback);
}

//getter + setter
cppQueue AnimationEngine::getQ(void)
{
    return this->q;
}

effect AnimationEngine::getCurrentAnimation(void)
{
    return this->currentAnimation;
}

void AnimationEngine::setCurrrentAnimation(effect newEffect)
{
    this->currentAnimation = newEffect;
}

void AnimationEngine::addNextAnimation(effect newEffect)
{
    this->q.push(&newEffect);
    if (wlanMeshEngine.getCurrentState() == WLAN_MESH_STATE::MASTER) {
        wlanMeshEngine.broadcastEffect(newEffect);
    }
#ifdef DEBUG
    Serial.println("pushed" + String(newEffect.fxId, DEC));
#endif
}

void AnimationEngine::run(void* animationEngineContext)
{
    Scheduler* animationEngineScheduler = (Scheduler*)animationEngineContext;

    while (1) {
        animationEngineScheduler->execute();
    }
}