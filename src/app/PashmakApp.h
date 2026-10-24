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
        int32_t dateKey = 0;
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
        int32_t monthKey = 0;
        uint64_t workMs = 0;
        uint64_t playMs = 0;
        uint64_t napMs = 0;
        uint32_t completedWorkSessions = 0;
    };

    struct TimerSettings
    {
        uint16_t workMinutes = 25;
        uint16_t playMinutes = 2;
        uint16_t napMinutes = 5;
        uint8_t workCyclesBeforeLongRest = 3;
    };

    PashmakApp();
    void begin();
    void update();

    bool setActivePet(uint8_t petId);
    bool interactWithActivePet(uint32_t now);
    uint8_t activePetId() const;
    uint8_t petBond(uint8_t petId) const;
    TimerSettings timerSettings() const;
    bool setTimerSettings(const TimerSettings &settings);
    uint8_t dailyHistoryCount() const;
    bool getDailyStats(uint8_t index, DailyStats &out) const;
    uint8_t monthlyHistoryCount() const;
    bool getMonthlyStats(uint8_t index, MonthlyStats &out) const;
    StatsSummary summarizeRecentDays(uint8_t days) const;
    StatsSummary summarizeMonth(uint16_t year, uint8_t month) const;
    String statisticsToJson(uint8_t recentDays = 7) const;

private:
    static constexpr uint8_t PET_COUNT = 3;
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

    // Versioned, bounded storage for each pet's energy and stable personality ID.
    struct TimerSettingsStorage
    {
        uint32_t version = 1;
        TimerSettings settings{};
    };

    struct PetProfileStorage
    {
        uint32_t version = 1;
        uint8_t count = 0;
        uint8_t petIds[PET_COUNT]{};
        uint8_t energy[PET_COUNT]{};
    };

    Display _display;
    PetRenderer _renderer;
    PetBehavior _behaviors[PET_COUNT];
    RtcService _rtc;
    uint8_t _activePetId = 0;
    TimerSettings _timerSettings{};

    PetMode _mode = PetMode::Work;

    uint32_t _lastFrame = 0;
    uint32_t _lastUpdateAt = 0;
    uint32_t _loopMetricCount = 0;
    uint64_t _loopMetricTotalMs = 0;
    uint32_t _loopMetricMaxGapMs = 0;
    uint32_t _loopMetricOverBudget = 0;
    uint32_t _lastFrameMetricsAt = 0;
    uint32_t _frameMetricCount = 0;
    uint64_t _frameMetricTotalUs = 0;
    uint32_t _frameMetricMaxUs = 0;
    uint32_t _frameMetricOverBudget = 0;
    uint32_t _frame = 0;
    uint64_t _workMs = 0;
    uint64_t _playMs = 0;
    uint64_t _napMs = 0;
    uint32_t _lastWorkTick = 0;
    uint32_t _lastStatsPersistAt = 0;
    uint32_t _lastPetProfilePersistAt = 0;
    uint8_t _petProfileEnergy[PET_COUNT] = {80, 80, 80};
    uint8_t _petBond[PET_COUNT] = {0, 0, 0};
    uint32_t _lastInteractionAt[PET_COUNT]{};
    bool _hasInteracted[PET_COUNT]{};
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
    char _serialCommandBuffer[32]{};
    uint8_t _serialCommandLength = 0;
    bool _serialCommandOverflow = false;

    void setMode(PetMode mode, uint32_t now);
    void updateAutomaticMode(uint32_t now);
    void updateRtcReminder(uint32_t now);
    void handleSerialCommands();
    void persistStats(uint32_t now, bool force = false);
    void loadTimerSettings();
    void loadPetProfiles();
    void persistPetProfiles(uint32_t now);
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