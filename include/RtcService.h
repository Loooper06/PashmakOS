#pragma once

#include <Arduino.h>
#include <RTClib.h>

enum class ReminderType : uint8_t
{
    None,
    DrinkWater,
    MoveBody,
    Compliment
};

class RtcService
{
public:
    bool begin();
    ReminderType update(uint32_t nowMs);
    bool isAvailable() const;
    DateTime now();

private:
    RTC_DS3231 _rtc;
    bool _available = false;
    uint32_t _lastPollMs = 0;
    uint32_t _lastWaterReminder = 0;
    uint32_t _lastMovementReminder = 0;
    int32_t _lastComplimentDate = -1;
};
