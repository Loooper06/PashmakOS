#include "PetBehavior.h"

namespace
{
    constexpr uint32_t MESSAGE_INTERVAL_MS = 45000;

    const char *const WORK_MESSAGES[] = {
        "Focus, engineer!",
        "One step at a time.",
        "You are making progress!",
        "Relax your shoulders.",
        "Remember to blink."};

    const char *const NAP_MESSAGES[] = {
        "Time for a real break.",
        "Rest your eyes, engineer.",
        "Take a slow breath.",
        "Get up and stretch a little."};

    const char *const PLAY_MESSAGES[] = {
        "Let's move a little!",
        "Stretch your neck gently.",
        "Water break, engineer!",
        "A short walk sounds good."};

    constexpr uint8_t WORK_MESSAGE_COUNT =
        sizeof(WORK_MESSAGES) / sizeof(WORK_MESSAGES[0]);
    constexpr uint8_t NAP_MESSAGE_COUNT =
        sizeof(NAP_MESSAGES) / sizeof(NAP_MESSAGES[0]);
    constexpr uint8_t PLAY_MESSAGE_COUNT =
        sizeof(PLAY_MESSAGES) / sizeof(PLAY_MESSAGES[0]);
}

PetBehavior::PetBehavior()
    : _state{PetMood::Focused, WORK_MESSAGES[0]},
      _lastMode(PetMode::Work),
      _lastMessageAt(0),
      _messageIndex(0),
      _initialized(false)
{
}

void PetBehavior::begin(uint32_t now)
{
    _lastMessageAt = now;
    _lastMode = PetMode::Work;
    _messageIndex = 0;
    _state = {PetMood::Focused, WORK_MESSAGES[0]};
    _initialized = true;
}

void PetBehavior::update(PetMode mode, uint32_t now)
{
    if (!_initialized)
    {
        begin(now);
        _lastMode = mode;
        selectMessage(mode, now);
        return;
    }

    const bool modeChanged = mode != _lastMode;
    const bool messageDue =
        static_cast<uint32_t>(now - _lastMessageAt) >= MESSAGE_INTERVAL_MS;

    if (modeChanged || messageDue)
    {
        selectMessage(mode, now);
        _lastMode = mode;
        _lastMessageAt = now;
    }
}

void PetBehavior::selectMessage(PetMode mode, uint32_t now)
{
    (void)now;

    switch (mode)
    {
    case PetMode::Work:
        _state.mood = PetMood::Focused;
        _state.message = WORK_MESSAGES[_messageIndex % WORK_MESSAGE_COUNT];
        _messageIndex++;
        break;

    case PetMode::Nap:
        _state.mood = PetMood::Sleepy;
        _state.message = NAP_MESSAGES[_messageIndex % NAP_MESSAGE_COUNT];
        _messageIndex++;
        break;

    case PetMode::Play:
        _state.mood = PetMood::Excited;
        _state.message = PLAY_MESSAGES[_messageIndex % PLAY_MESSAGE_COUNT];
        _messageIndex++;
        break;

    default:
        _state.mood = PetMood::Curious;
        _state.message = "Hello, engineer!";
        break;
    }
}

const PetBehaviorState &PetBehavior::state() const
{
    return _state;
}
