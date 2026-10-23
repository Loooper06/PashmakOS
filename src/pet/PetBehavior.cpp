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

    constexpr uint32_t WORK_ENERGY_STEP_MS = 120000UL;
    constexpr uint32_t PLAY_ENERGY_STEP_MS = 60000UL;
    constexpr uint32_t NAP_ENERGY_STEP_MS = 30000UL;
}

PetBehavior::PetBehavior()
    : _state{PetMood::Focused, WORK_MESSAGES[0], 80},
      _lastMode(PetMode::Work),
      _lastMessageAt(0),
      _lastEnergyTickAt(0),
      _energyRemainderMs(0),
      _workMessageIndex(0),
      _napMessageIndex(0),
      _playMessageIndex(0),
      _initialized(false)
{
}

void PetBehavior::begin(uint32_t now)
{
    _lastMessageAt = now;
    _lastEnergyTickAt = now;
    _energyRemainderMs = 0;
    _lastMode = PetMode::Work;
    _workMessageIndex = 0;
    _napMessageIndex = 0;
    _playMessageIndex = 0;
    _state = {PetMood::Focused, WORK_MESSAGES[0], 80};
    _initialized = true;
}

void PetBehavior::update(PetMode mode, uint32_t now)
{
    if (!_initialized)
    {
        begin(now);
        _lastMode = mode;
        selectMessage(mode, now);
        updateMood(mode);
        return;
    }

    if (mode != _lastMode)
        _energyRemainderMs = 0;

    updateEnergy(mode, now);
    updateMood(mode);

    const bool modeChanged = mode != _lastMode;
    const bool messageDue =
        static_cast<uint32_t>(now - _lastMessageAt) >= Config::PET_MESSAGE_INTERVAL_MS;

    if (modeChanged || messageDue)
    {
        selectMessage(mode, now);
        updateMood(mode);
        _lastMode = mode;
        _lastMessageAt = now;
    }
}

void PetBehavior::updateEnergy(PetMode mode, uint32_t now)
{
    const uint32_t elapsed = static_cast<uint32_t>(now - _lastEnergyTickAt);
    _lastEnergyTickAt = now;
    _energyRemainderMs += elapsed;

    uint32_t stepMs = WORK_ENERGY_STEP_MS;
    int8_t delta = -1;
    if (mode == PetMode::Play)
    {
        stepMs = PLAY_ENERGY_STEP_MS;
        delta = 1;
    }
    else if (mode == PetMode::Nap)
    {
        stepMs = NAP_ENERGY_STEP_MS;
        delta = 1;
    }

    while (_energyRemainderMs >= stepMs)
    {
        _energyRemainderMs -= stepMs;
        if (delta < 0)
        {
            if (_state.energy > 0)
                --_state.energy;
        }
        else if (_state.energy < 100)
        {
            ++_state.energy;
        }
    }

    if ((_state.energy == 0 && delta < 0) ||
        (_state.energy == 100 && delta > 0))
        _energyRemainderMs = 0;
}

void PetBehavior::updateMood(PetMode mode)
{
    if (_state.energy <= 10)
    {
        _state.mood = PetMood::Exhausted;
        return;
    }
    if (_state.energy <= 30)
    {
        _state.mood = PetMood::Tired;
        return;
    }

    switch (mode)
    {
    case PetMode::Work: _state.mood = PetMood::Focused; break;
    case PetMode::Nap: _state.mood = PetMood::Sleepy; break;
    case PetMode::Play: _state.mood = PetMood::Excited; break;
    default: _state.mood = PetMood::Curious; break;
    }
}

void PetBehavior::selectMessage(PetMode mode, uint32_t now)
{
    (void)now;

    if (_state.energy <= 10)
    {
        _state.message = "I need a longer rest.";
        return;
    }
    if (_state.energy <= 30)
    {
        _state.message = "I'm getting tired.";
        return;
    }

    switch (mode)
    {
    case PetMode::Work:
        _state.message = WORK_MESSAGES[_workMessageIndex % WORK_MESSAGE_COUNT];
        _workMessageIndex++;
        break;
    case PetMode::Nap:
        _state.message = NAP_MESSAGES[_napMessageIndex % NAP_MESSAGE_COUNT];
        _napMessageIndex++;
        break;
    case PetMode::Play:
        _state.message = PLAY_MESSAGES[_playMessageIndex % PLAY_MESSAGE_COUNT];
        _playMessageIndex++;
        break;
    default:
        _state.message = "Hello, Engineer!";
        _state.mood = PetMood::Curious;
        break;
    }
}

const PetBehaviorState &PetBehavior::state() const
{
    return _state;
}
