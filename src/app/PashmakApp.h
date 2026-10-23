#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include "RtcService.h"
#include "../core/PetState.h"
#include "../display/Display.h"
#include "../pet/PetRenderer.h"
#include "../pet/PetBehavior.h"

class PashmakApp
{
public:
    struct DailyStats
    {
        int32_t dateKey = 0; // YYYYMMDD
        uint32_t workMs = 0;
        uint32_t playMs = 0;
        uint32_t napMs = 0;
        uint16_t completedWorkSessions = 0;
        uint16_t reserved = 0;
    };

    struct StatsSummary
    {
        uint64_t workMs = 0;
        uint64_t playMs = 0;
        uint64_t napMs = 0;
        uint32_t completedWorkSessions = 0;
        uint8_t daysIncluded = 0;
    };

    struct MonthlyStats
    {
        int32_t monthKey = 0; // YYYYMM
        uint64_t workMs = 0;
        uint64_t playMs = 0;
        uint64_t napMs = 0;
        uint32_t completedWorkSessions = 0;
    };

    PashmakApp();
    void begin();
    void update();

    uint8_t dailyHistoryCount() const;
    bool getDailyStats(uint8_t index, DailyStats &out) const;
    StatsSummary summarizeRecentDays(uint8_t days) const;
    StatsSummary summarizeMonth(uint16_t year, uint8_t month) const;

private:
    static constexpr uint8_t DAILY_HISTORY_DAYS = 14;
    static constexpr uint8_t MONTHLY_HISTORY_MONTHS = 12;

    struct DailyHistoryStorage
    {
        uint32_t version = 1;
        uint8_t count = 0;
        uint8_t reserved[3]{};
        DailyStats records[DAILY_HISTORY_DAYS]{};
    };

    struct MonthlyHistoryStorage
    {
        uint32_t version = 1;
        uint8_t count = 0;
        uint8_t reserved[3]{};
        MonthlyStats records[MONTHLY_HISTORY_MONTHS]{};
    };

    Display _display;
    PetRenderer _renderer;
    PetBehavior _behavior;
    RtcService _rtc;

    PetMode _mode = PetMode::Work;

    uint32_t _lastFrame = 0;
    uint32_t _frame = 0;
    uint64_t _workMs = 0;
    uint64_t _playMs = 0;
    uint64_t _napMs = 0;
    uint32_t _lastWorkTick = 0;
    uint32_t _lastStatsPersistAt = 0;
    uint32_t _completedWorkSessions = 0;
    Preferences _statsPreferences;
    bool _statsReady = false;
    uint32_t _modeStartedAt = 0;
    uint32_t _reminderShownAt = 0;

    DailyStats _dailyHistory[DAILY_HISTORY_DAYS]{};
    uint8_t _dailyHistoryCount = 0;
    int8_t _currentDailyIndex = -1;
    uint32_t _lastHistoryPersistAt = 0;
    static constexpr uint32_t HISTORY_PERSIST_INTERVAL_MS = 60000UL;

    MonthlyStats _monthlyHistory[MONTHLY_HISTORY_MONTHS]{};
    uint8_t _monthlyHistoryCount = 0;
    int8_t _currentMonthlyIndex = -1;
    uint32_t _lastMonthlyPersistAt = 0;

    uint8_t _completedWorkCycles = 0;
    bool _longRestPending = false;
    const char *_reminderMessage = nullptr;

    void setMode(PetMode mode, uint32_t now);
    void updateAutomaticMode(uint32_t now);
    void updateRtcReminder(uint32_t now);
    void persistStats(uint32_t now, bool force = false);
    void loadDailyHistory();
    void syncDailyHistory(uint32_t now);
    void loadMonthlyHistory();
    void syncMonthlyHistory(uint32_t now);
    void persistMonthlyHistory(uint32_t now, bool force = false);
    void recordMonthlyElapsed(uint32_t elapsedMs);
    void recordMonthlyCompletedWorkSession();
    int8_t findMonthlyRecord(int32_t monthKey) const;
    void persistDailyHistory(uint32_t now, bool force = false);
    void recordDailyElapsed(uint32_t elapsedMs);
    void recordCompletedWorkSession();
    int8_t findDailyRecord(int32_t dateKey) const;
    static int32_t makeDateKey(const DateTime &date);
};
