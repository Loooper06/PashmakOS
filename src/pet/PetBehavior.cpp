#include "PetBehavior.h"
#include "Config.h"

namespace
{
    struct MessageProfile
    {
        const char *const *work;
        uint8_t workCount;
        const char *const *nap;
        uint8_t napCount;
        const char *const *play;
        uint8_t playCount;
        const char *const *spontaneous;
        uint8_t spontaneousCount;
    };

    const char *const PASHMAK_WORK[] = {
        "Let's Focus, Engineer!", "Step by Step Is Better",
        "Improving Every Day!", "Relax, Engineer!", "Don't Forget to Blink!"};
    const char *const PASHMAK_NAP[] = {
        "Time for a Proper Break.", "Give Your Eyes Some Rest.",
        "Take a Deep Breath.", "Stand Up and Stretch a Little."};
    const char *const PASHMAK_PLAY[] = {
        "Let's Get Moving!", "Stretch Your Neck Gently.",
        "Time to Drink Some Water!", "A Short Walk Would Be Nice."};
    const char *const PASHMAK_EVENTS[] = {
        "Hey! Still there, Engineer?", "A tiny break can help.",
        "How about a sip of water?", "You're making progress!",
        "Remember to relax your shoulders.", "One step at a time."};

    const char *const MOCHI_WORK[] = {
        "Slow and steady, okay?", "You are doing enough.",
        "Let's make this a gentle session.", "Breathe; one thing at a time."};
    const char *const MOCHI_NAP[] = {
        "A cozy little pause.", "Rest is part of the work.",
        "Let's soften the pace.", "Close your eyes for a moment."};
    const char *const MOCHI_PLAY[] = {
        "A little stretch together?", "Let's take a gentle walk.",
        "Water break, friend?", "Move a little, no rush."};
    const char *const MOCHI_EVENTS[] = {
        "Just checking in, friend.", "It's okay to pause.",
        "Your pace is your pace.", "A sip of water sounds nice."};

    const char *const PIXEL_WORK[] = {
        "Task mode: ON.", "Tiny steps. Big progress.",
        "Checkpoint reached!", "Focus.exe is running."};
    const char *const PIXEL_NAP[] = {
        "Power-saving mode activated.", "Checkpoint: rest.",
        "Buffering... take a breath.", "System cool-down time."};
    const char *const PIXEL_PLAY[] = {
        "Movement bonus unlocked!", "Side quest: stretch.",
        "Hydration achievement available.", "Let's collect some fresh air."};
    const char *const PIXEL_EVENTS[] = {
        "Ping! Still with me?", "Achievement: take a sip of water.",
        "Micro-break detected.", "One small commit at a time."};

    constexpr MessageProfile PROFILES[] = {
        {PASHMAK_WORK, 5, PASHMAK_NAP, 4, PASHMAK_PLAY, 4, PASHMAK_EVENTS, 6},
        {MOCHI_WORK, 4, MOCHI_NAP, 4, MOCHI_PLAY, 4, MOCHI_EVENTS, 4},
        {PIXEL_WORK, 4, PIXEL_NAP, 4, PIXEL_PLAY, 4, PIXEL_EVENTS, 4}};
    constexpr uint8_t PROFILE_COUNT = sizeof(PROFILES) / sizeof(PROFILES[0]);

    constexpr uint32_t WORK_ENERGY_STEP_MS = 120000UL;
    constexpr uint32_t PLAY_ENERGY_STEP_MS = 60000UL;
    constexpr uint32_t NAP_ENERGY_STEP_MS = 30000UL;
    constexpr uint32_t EVENT_DURATION_MS = 7000UL;
    constexpr uint32_t EVENT_MIN_INTERVAL_MS = 90000UL;
    constexpr uint32_t EVENT_INTERVAL_VARIATION_MS = 15000UL;
}

PetBehavior::PetBehavior(uint8_t personalityId)
    : _personalityId(personalityId < PROFILE_COUNT ? personalityId : 0),
      _state{PetMood::Focused, PASHMAK_WORK[0], 80},
      _baseMessage(PASHMAK_WORK[0]),
      _lastMode(PetMode::Work),
      _lastMessageAt(0),
      _lastEnergyTickAt(0),
      _energyRemainderMs(0),
      _eventUntil(0),
      _nextEventAt(0),
      _workMessageIndex(0),
      _napMessageIndex(0),
      _playMessageIndex(0),
      _eventIndex(0),
      _eventActive(false),
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
    _eventIndex = 0;
    _eventActive = false;
    _eventUntil = now;
    _nextEventAt = now + EVENT_MIN_INTERVAL_MS;
    const MessageProfile &profile = PROFILES[_personalityId];
    _baseMessage = profile.work[0];
    _state = {PetMood::Focused, _baseMessage, 80};
    _initialized = true;
}

void PetBehavior::update(PetMode mode, uint32_t now, int8_t hour)
{
    if (!_initialized)
    {
        begin(now);
        _lastMode = mode;
        selectMessage(mode, now, hour);
        updateMood(mode);
        return;
    }

    const bool modeChanged = mode != _lastMode;
    if (modeChanged)
    {
        // Charge elapsed time to the mode that was active during that interval.
        updateEnergy(_lastMode, now);
        // Fractional time from one mode must not leak into another mode's rate.
        _energyRemainderMs = 0;
        _lastEnergyTickAt = now;
    }

    updateEnergy(mode, now);
    updateMood(mode);
    const bool messageDue =
        static_cast<uint32_t>(now - _lastMessageAt) >= Config::PET_MESSAGE_INTERVAL_MS;

    if (modeChanged || messageDue)
    {
        selectMessage(mode, now, hour);
        updateMood(mode);
        _lastMode = mode;
        _lastMessageAt = now;
    }

    updateSpontaneousEvent(now);
}

void PetBehavior::updateEnergy(PetMode mode, uint32_t now)
{
    const uint32_t elapsed = static_cast<uint32_t>(now - _lastEnergyTickAt);
    _lastEnergyTickAt = now;
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

    // Use a wider accumulator so a delayed update cannot overflow when
    // elapsed time is large (for example after a long pause or debugger stop).
    const uint64_t accumulatedMs =
        static_cast<uint64_t>(_energyRemainderMs) + elapsed;
    const uint64_t steps = accumulatedMs / stepMs;
    _energyRemainderMs = static_cast<uint32_t>(accumulatedMs % stepMs);

    if (delta < 0)
    {
        _state.energy = steps >= _state.energy
                            ? 0
                            : static_cast<uint8_t>(_state.energy - steps);
    }
    else
    {
        const uint8_t missingEnergy = static_cast<uint8_t>(100 - _state.energy);
        _state.energy = steps >= missingEnergy
                            ? 100
                            : static_cast<uint8_t>(_state.energy + steps);
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

void PetBehavior::selectMessage(PetMode mode, uint32_t now, int8_t hour)
{
    (void)now;

    const MessageProfile &profile = PROFILES[_personalityId];
    // Add occasional time-aware lines without replacing each pet's personality.
    const bool timeMessage = hour >= 0 && hour <= 23 &&
                             (_workMessageIndex + _napMessageIndex + _playMessageIndex) % 3 == 0;
    if (_state.energy <= 10)
    {
        _baseMessage = "I need a longer rest.";
    }
    else if (_state.energy <= 30)
    {
        _baseMessage = "I'm getting tired.";
    }
    else if (timeMessage && hour >= 6 && hour < 11)
    {
        switch (_personalityId)
        {
        case 1: _baseMessage = "Good morning. Take it gently."; break;
        case 2: _baseMessage = "Boot sequence: good morning!"; break;
        default: _baseMessage = "Good morning, Engineer!"; break;
        }
    }
    else if (timeMessage && hour >= 21)
    {
        switch (_personalityId)
        {
        case 1: _baseMessage = "It's late. Be kind to yourself."; break;
        case 2: _baseMessage = "Night mode recommended."; break;
        default: _baseMessage = "Late session? Remember to rest."; break;
        }
    }
    else if (timeMessage && hour >= 12 && hour < 14)
    {
        switch (_personalityId)
        {
        case 1: _baseMessage = "A calm midday pause is welcome."; break;
        case 2: _baseMessage = "Midday checkpoint."; break;
        default: _baseMessage = "A little midday reset helps."; break;
        }
    }
    else
    {
        switch (mode)
        {
        case PetMode::Work:
            _baseMessage = profile.work[_workMessageIndex % profile.workCount];
            ++_workMessageIndex;
            break;
        case PetMode::Nap:
            _baseMessage = profile.nap[_napMessageIndex % profile.napCount];
            ++_napMessageIndex;
            break;
        case PetMode::Play:
            _baseMessage = profile.play[_playMessageIndex % profile.playCount];
            ++_playMessageIndex;
            break;
        default:
            _baseMessage = "Hello, Engineer!";
            _state.mood = PetMood::Curious;
            break;
        }
    }

    if (!_eventActive)
        _state.message = _baseMessage;
}

void PetBehavior::updateSpontaneousEvent(uint32_t now)
{
    // Signed subtraction keeps deadline checks safe across millis() rollover.
    if (_eventActive && static_cast<int32_t>(now - _eventUntil) >= 0)
    {
        _eventActive = false;
        _state.message = _baseMessage;
    }

    if (_eventActive || static_cast<int32_t>(now - _nextEventAt) < 0)
        return;

    // Avoid cheerful interruptions when the pet is exhausted.
    if (_state.energy <= 10)
    {
        _nextEventAt = now + EVENT_MIN_INTERVAL_MS;
        return;
    }

    const MessageProfile &profile = PROFILES[_personalityId];
    _state.message = profile.spontaneous[_eventIndex % profile.spontaneousCount];
    ++_eventIndex;
    _eventActive = true;
    _eventUntil = now + EVENT_DURATION_MS;
    _nextEventAt = now + EVENT_MIN_INTERVAL_MS +
                   static_cast<uint32_t>(_eventIndex % 4) * EVENT_INTERVAL_VARIATION_MS;
}

const PetBehaviorState &PetBehavior::state() const
{
    return _state;
}
