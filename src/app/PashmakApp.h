#pragma once

#include <Arduino.h>
#include "../core/PetState.h"
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

    PetMode _mode = PetMode::Work;

    uint32_t _lastFrame = 0;
    uint32_t _frame = 0;
    uint32_t _workMs = 0;
    uint32_t _lastWorkTick = 0;
    uint32_t _modeStartedAt = 0;

    uint8_t _completedWorkCycles = 0;
    bool _longRestPending = false;

    void setMode(PetMode mode, uint32_t now);
    void updateAutomaticMode(uint32_t now);
};
