#include "RtcService.h"
#include "Config.h"
#include <Wire.h>

namespace
{
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
    _lastWaterReminder = current.unixtime();
    _lastMovementReminder = current.unixtime();
    _lastComplimentDate = -1;

    _available = true;
    Serial.printf(
        "[RTC] DS3231 ready: %04d-%02d-%02d %02d:%02d:%02d\n",
        current.year(), current.month(), current.day(),
        current.hour(), current.minute(), current.second());
    return true;
}

ReminderType RtcService::update(uint32_t nowMs)
{
    if (!_available ||
        static_cast<uint32_t>(nowMs - _lastPollMs) < Config::RTC_POLL_INTERVAL_MS)
    {
        return ReminderType::None;
    }

    _lastPollMs = nowMs;
    const DateTime current = _rtc.now();
    const uint32_t epoch = current.unixtime();

    // If the clock was corrected backwards, reset baselines before interval checks.
    if (epoch < _lastWaterReminder)
        _lastWaterReminder = epoch;
    if (epoch < _lastMovementReminder)
        _lastMovementReminder = epoch;

    // Keep reminders within a daytime window; do not nag overnight.
    if (current.hour() < 9 || current.hour() >= 21)
        return ReminderType::None;

    // Trigger once per day at or after 10:00. This also survives booting
    // after 10:00 and short polling delays without missing the daily message.
    if (current.hour() >= 10)
    {
        const int32_t today = dateKey(current);
        if (_lastComplimentDate != today)
        {
            _lastComplimentDate = today;
            return ReminderType::Compliment;
        }
    }

    if (epoch - _lastWaterReminder >= Config::WATER_REMINDER_INTERVAL_SECONDS)
    {
        _lastWaterReminder = epoch;
        return ReminderType::DrinkWater;
    }

    if (epoch - _lastMovementReminder >= Config::MOVEMENT_REMINDER_INTERVAL_SECONDS)
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
