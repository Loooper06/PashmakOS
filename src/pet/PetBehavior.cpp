#include "PetBehavior.h"
#include "Config.h"

namespace
{
    const char *const WORK_MESSAGES[] = {
        "Let's Focus, Engineer!",
        "Step by Step Is Better",
        "Improving Every Day!",
        "Relax, Engineer!",
        "Don't Forget to Blink!"};

    const char *const NAP_MESSAGES[] = {
        "Time for a Proper Break.",
        "Give Your Eyes Some Rest.",
        "Take a Deep Breath.",
        "Stand Up and Stretch a Little."};

    const char *const PLAY_MESSAGES[] = {
        "Let's Get Moving!",
        "Stretch Your Neck Gently.",
        "Time to Drink Some Water!",
        "A Short Walk Would Be Nice."};

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
      _workMessageIndex(0),
      _napMessageIndex(0),
      _playMessageIndex(0),
      _initialized(false)
{
}

void PetBehavior::begin(uint32_t now)
{
    _lastMessageAt = now;
    _lastMode = PetMode::Work;
    _workMessageIndex = 0;
    _napMessageIndex = 0;
    _playMessageIndex = 0;
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
        static_cast<uint32_t>(now - _lastMessageAt) >= Config::PET_MESSAGE_INTERVAL_MS;

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
        _state.message = WORK_MESSAGES[_workMessageIndex % WORK_MESSAGE_COUNT];
        _workMessageIndex++;
        break;

    case PetMode::Nap:
        _state.mood = PetMood::Sleepy;
        _state.message = NAP_MESSAGES[_napMessageIndex % NAP_MESSAGE_COUNT];
        _napMessageIndex++;
        break;

    case PetMode::Play:
        _state.mood = PetMood::Excited;
        _state.message = PLAY_MESSAGES[_playMessageIndex % PLAY_MESSAGE_COUNT];
        _playMessageIndex++;
        break;

    default:
        _state.mood = PetMood::Curious;
        _state.message = "Hello, Engineer!";
        break;
    }
}

const PetBehaviorState &PetBehavior::state() const
{
    return _state;
}
