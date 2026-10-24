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
    bool restoreEnergy(uint8_t energy, uint32_t now);
    void update(PetMode mode, uint32_t now, int8_t hour = -1, bool quietHours = false);
    void interact(PetMode mode, uint32_t now, uint8_t bond);

    const PetBehaviorState &state() const;

private:
    uint8_t _personalityId;
    PetBehaviorState _state;
    const char *_baseMessage;
    const char *_lastBaseMessage;

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
    bool _interactionActive;
    uint32_t _interactionUntil;
    const char *_interactionMessage;
    bool _initialized;

    void selectMessage(PetMode mode, uint32_t now, int8_t hour);
    void updateEnergy(PetMode mode, uint32_t now);
    void updateMood(PetMode mode);
    void updateSpontaneousEvent(PetMode mode, uint32_t now, bool quietHours);
};