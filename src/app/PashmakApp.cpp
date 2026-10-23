#include "PashmakApp.h"
#include "Config.h"
#include <Preferences.h>

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
    _lastStatsPersistAt = now;

    _statsReady = _statsPreferences.begin("petstats", false);
    if (_statsReady)
    {
        _workMs = _statsPreferences.getULong64("workMs", 0);
        _playMs = _statsPreferences.getULong64("playMs", 0);
        _napMs = _statsPreferences.getULong64("napMs", 0);
        _completedWorkSessions = _statsPreferences.getUInt("workSessions", 0);
        Serial.println("[STATS] Persistent session statistics loaded.");
    }
    else
    {
        Serial.println("[STATS] Preferences unavailable; statistics are RAM-only.");
    }

    _behavior.begin(now);
    _rtc.begin();

    _renderer.draw(
        _display,
        _mode,
        _frame,
        _behavior.state().message,
        nullptr);

    Serial.println("PashmakOS: automatic pet behavior enabled");
}

void PashmakApp::setMode(PetMode mode, uint32_t now)
{
    if (_mode == mode)
        return;

    if (_mode == PetMode::Work)
        ++_completedWorkSessions;

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
        if (modeElapsed >= Config::WORK_SESSION_DURATION_MS)
        {
            ++_completedWorkCycles;

            if (_completedWorkCycles >= Config::WORK_CYCLES_BEFORE_LONG_REST)
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
        if (modeElapsed >= Config::PLAY_SESSION_DURATION_MS)
            setMode(PetMode::Work, now);
        break;

    case PetMode::Nap:
        if (modeElapsed >= Config::NAP_SESSION_DURATION_MS)
        {
            _longRestPending = false;
            setMode(PetMode::Work, now);
        }
        break;
    }
}


void PashmakApp::persistStats(uint32_t now, bool force)
{
    constexpr uint32_t STATS_PERSIST_INTERVAL_MS = 60000UL;
    if (!_statsReady ||
        (!force && static_cast<uint32_t>(now - _lastStatsPersistAt) < STATS_PERSIST_INTERVAL_MS))
    {
        return;
    }

    _statsPreferences.putULong64("workMs", _workMs);
    _statsPreferences.putULong64("playMs", _playMs);
    _statsPreferences.putULong64("napMs", _napMs);
    _statsPreferences.putUInt("workSessions", _completedWorkSessions);
    _lastStatsPersistAt = now;

    Serial.printf("[STATS] Work: %llu min | Play: %llu min | Rest: %llu min | Work sessions: %lu\\n",
                  static_cast<unsigned long long>(_workMs / 60000ULL),
                  static_cast<unsigned long long>(_playMs / 60000ULL),
                  static_cast<unsigned long long>(_napMs / 60000ULL),
                  static_cast<unsigned long>(_completedWorkSessions));
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

    const uint32_t elapsed = static_cast<uint32_t>(now - _lastWorkTick);
    switch (_mode)
    {
    case PetMode::Work: _workMs += elapsed; break;
    case PetMode::Play: _playMs += elapsed; break;
    case PetMode::Nap: _napMs += elapsed; break;
    }
    _lastWorkTick = now;
    persistStats(now);

    updateAutomaticMode(now);
    _behavior.update(_mode, now);
    updateRtcReminder(now);

    if (_reminderMessage != nullptr &&
        static_cast<uint32_t>(now - _reminderShownAt) >= Config::REMINDER_DISPLAY_DURATION_MS)
    {
        _reminderMessage = nullptr;
    }

    if (static_cast<uint32_t>(now - _lastFrame) >= Config::FRAME_INTERVAL_MS)
    {
        ++_frame;
        const char *message = _reminderMessage != nullptr
                                  ? _reminderMessage
                                  : _behavior.state().message;

        char dateTime[24] = "--:--  ----/--/--";
        if (_rtc.isAvailable())
        {
            const DateTime current = _rtc.now();
            snprintf(dateTime, sizeof(dateTime), "%02u:%02u  %04u-%02u-%02u",
                     current.hour(), current.minute(), current.year(),
                     current.month(), current.day());
        }
        _renderer.draw(_display, _mode, _frame, message, dateTime);
        _lastFrame = now;
    }
}
