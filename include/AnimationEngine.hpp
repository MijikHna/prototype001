#ifndef __ANIMATION_ENGINE__
#define __ANIMATION_ENGINE__

#include "WlanMeshEngine.hpp"
#include "cppQueue.h"
#include "definitions.hpp"

class AnimationEngine {
private:
public:
    effect currentAnimation;
    cppQueue q;
    Task FXtask;
    //Konstruktoren
    AnimationEngine();

    //getter + setter
    cppQueue getQ(void);
    effect getCurrentAnimation(void);
    void setCurrrentAnimation(effect newEffect);
    
    void addNextAnimation(effect newEffect);

    //helper methods
    static void run(void* animationEngineContext);
};
#endif