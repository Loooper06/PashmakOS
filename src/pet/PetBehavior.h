#pragma once

#include <stdint.h>
#include "../core/PetState.h"

enum class PetMood : uint8_t
{
    Happy,
    Focused,
    Sleepy,
    Curious,
    Excited,
    Tired,
    Exhausted
};

struct PetBehaviorState
{
    PetMood mood;
    const char *message;
    uint8_t energy;
};

class PetBehavior
{
public:
    explicit PetBehavior(uint8_t personalityId = 0);

    void begin(uint32_t now);
    void update(PetMode mode, uint32_t now);

    const PetBehaviorState &state() const;

private:
    uint8_t _personalityId;
    PetBehaviorState _state;
    const char *_baseMessage;

    PetMode _lastMode;
    uint32_t _lastMessageAt;
    uint32_t _lastEnergyTickAt;
    uint32_t _energyRemainderMs;
    uint32_t _eventUntil;
    uint32_t _nextEventAt;
    uint8_t _workMessageIndex;
    uint8_t _napMessageIndex;
    uint8_t _playMessageIndex;
    uint8_t _eventIndex;
    bool _eventActive;
    bool _initialized;

    void selectMessage(PetMode mode, uint32_t now);
    void updateEnergy(PetMode mode, uint32_t now);
    void updateMood(PetMode mode);
    void updateSpontaneousEvent(uint32_t now);
};