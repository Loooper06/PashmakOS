#include "RtcService.h"
#include "Config.h"
#include <Wire.h>

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
        // Initial fallback: seed the RTC from firmware build time.
        // Set the correct system time before the first flash for best accuracy.
        _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("[RTC] Backup power was lost; clock seeded from build time.");
    }

    const DateTime current = _rtc.now();
    _lastWaterReminder = current.unixtime();
    _lastMovementReminder = current.unixtime();
    _lastComplimentDay = current.day();

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

    // Keep reminders within a daytime window; do not nag overnight.
    if (current.hour() >= 9 && current.hour() < 21)
    {
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

        // One light-hearted compliment per day, at 10:00.
        if (current.hour() == 10 && current.minute() == 0 &&
            _lastComplimentDay != current.day())
        {
            _lastComplimentDay = current.day();
            return ReminderType::Compliment;
        }
    }

    return ReminderType::None;
}

bool RtcService::isAvailable() const
{
    return _available;
}

DateTime RtcService::now() const
{
    return _rtc.now();
}
