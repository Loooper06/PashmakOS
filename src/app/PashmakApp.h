
#pragma once

#include <Arduino.h>
#include "../core/PetState.h"
#include "../input/Button.h"
#include "../display/Display.h"
#include "../pet/PetRenderer.h"
#include "../pet/PetBehavior.h"

class PashmakApp
{
public:
    PashmakApp();
    
    void begin();
    void update();

private:
    Display _display;
    PetRenderer _renderer;
    PetBehavior _behavior;

    Button _modeButton;
    Button _napButton;
    Button _playButton;

    PetMode _mode = PetMode::Work;

    uint32_t _lastFrame = 0;
    uint32_t _frame = 0;
    uint32_t _workMs = 0;
    uint32_t _lastWorkTick = 0;

    void setMode(PetMode mode);
};
