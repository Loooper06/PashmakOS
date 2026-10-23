#include "PashmakApp.h"
#include "Config.h"

namespace
{
    const char *modeName(PetMode mode)
    {
        switch (mode)
        {
        case PetMode::Work: return "Work";
        case PetMode::Nap: return "Nap";
        case PetMode::Play: return "Play";
        default: return "Unknown";
        }
    }

    constexpr uint32_t WORK_SESSION_MS = 25UL * 60UL * 1000UL;
    constexpr uint32_t PLAY_SESSION_MS = 2UL * 60UL * 1000UL;
    constexpr uint32_t NAP_SESSION_MS = 5UL * 60UL * 1000UL;
    constexpr uint32_t REMINDER_DISPLAY_MS = 12000UL;
    constexpr uint8_t WORK_CYCLES_BEFORE_NAP = 3;
}

PashmakApp::PashmakApp() = default;

void PashmakApp::begin()
{
    Serial.begin(115200);
    _display.begin();

    const uint32_t now = millis();
    _lastWorkTick = now;
    _lastFrame = now;
    _modeStartedAt = now;

    _behavior.begin(now);
    _rtc.begin();

    _renderer.draw(
        _display,
        _mode,
        _frame,
        _behavior.state().message);

    Serial.println("PashmakOS: automatic pet behavior enabled");
}

void PashmakApp::setMode(PetMode mode, uint32_t now)
{
    if (_mode == mode)
        return;

    _mode = mode;
    _modeStartedAt = now;
    Serial.printf("[PET] Mode changed: %s\n", modeName(_mode));
}

void PashmakApp::updateAutomaticMode(uint32_t now)
{
    const uint32_t modeElapsed = static_cast<uint32_t>(now - _modeStartedAt);

    switch (_mode)
    {
    case PetMode::Work:
        if (modeElapsed >= WORK_SESSION_MS)
        {
            ++_completedWorkCycles;

            if (_completedWorkCycles >= WORK_CYCLES_BEFORE_NAP)
            {
                _completedWorkCycles = 0;
                _longRestPending = true;
                setMode(PetMode::Nap, now);
            }
            else
            {
                setMode(PetMode::Play, now);
            }
        }
        break;

    case PetMode::Play:
        if (modeElapsed >= PLAY_SESSION_MS)
            setMode(PetMode::Work, now);
        break;

    case PetMode::Nap:
        if (modeElapsed >= NAP_SESSION_MS)
        {
            _longRestPending = false;
            setMode(PetMode::Work, now);
        }
        break;
    }
}

void PashmakApp::updateRtcReminder(uint32_t now)
{
    const ReminderType reminder = _rtc.update(now);
    if (reminder == ReminderType::None)
        return;

    switch (reminder)
    {
    case ReminderType::DrinkWater:
        _reminderMessage = "Time to drink water!";
        Serial.println("[RTC] Reminder: drink water.");
        break;

    case ReminderType::MoveBody:
        _reminderMessage = "Stand up and stretch.";
        Serial.println("[RTC] Reminder: stand up and stretch.");
        break;

    case ReminderType::Compliment:
        _reminderMessage = "You look great today!";
        Serial.println("[RTC] Reminder: daily compliment.");
        break;

    case ReminderType::None:
    default:
        return;
    }

    _reminderShownAt = now;
}

void PashmakApp::update()
{
    const uint32_t now = millis();

    if (_mode == PetMode::Work)
        _workMs += static_cast<uint32_t>(now - _lastWorkTick);
    _lastWorkTick = now;

    updateAutomaticMode(now);
    _behavior.update(_mode, now);
    updateRtcReminder(now);

    if (_reminderMessage != nullptr &&
        static_cast<uint32_t>(now - _reminderShownAt) >= REMINDER_DISPLAY_MS)
    {
        _reminderMessage = nullptr;
    }

    if (static_cast<uint32_t>(now - _lastFrame) >= Config::FRAME_INTERVAL_MS)
    {
        ++_frame;
        const char *message = _reminderMessage != nullptr
                                  ? _reminderMessage
                                  : _behavior.state().message;

        _renderer.draw(_display, _mode, _frame, message);
        _lastFrame = now;
    }
}
