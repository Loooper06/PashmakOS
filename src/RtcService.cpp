#include "RtcService.h"
#include "Config.h"
#include <Wire.h>

namespace
{
    constexpr char PREFERENCES_NAMESPACE[] = "pashmak";
    constexpr char LAST_COMPLIMENT_KEY[] = "complimentDay";

    int32_t dateKey(const DateTime &date)
    {
        return static_cast<int32_t>(date.year()) * 10000L +
               static_cast<int32_t>(date.month()) * 100L +
               static_cast<int32_t>(date.day());
    }
}

bool RtcService::begin()
{
    Wire.begin(Config::RTC_PIN_SDA, Config::RTC_PIN_SCL);

    if (!_rtc.begin())
    {
        Serial.println("[RTC] DS3231 not detected. Check VCC, GND, SDA, SCL.");
        _available = false;
        return false;
    }

    if (_rtc.lostPower())
    {
        // Fallback only: this is the firmware build timestamp, not live network time.
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("[RTC] Backup power was lost; clock seeded from firmware build time.");
    }

    const DateTime current = _rtc.now();
    const uint32_t epoch = current.unixtime();
    _lastWaterReminder = epoch;
    _lastMovementReminder = epoch;

    _preferencesReady = _preferences.begin(PREFERENCES_NAMESPACE, false);
    if (_preferencesReady)
    {
        _lastComplimentDate = _preferences.getInt(LAST_COMPLIMENT_KEY, -1);
    }
    else
    {
        _lastComplimentDate = -1;
        Serial.println("[RTC] Preferences unavailable; daily compliment guard is RAM-only.");
    }

    _available = true;
    Serial.printf(
        "[RTC] DS3231 ready: %04d-%02d-%02d %02d:%02d:%02d\n",
        current.year(), current.month(), current.day(),
        current.hour(), current.minute(), current.second());
    return true;
}

ReminderType RtcService::update(uint32_t nowMs, bool quietHours,
                                uint16_t waterIntervalMinutes,
                                uint16_t movementIntervalMinutes)
{
    if (!_available ||
        static_cast<uint32_t>(nowMs - _lastPollMs) < Config::RTC_POLL_INTERVAL_MS)
    {
        return ReminderType::None;
    }

    _lastPollMs = nowMs;
    const DateTime current = _rtc.now();
    const uint32_t epoch = current.unixtime();

    if (epoch < _lastWaterReminder)
        _lastWaterReminder = epoch;
    if (epoch < _lastMovementReminder)
        _lastMovementReminder = epoch;

    // Quiet hours suppress reminders and restart interval clocks so an overdue
    // reminder does not fire immediately when quiet hours end.
    if (quietHours)
    {
        _lastWaterReminder = epoch;
        _lastMovementReminder = epoch;
        return ReminderType::None;
    }

    // Persist the date so a reboot cannot repeat the daily compliment.
    if (current.hour() >= 10)
    {
        const int32_t today = dateKey(current);
        if (_lastComplimentDate != today)
        {
            _lastComplimentDate = today;
            if (_preferencesReady)
                _preferences.putInt(LAST_COMPLIMENT_KEY, today);
            return ReminderType::Compliment;
        }
    }

    // A zero interval disables that reminder and continually resets its clock,
    // so re-enabling it starts a fresh interval rather than firing immediately.
    if (waterIntervalMinutes == 0)
        _lastWaterReminder = epoch;
    else if (epoch - _lastWaterReminder >= static_cast<uint32_t>(waterIntervalMinutes) * 60UL)
    {
        _lastWaterReminder = epoch;
        return ReminderType::DrinkWater;
    }

    if (movementIntervalMinutes == 0)
        _lastMovementReminder = epoch;
    else if (epoch - _lastMovementReminder >= static_cast<uint32_t>(movementIntervalMinutes) * 60UL)
    {
        _lastMovementReminder = epoch;
        return ReminderType::MoveBody;
    }

    return ReminderType::None;
}

bool RtcService::isAvailable() const
{
    return _available;
}

DateTime RtcService::now()
{
    return _rtc.now();
}
