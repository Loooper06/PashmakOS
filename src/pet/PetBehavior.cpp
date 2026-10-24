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
        uint32_t workEnergyStepMs;
        uint32_t playEnergyStepMs;
        uint32_t napEnergyStepMs;
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

    // Personality profiles control both voice and energy dynamics.
    // Pashmak is balanced, Mochi recovers gently, and Pixel is more energetic.
    constexpr MessageProfile PROFILES[] = {
        {PASHMAK_WORK, 5, PASHMAK_NAP, 4, PASHMAK_PLAY, 4, PASHMAK_EVENTS, 6,
         120000UL, 60000UL, 30000UL},
        {MOCHI_WORK, 4, MOCHI_NAP, 4, MOCHI_PLAY, 4, MOCHI_EVENTS, 4,
         150000UL, 90000UL, 25000UL},
        {PIXEL_WORK, 4, PIXEL_NAP, 4, PIXEL_PLAY, 4, PIXEL_EVENTS, 4,
         90000UL, 45000UL, 40000UL}};
    constexpr uint8_t PROFILE_COUNT = sizeof(PROFILES) / sizeof(PROFILES[0]);
    constexpr uint32_t EVENT_DURATION_MS = 7000UL;
    constexpr uint32_t EVENT_MIN_INTERVAL_MS = 90000UL;
    constexpr uint32_t EVENT_INTERVAL_VARIATION_MS = 15000UL;
}

PetBehavior::PetBehavior(uint8_t personalityId)
    : _personalityId(personalityId < PROFILE_COUNT ? personalityId : 0),
      _state{PetMood::Focused, PASHMAK_WORK[0], 80, 70},
      _baseMessage(PASHMAK_WORK[0]),
      _lastBaseMessage(nullptr),
      _lastMode(PetMode::Work),
      _lastMessageAt(0),
      _lastEnergyTickAt(0),
      _energyRemainderMs(0),
      _lastHappinessTickAt(0),
      _happinessRemainderMs(0),
      _eventUntil(0),
      _nextEventAt(0),
      _workMessageIndex(0),
      _napMessageIndex(0),
      _playMessageIndex(0),
      _eventIndex(0),
      _eventActive(false),
      _interactionActive(false),
      _interactionUntil(0),
      _interactionMessage(nullptr),
      _initialized(false)
{
}

void PetBehavior::begin(uint32_t now)
{
    _lastMessageAt = now;
    _lastEnergyTickAt = now;
    _energyRemainderMs = 0;
    _lastHappinessTickAt = now;
    _happinessRemainderMs = 0;
    _lastMode = PetMode::Work;
    _workMessageIndex = 0;
    _napMessageIndex = 0;
    _playMessageIndex = 0;
    _eventIndex = 0;
    _eventActive = false;
    _interactionActive = false;
    _interactionUntil = now;
    _interactionMessage = nullptr;
    _eventUntil = now;
    _nextEventAt = now + EVENT_MIN_INTERVAL_MS;
    const MessageProfile &profile = PROFILES[_personalityId];
    _baseMessage = profile.work[0];
    _lastBaseMessage = nullptr;
    _state = {PetMood::Focused, _baseMessage, 80, 70};
    _initialized = true;
}

bool PetBehavior::restoreEnergy(uint8_t energy, uint32_t now)
{
    if (energy > 100)
        return false;

    _state.energy = energy;
    _lastEnergyTickAt = now;
    _energyRemainderMs = 0;
    updateMood(_lastMode);
    return true;
}

void PetBehavior::update(PetMode mode, uint32_t now, int8_t hour, bool quietHours)
{
    if (!_initialized)
    {
        begin(now);
        _lastMode = mode;
        selectMessage(mode, now, hour, quietHours);
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
    updateHappiness(mode, now);
    updateMood(mode);
    const bool messageDue =
        static_cast<uint32_t>(now - _lastMessageAt) >= Config::PET_MESSAGE_INTERVAL_MS;

    if (modeChanged || messageDue)
    {
        selectMessage(mode, now, hour, quietHours);
        updateMood(mode);
        _lastMode = mode;
        _lastMessageAt = now;
    }

    updateSpontaneousEvent(mode, now, quietHours);

    if (_interactionActive)
    {
        if (static_cast<int32_t>(now - _interactionUntil) >= 0)
        {
            _interactionActive = false;
            _state.message = _baseMessage;
        }
        else if (mode == PetMode::Nap)
        {
            _state.mood = PetMood::Happy;
        }
        else if (_state.energy > 30)
        {
            _state.mood = PetMood::Excited;
        }
    }
    if (_interactionActive)
    {
        // Keep the direct interaction reaction visible above spontaneous events.
        _state.message = _interactionMessage != nullptr ? _interactionMessage : _baseMessage;
    }
}

void PetBehavior::updateEnergy(PetMode mode, uint32_t now)
{
    const uint32_t elapsed = static_cast<uint32_t>(now - _lastEnergyTickAt);
    _lastEnergyTickAt = now;
    const MessageProfile &profile = PROFILES[_personalityId];
    uint32_t stepMs = profile.workEnergyStepMs;
    int8_t delta = -1;
    if (mode == PetMode::Play)
    {
        stepMs = profile.playEnergyStepMs;
        delta = 1;
    }
    else if (mode == PetMode::Nap)
    {
        stepMs = profile.napEnergyStepMs;
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

void PetBehavior::updateHappiness(PetMode mode, uint32_t now)
{
    const uint32_t elapsed = static_cast<uint32_t>(now - _lastHappinessTickAt);
    _lastHappinessTickAt = now;
    uint32_t stepMs = 60000UL;
    int8_t delta = 0;
    if (mode == PetMode::Work) { stepMs = 300000UL; delta = -1; }
    else if (mode == PetMode::Play) { stepMs = 60000UL; delta = 2; }
    else if (mode == PetMode::Nap) { stepMs = 90000UL; delta = 1; }
    if (delta == 0) { _happinessRemainderMs = 0; return; }
    const uint64_t accumulatedMs = static_cast<uint64_t>(_happinessRemainderMs) + elapsed;
    const uint64_t steps = accumulatedMs / stepMs;
    _happinessRemainderMs = static_cast<uint32_t>(accumulatedMs % stepMs);
    if (delta < 0)
        _state.happiness = steps >= _state.happiness ? 0 : static_cast<uint8_t>(_state.happiness - steps);
    else {
        const uint8_t missing = static_cast<uint8_t>(100 - _state.happiness);
        const uint64_t gain = steps * static_cast<uint8_t>(delta);
        _state.happiness = gain >= missing ? 100 : static_cast<uint8_t>(_state.happiness + gain);
    }
    if ((_state.happiness == 0 && delta < 0) || (_state.happiness == 100 && delta > 0)) _happinessRemainderMs = 0;
}

bool PetBehavior::restoreHappiness(uint8_t happiness, uint32_t now)
{
    if (happiness > 100) return false;
    _state.happiness = happiness;
    _lastHappinessTickAt = now;
    _happinessRemainderMs = 0;
    return true;
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

void PetBehavior::selectMessage(PetMode mode, uint32_t now, int8_t hour, bool quietHours)
{
    (void)now;

    const MessageProfile &profile = PROFILES[_personalityId];
    // Time-aware messages are context-sensitive: never override quiet hours or Nap.
    const bool timeMessage = !quietHours && mode != PetMode::Nap &&
                             hour >= 0 && hour <= 23 &&
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
        if (mode == PetMode::Play)
        {
            switch (_personalityId)
            {
            case 1: _baseMessage = "Good morning, friend. A gentle stretch?"; break;
            case 2: _baseMessage = "Morning side quest unlocked!"; break;
            default: _baseMessage = "Good morning! Ready for a little movement?"; break;
            }
        }
        else
        {
            switch (_personalityId)
            {
            case 1: _baseMessage = "Good morning. Take it gently."; break;
            case 2: _baseMessage = "Boot sequence: good morning!"; break;
            default: _baseMessage = "Good morning, Engineer!"; break;
            }
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

    // Do not let repeated time-aware lines suppress low-energy warnings.
    if (_state.energy > 30 && timeMessage && _baseMessage == _lastBaseMessage)
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
            break;
        }
    }

    _lastBaseMessage = _baseMessage;
    if (!_eventActive)
        _state.message = _baseMessage;
}

void PetBehavior::interact(PetMode mode, uint32_t now, uint8_t bond, PetInteraction interaction)
{
    _interactionActive = true;
    _interactionUntil = now + 6000UL;
    const uint8_t happinessGain = interaction == PetInteraction::Playful ? 12 : (interaction == PetInteraction::Affection ? 8 : 5);
    const uint16_t nextHappiness = static_cast<uint16_t>(_state.happiness) + happinessGain;
    _state.happiness = nextHappiness > 100U ? 100U : static_cast<uint8_t>(nextHappiness);

    // Typed actions provide visibly different responses without changing the
    // existing energy model or adding flash writes for every interaction.
    if (interaction == PetInteraction::Playful)
    {
        switch (_personalityId)
        {
        case 1: _interactionMessage = "A tiny game? Count me in!"; break;
        case 2: _interactionMessage = "Mini-game initialized!"; break;
        default: _interactionMessage = "Let's play together!"; break;
        }
        _state.message = _interactionMessage;
        _state.mood = _state.energy <= 10 ? PetMood::Tired : PetMood::Excited;
        return;
    }

    if (interaction == PetInteraction::Comfort)
    {
        switch (_personalityId)
        {
        case 1: _interactionMessage = "I'm here with you, friend."; break;
        case 2: _interactionMessage = "Calm mode engaged."; break;
        default: _interactionMessage = "A little comfort, just for you."; break;
        }
        _state.message = _interactionMessage;
        _state.mood = mode == PetMode::Nap ? PetMood::Sleepy : PetMood::Happy;
        return;
    }

    if (mode == PetMode::Nap)
    {
        switch (_personalityId)
        {
        case 1: _interactionMessage = "A gentle hello, friend."; break;
        case 2: _interactionMessage = "Quiet ping received."; break;
        default: _interactionMessage = "A soft hello back."; break;
        }
        _state.message = _interactionMessage;
        _state.mood = PetMood::Happy;
        return;
    }

    // Relationship tiers make each persisted bond milestone visible in the
    // pet's own voice. Boundaries match the dashboard labels.
    const uint8_t tier = bond >= 75 ? 3 : bond >= 50 ? 2 : bond >= 25 ? 1 : 0;
    switch (_personalityId)
    {
    case 1:
        switch (tier)
        {
        case 3: _interactionMessage = "My favorite human!"; break;
        case 2: _interactionMessage = "Our bond feels cozy."; break;
        case 1: _interactionMessage = "You're becoming a familiar face!"; break;
        default: _interactionMessage = "Thanks for being here!"; break;
        }
        break;
    case 2:
        switch (tier)
        {
        case 3: _interactionMessage = "Best-friend protocol!"; break;
        case 2: _interactionMessage = "Trust level: strong."; break;
        case 1: _interactionMessage = "Familiar human detected."; break;
        default: _interactionMessage = "Bond++: friend detected!"; break;
        }
        break;
    default:
        switch (tier)
        {
        case 3: _interactionMessage = "You're my best friend!"; break;
        case 2: _interactionMessage = "We make a great team!"; break;
        case 1: _interactionMessage = "I'm happy to see you again!"; break;
        default: _interactionMessage = "Happy tail wag for you!"; break;
        }
        break;
    }
    _state.message = _interactionMessage;
    if (_state.energy > 30)
        _state.mood = PetMood::Excited;
    else if (_state.energy > 10)
        _state.mood = PetMood::Happy;
}

void PetBehavior::updateSpontaneousEvent(PetMode mode, uint32_t now, bool quietHours)
{
    // Quiet hours immediately suppress an already visible spontaneous event too.
    if (quietHours)
    {
        if (_eventActive)
        {
            _eventActive = false;
            if (!_interactionActive)
                _state.message = _baseMessage;
        }
        _nextEventAt = now + EVENT_MIN_INTERVAL_MS;
        return;
    }

    // Signed subtraction keeps deadline checks safe across millis() rollover.
    if (_eventActive && static_cast<int32_t>(now - _eventUntil) >= 0)
    {
        _eventActive = false;
        _state.message = _baseMessage;
    }

    if (_eventActive || static_cast<int32_t>(now - _nextEventAt) < 0)
        return;

    // Nap is a quiet state: do not start a spontaneous interruption while the
    // pet is resting. Schedule the next opportunity from now so an overdue
    // event cannot fire immediately when Nap ends.
    if (mode == PetMode::Nap)
    {
        _nextEventAt = now + EVENT_MIN_INTERVAL_MS;
        return;
    }

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
