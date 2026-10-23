
#pragma once

#include <stdint.h>
#include "../core/PetState.h"

enum class PetMood : uint8_t
{
    Happy,
    Focused,
    Sleepy,
    Curious,
    Excited
};

struct PetBehaviorState
{
    PetMood mood;
    const char *message;
};

class PetBehavior
{
public:
    PetBehavior();

    void begin(uint32_t now);
    void update(PetMode mode, uint32_t now);

    const PetBehaviorState &state() const;

private:
    PetBehaviorState _state;

    PetMode _lastMode;
    uint32_t _lastMessageAt;
    uint8_t _workMessageIndex;
    uint8_t _napMessageIndex;
    uint8_t _playMessageIndex;
    bool _initialized;

    void selectMessage(PetMode mode, uint32_t now);
};
