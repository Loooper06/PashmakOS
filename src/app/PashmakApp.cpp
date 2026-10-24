#include "PashmakApp.h"
#include "Config.h"
#include "../core/JsonEscape.h"
#include "../core/HistoryDate.h"
#include "../core/PetProfileValidation.h"
#include "../core/StatsAggregation.h"
#include "../core/StatsJson.h"
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

PashmakApp::PashmakApp()
    : _behaviors{PetBehavior(0), PetBehavior(1), PetBehavior(2)}
{
}

bool PashmakApp::setActivePet(uint8_t petId)
{
    if (petId >= PET_COUNT)
        return false;
    _activePetId = petId;
    if (_statsReady) _statsPreferences.putUChar("activePet", _activePetId);
    return true;
}

bool PashmakApp::interactWithActivePet(uint32_t now, PetInteraction interaction)
{
    if (_activePetId >= PET_COUNT)
        return false;

    const uint8_t petId = _activePetId;
    // Per-pet cooldown prevents dashboard clicks from spamming reactions or flash writes.
    if (_hasInteracted[petId] &&
        static_cast<uint32_t>(now - _lastInteractionAt[petId]) < 8000UL)
        return false;

    _hasInteracted[petId] = true;
    _lastInteractionAt[petId] = now;
    _attentionEffectStartedAt = now;
    _attentionEffectUntil = now + 3000UL;
    if (_petBond[petId] < 100)
    {
        const uint16_t nextBond = static_cast<uint16_t>(_petBond[petId]) + 5U;
        _petBond[petId] = nextBond > 100U ? 100U : static_cast<uint8_t>(nextBond);
    }

    if (_statsReady)
    {
        char key[8];
        snprintf(key, sizeof(key), "bond%u", petId);
        if (_statsPreferences.putUChar(key, _petBond[petId]) != sizeof(uint8_t))
            Serial.printf("[BOND] Failed to persist bond for pet %u.\n", petId);
    }

    _behaviors[petId].interact(_mode, now, _petBond[petId], interaction);
    const uint32_t newlyUnlocked = AchievementProgress::recordInteraction(_achievementProgress, petId, _petBond[petId]);
    persistAchievementProgress(now, newlyUnlocked != 0);
    if (newlyUnlocked != 0) Serial.printf("[ACHIEVEMENT] Pet %u unlocked mask 0x%08lX.\\n", petId, static_cast<unsigned long>(newlyUnlocked));
    Serial.printf("[BOND] Pet %u interaction; bond=%u/100.\n", petId, _petBond[petId]);
    return true;
}

uint8_t PashmakApp::activePetId() const
{
    return _activePetId;
}

uint8_t PashmakApp::petBond(uint8_t petId) const
{
    return petId < PET_COUNT ? _petBond[petId] : 0;
}

PashmakApp::QuietHoursSettings PashmakApp::quietHoursSettings() const
{
    return _quietHours;
}

bool PashmakApp::setQuietHoursSettings(const QuietHoursSettings &settings)
{
    if (settings.startHour > 23 || settings.endHour > 23 ||
        settings.startHour == settings.endHour)
        return false;
    if (!_statsReady)
        return false;

    QuietHoursStorage stored{};
    stored.version = 1;
    stored.settings = settings;
    if (_statsPreferences.putBytes("quietCfg", &stored, sizeof(stored)) != sizeof(stored))
    {
        Serial.println("[QUIET] Failed to persist quiet hours.");
        return false;
    }
    _quietHours = settings;
    Serial.printf("[QUIET] %s, %02u:00-%02u:00.\n",
                  _quietHours.enabled ? "Enabled" : "Disabled",
                  _quietHours.startHour, _quietHours.endHour);
    return true;
}

PashmakApp::ReminderSettings PashmakApp::reminderSettings() const
{
    return _reminderSettings;
}

bool PashmakApp::setReminderSettings(const ReminderSettings &settings)
{
    if ((settings.waterMinutes != 0 && (settings.waterMinutes < 15 || settings.waterMinutes > 240)) ||
        (settings.movementMinutes != 0 && (settings.movementMinutes < 15 || settings.movementMinutes > 180)))
        return false;
    if (!_statsReady)
        return false;

    ReminderSettingsStorage stored{};
    stored.version = 1;
    stored.settings = settings;
    if (_statsPreferences.putBytes("reminderCfg", &stored, sizeof(stored)) != sizeof(stored))
    {
        Serial.println("[REMINDER] Failed to persist reminder settings.");
        return false;
    }
    _reminderSettings = settings;
    Serial.printf("[REMINDER] Water=%u min, movement=%u min.\n",
                  _reminderSettings.waterMinutes, _reminderSettings.movementMinutes);
    return true;
}

bool PashmakApp::isQuietHour(int8_t hour) const
{
    if (!_quietHours.enabled || hour < 0 || hour > 23 ||
        _quietHours.startHour == _quietHours.endHour)
        return false;

    if (_quietHours.startHour < _quietHours.endHour)
        return hour >= _quietHours.startHour && hour < _quietHours.endHour;
    return hour >= _quietHours.startHour || hour < _quietHours.endHour;
}

PashmakApp::TimerSettings PashmakApp::timerSettings() const
{
    return _timerSettings;
}

bool PashmakApp::setTimerSettings(const TimerSettings &settings)
{
    if (settings.workMinutes < 1 || settings.workMinutes > 180 ||
        settings.playMinutes < 1 || settings.playMinutes > 60 ||
        settings.napMinutes < 1 || settings.napMinutes > 120 ||
        settings.workCyclesBeforeLongRest < 1 || settings.workCyclesBeforeLongRest > 8)
        return false;

    if (!_statsReady)
        return false;

    TimerSettingsStorage stored{};
    stored.version = 1;
    stored.settings = settings;
    if (_statsPreferences.putBytes("timerCfg", &stored, sizeof(stored)) != sizeof(stored))
    {
        Serial.println("[TIMER] Failed to persist timer settings.");
        return false;
    }

    _timerSettings = settings;
    _timerPaused = false;
    _pausedRemainingMs = 0;
    _modeStartedAt = millis();
    _lastWorkTick = _modeStartedAt;
    Serial.printf("[TIMER] Settings saved: work=%u min, play=%u min, nap=%u min, long-rest every %u work sessions.\n",
                  _timerSettings.workMinutes, _timerSettings.playMinutes,
                  _timerSettings.napMinutes, _timerSettings.workCyclesBeforeLongRest);
    return true;
}


bool PashmakApp::controlTimer(TimerCommand command, uint32_t now)
{
    const auto durationFor = [this](PetMode phase) -> uint32_t
    {
        switch (phase)
        {
        case PetMode::Work: return static_cast<uint32_t>(_timerSettings.workMinutes) * 60000UL;
        case PetMode::Play: return static_cast<uint32_t>(_timerSettings.playMinutes) * 60000UL;
        case PetMode::Nap: return static_cast<uint32_t>(_timerSettings.napMinutes) * 60000UL;
        }
        return 0;
    };

    switch (command)
    {
    case TimerCommand::Pause:
        if (_timerPaused)
            return true;
        {
            const uint32_t duration = durationFor(_mode);
            const uint32_t elapsed = static_cast<uint32_t>(now - _modeStartedAt);
            _pausedRemainingMs = elapsed >= duration ? 0U : duration - elapsed;
            _timerPaused = true;
            _lastWorkTick = now;
            Serial.printf("[TIMER] Paused with %lu ms remaining.\n",
                          static_cast<unsigned long>(_pausedRemainingMs));
        }
        return true;

    case TimerCommand::Start:
        if (_timerPaused)
        {
            const uint32_t duration = durationFor(_mode);
            const uint32_t remaining = _pausedRemainingMs > duration ? duration : _pausedRemainingMs;
            _modeStartedAt = now - (duration - remaining);
            _timerPaused = false;
            _pausedRemainingMs = 0;
        }
        else
        {
            _modeStartedAt = now;
        }
        _lastWorkTick = now;
        Serial.println("[TIMER] Started/resumed.");
        return true;

    case TimerCommand::Skip:
        if (_mode == PetMode::Work)
        {
            // Skipping is not a completed focus session and must not award stats or achievements.
            setMode(PetMode::Play, now);
        }
        else
        {
            setMode(PetMode::Work, now);
            _longRestPending = false;
        }
        _lastWorkTick = now;
        return true;

    case TimerCommand::Reset:
        _completedWorkCycles = 0;
        _longRestPending = false;
        _timerPaused = false;
        _pausedRemainingMs = 0;
        setMode(PetMode::Work, now);
        _modeStartedAt = now;
        _lastWorkTick = now;
        Serial.println("[TIMER] Reset to a fresh work session.");
        return true;
    }
    return false;
}

PashmakApp::TimerStatus PashmakApp::timerStatus(uint32_t now) const
{
    TimerStatus status{};
    status.phase = _mode;
    status.paused = _timerPaused;
    status.completedCycles = _completedWorkCycles;
    status.cyclesBeforeLongRest = _timerSettings.workCyclesBeforeLongRest;

    uint32_t duration = 0;
    switch (_mode)
    {
    case PetMode::Work: duration = static_cast<uint32_t>(_timerSettings.workMinutes) * 60000UL; break;
    case PetMode::Play: duration = static_cast<uint32_t>(_timerSettings.playMinutes) * 60000UL; break;
    case PetMode::Nap: duration = static_cast<uint32_t>(_timerSettings.napMinutes) * 60000UL; break;
    }
    if (_timerPaused)
        status.remainingMs = _pausedRemainingMs > duration ? duration : _pausedRemainingMs;
    else
    {
        const uint32_t elapsed = static_cast<uint32_t>(now - _modeStartedAt);
        status.remainingMs = elapsed >= duration ? 0U : duration - elapsed;
    }
    return status;
}

uint8_t PashmakApp::dailyHistoryCount() const
{
    return _dailyHistoryCount;
}

bool PashmakApp::getDailyStats(uint8_t index, DailyStats &out) const
{
    if (index >= _dailyHistoryCount)
        return false;

    out = _dailyHistory[index];
    return true;
}

uint8_t PashmakApp::monthlyHistoryCount() const
{
    return _monthlyHistoryCount;
}

bool PashmakApp::getMonthlyStats(uint8_t index, MonthlyStats &out) const
{
    if (index >= _monthlyHistoryCount)
        return false;

    out = _monthlyHistory[index];
    return true;
}

PashmakApp::StatsSummary PashmakApp::summarizeRecentDays(uint8_t days) const
{
    return StatsAggregation::summarizeRecentDays<DailyStats, StatsSummary>(
        _dailyHistory, _dailyHistoryCount, days);
}

PashmakApp::StatsSummary PashmakApp::summarizeMonth(uint16_t year, uint8_t month) const
{
    return StatsAggregation::summarizeMonth<MonthlyStats, StatsSummary>(
        _monthlyHistory, _monthlyHistoryCount, year, month);
}

String PashmakApp::statisticsToJson(uint8_t recentDays) const
{
    const StatsSummary recent = summarizeRecentDays(recentDays);
    const auto moodName = [](PetMood mood) -> const char *
    {
        switch (mood)
        {
        case PetMood::Happy: return "happy";
        case PetMood::Focused: return "focused";
        case PetMood::Sleepy: return "sleepy";
        case PetMood::Curious: return "curious";
        case PetMood::Excited: return "excited";
        case PetMood::Tired: return "tired";
        case PetMood::Exhausted: return "exhausted";
        default: return "curious";
        }
    };

    static const char *petNames[] = {"Pashmak", "Mochi", "Pixel"};
    StatsJson::Pet pets[PET_COUNT]{};
    for (uint8_t i = 0; i < PET_COUNT; ++i)
    {
        const PetBehaviorState &profile = _behaviors[i].state();
        pets[i] = StatsJson::Pet{
            i, profile.energy, profile.happiness, _petBond[i], petNames[i], moodName(profile.mood), profile.message,
            _achievementProgress.interactions[i], _achievementProgress.petMasks[i]};
    }

    const PetBehaviorState &active = _behaviors[_activePetId].state();
    const StatsJson::Counters totals{
        _workMs, _playMs, _napMs, _completedWorkSessions};
    const StatsJson::Recent recentView{
        recentDays,
        recent.daysIncluded,
        StatsJson::Counters{
            recent.workMs,
            recent.playMs,
            recent.napMs,
            recent.completedWorkSessions}};

    StatsJson::DailyRecord daily[DAILY_HISTORY_DAYS]{};
    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
    {
        const DailyStats &record = _dailyHistory[i];
        daily[i] = StatsJson::DailyRecord{
            record.dateKey,
            record.workMs,
            record.playMs,
            record.napMs,
            record.completedWorkSessions};
    }

    StatsJson::MonthlyRecord monthly[MONTHLY_HISTORY_MONTHS]{};
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
    {
        const MonthlyStats &record = _monthlyHistory[i];
        monthly[i] = StatsJson::MonthlyRecord{
            record.monthKey,
            record.workMs,
            record.playMs,
            record.napMs,
            record.completedWorkSessions};
    }

    const StatsJson::Payload payload{
        _activePetId,
        pets,
        PET_COUNT,
        StatsJson::Pet{_activePetId, active.energy, active.happiness, _petBond[_activePetId], nullptr, moodName(active.mood), active.message,
            _achievementProgress.interactions[_activePetId], _achievementProgress.petMasks[_activePetId]},
        totals,
        recentView,
        daily,
        _dailyHistoryCount,
        monthly,
        _monthlyHistoryCount,
        _achievementProgress.globalMask};

    String json;
    json.reserve(180 + static_cast<size_t>(_dailyHistoryCount) * 100 +
                 static_cast<size_t>(_monthlyHistoryCount) * 140);
    StatsJson::appendStatisticsJson(json, payload);
    return json;
}

void PashmakApp::begin()
{
    Serial.begin(115200);
    _display.begin();

    const uint32_t now = millis();
    _lastWorkTick = now;
    _lastFrame = now;
    _lastFrameMetricsAt = now;
    _lastUpdateAt = now;
    _modeStartedAt = now;
    _lastStatsPersistAt = now;
    _lastPetProfilePersistAt = now;
    _lastAchievementPersistAt = now;

    _statsReady = _statsPreferences.begin("petstats", false);
    if (_statsReady)
    {
        _activePetId = _statsPreferences.getUChar("activePet", 0);
        for (uint8_t i = 0; i < PET_COUNT; ++i)
        {
            char key[8];
            snprintf(key, sizeof(key), "bond%u", i);
            const uint8_t savedBond = _statsPreferences.getUChar(key, 0);
            _petBond[i] = savedBond <= 100 ? savedBond : 0;
        }
        if (_activePetId >= 3) _activePetId = 0;
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

    loadTimerSettings();
    loadQuietHoursSettings();
    loadReminderSettings();
    loadPetProfiles();
    loadAchievementProgress();
    for (uint8_t i = 0; i < PET_COUNT; ++i)
    {
        _behaviors[i].begin(now);
        if (!_behaviors[i].restoreEnergy(_petProfileEnergy[i], now))
            _behaviors[i].restoreEnergy(80, now);
        if (!_behaviors[i].restoreHappiness(_petProfileHappiness[i], now))
            _behaviors[i].restoreHappiness(70, now);
    }
    _rtc.begin();
    loadDailyHistory();
    syncDailyHistory(now);
    loadMonthlyHistory();
    syncMonthlyHistory(now);

    _renderer.draw(
        _display,
        _mode,
        _frame,
        _activePetId,
        _behaviors[_activePetId].state().mood,
        _behaviors[_activePetId].state().message,
        nullptr,
        false,
        0);

    Serial.println("PashmakOS: automatic pet behavior enabled");
}

void PashmakApp::setMode(PetMode mode, uint32_t now)
{
    if (_mode == mode)
        return;

    _mode = mode;
    _timerPaused = false;
    _pausedRemainingMs = 0;
    _modeStartedAt = now;
    persistStats(now, true);
    persistDailyHistory(now, true);
    persistMonthlyHistory(now, true);
    Serial.printf("[PET] Mode changed: %s\n", modeName(_mode));
}

void PashmakApp::updateAutomaticMode(uint32_t now)
{
    if (_timerPaused)
        return;

    const uint32_t modeElapsed = static_cast<uint32_t>(now - _modeStartedAt);

    switch (_mode)
    {
    case PetMode::Work:
        if (modeElapsed >= static_cast<uint32_t>(_timerSettings.workMinutes) * 60UL * 1000UL)
        {
            ++_completedWorkSessions;
            recordCompletedWorkSession();
            recordMonthlyCompletedWorkSession();
            ++_completedWorkCycles;

            if (_completedWorkCycles >= _timerSettings.workCyclesBeforeLongRest)
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
        if (modeElapsed >= static_cast<uint32_t>(_timerSettings.playMinutes) * 60UL * 1000UL)
            setMode(PetMode::Work, now);
        break;

    case PetMode::Nap:
        if (modeElapsed >= static_cast<uint32_t>(_timerSettings.napMinutes) * 60UL * 1000UL)
        {
            _longRestPending = false;
            setMode(PetMode::Work, now);
        }
        break;
    }
}


void PashmakApp::loadTimerSettings()
{
    _timerSettings = TimerSettings{};
    if (!_statsReady)
        return;

    const size_t expected = sizeof(TimerSettingsStorage);
    if (_statsPreferences.getBytesLength("timerCfg") != expected)
    {
        Serial.println("[TIMER] No saved timer settings; using defaults.");
        return;
    }

    TimerSettingsStorage stored{};
    if (_statsPreferences.getBytes("timerCfg", &stored, expected) != expected ||
        stored.version != 1 ||
        stored.settings.workMinutes < 1 || stored.settings.workMinutes > 180 ||
        stored.settings.playMinutes < 1 || stored.settings.playMinutes > 60 ||
        stored.settings.napMinutes < 1 || stored.settings.napMinutes > 120 ||
        stored.settings.workCyclesBeforeLongRest < 1 ||
        stored.settings.workCyclesBeforeLongRest > 8)
    {
        Serial.println("[TIMER] Invalid saved settings; using defaults.");
        return;
    }

    _timerSettings = stored.settings;
    Serial.printf("[TIMER] Loaded settings: work=%u min, play=%u min, nap=%u min, long-rest every %u work sessions.\n",
                  _timerSettings.workMinutes, _timerSettings.playMinutes,
                  _timerSettings.napMinutes, _timerSettings.workCyclesBeforeLongRest);
}

void PashmakApp::loadQuietHoursSettings()
{
    _quietHours = QuietHoursSettings{};
    if (!_statsReady)
        return;

    QuietHoursStorage stored{};
    if (_statsPreferences.getBytesLength("quietCfg") != sizeof(stored) ||
        _statsPreferences.getBytes("quietCfg", &stored, sizeof(stored)) != sizeof(stored) ||
        stored.version != 1 || stored.settings.startHour > 23 ||
        stored.settings.endHour > 23 ||
        stored.settings.startHour == stored.settings.endHour)
    {
        Serial.println("[QUIET] Using default quiet hours (21:00-09:00).");
        return;
    }

    _quietHours = stored.settings;
    Serial.printf("[QUIET] Loaded %s, %02u:00-%02u:00.\n",
                  _quietHours.enabled ? "enabled" : "disabled",
                  _quietHours.startHour, _quietHours.endHour);
}

void PashmakApp::loadReminderSettings()
{
    _reminderSettings = ReminderSettings{};
    if (!_statsReady)
        return;

    ReminderSettingsStorage stored{};
    if (_statsPreferences.getBytesLength("reminderCfg") != sizeof(stored) ||
        _statsPreferences.getBytes("reminderCfg", &stored, sizeof(stored)) != sizeof(stored) ||
        stored.version != 1 ||
        (stored.settings.waterMinutes != 0 && (stored.settings.waterMinutes < 15 || stored.settings.waterMinutes > 240)) ||
        (stored.settings.movementMinutes != 0 && (stored.settings.movementMinutes < 15 || stored.settings.movementMinutes > 180)))
    {
        Serial.println("[REMINDER] Using default intervals (water 60 min, movement 45 min).");
        return;
    }

    _reminderSettings = stored.settings;
    Serial.printf("[REMINDER] Loaded water=%u min, movement=%u min.\n",
                  _reminderSettings.waterMinutes, _reminderSettings.movementMinutes);
}

void PashmakApp::loadAchievementProgress()
{
    _achievementProgress = AchievementProgress::Storage{};
    if (_statsReady && _statsPreferences.getBytesLength("achieve") == sizeof(AchievementProgress::Storage))
    {
        AchievementProgress::Storage stored{};
        if (_statsPreferences.getBytes("achieve", &stored, sizeof(stored)) == sizeof(stored) &&
            AchievementProgress::isValid(stored))
        {
            _achievementProgress = stored;
            Serial.println("[ACHIEVEMENT] Persistent achievement progress loaded.");
        }
        else
        {
            Serial.println("[ACHIEVEMENT] Invalid saved progress; using safe defaults.");
        }
    }
    _savedAchievementProgress = _achievementProgress;
}

void PashmakApp::persistAchievementProgress(uint32_t now, bool force)
{
    if (!_statsReady || AchievementProgress::equals(_achievementProgress, _savedAchievementProgress))
        return;
    constexpr uint32_t ACHIEVEMENT_PERSIST_INTERVAL_MS = 60000UL;
    if (!force && static_cast<uint32_t>(now - _lastAchievementPersistAt) < ACHIEVEMENT_PERSIST_INTERVAL_MS)
        return;
    if (_statsPreferences.putBytes("achieve", &_achievementProgress, sizeof(_achievementProgress)) != sizeof(_achievementProgress))
    {
        Serial.println("[ACHIEVEMENT] Failed to persist progression; will retry.");
        return;
    }
    _savedAchievementProgress = _achievementProgress;
    _lastAchievementPersistAt = now;
    Serial.println("[ACHIEVEMENT] Progress saved.");
}

void PashmakApp::loadPetProfiles()
{
    for (uint8_t i = 0; i < PET_COUNT; ++i)
    {
        _petProfileEnergy[i] = 80;
        _petProfileHappiness[i] = 70;
    }
    if (!_statsReady) return;

    const size_t storedLength = _statsPreferences.getBytesLength("petProfile");
    if (storedLength == sizeof(PetProfileStorage))
    {
        PetProfileStorage stored{};
        if (_statsPreferences.getBytes("petProfile", &stored, sizeof(stored)) == sizeof(stored) &&
            PetProfileValidation::isValid<PetProfileStorage, PET_COUNT>(stored, 2))
        {
            bool validHappiness = true;
            for (uint8_t i = 0; i < PET_COUNT; ++i)
                validHappiness = validHappiness && stored.happiness[i] <= 100;
            if (!validHappiness)
            {
                Serial.println("[PET_PROFILE] Invalid happiness; using defaults.");
                return;
            }
            for (uint8_t i = 0; i < PET_COUNT; ++i)
            {
                _petProfileEnergy[i] = stored.energy[i];
                _petProfileHappiness[i] = stored.happiness[i];
            }
            Serial.println("[PET_PROFILE] Loaded version 2 energy and happiness for all pets.");
            return;
        }
    }
    if (storedLength == sizeof(LegacyPetProfileStorage))
    {
        LegacyPetProfileStorage legacy{};
        if (_statsPreferences.getBytes("petProfile", &legacy, sizeof(legacy)) == sizeof(legacy) &&
            PetProfileValidation::isValid<LegacyPetProfileStorage, PET_COUNT>(legacy, 1))
        {
            for (uint8_t i = 0; i < PET_COUNT; ++i) _petProfileEnergy[i] = legacy.energy[i];
            Serial.println("[PET_PROFILE] Migrated version 1 energy; happiness defaults to 70.");
            return;
        }
    }
    Serial.println("[PET_PROFILE] No compatible saved profiles; using defaults.");
}

void PashmakApp::persistPetProfiles(uint32_t now)
{
    constexpr uint32_t PET_PROFILE_PERSIST_INTERVAL_MS = 60000UL;
    if (!_statsReady || static_cast<uint32_t>(now - _lastPetProfilePersistAt) < PET_PROFILE_PERSIST_INTERVAL_MS) return;
    _lastPetProfilePersistAt = now;
    PetProfileStorage stored{};
    stored.version = 2;
    stored.count = PET_COUNT;
    bool changed = false;
    for (uint8_t i = 0; i < PET_COUNT; ++i)
    {
        stored.petIds[i] = i;
        stored.energy[i] = _behaviors[i].state().energy;
        stored.happiness[i] = _behaviors[i].state().happiness;
        changed = changed || stored.energy[i] != _petProfileEnergy[i] || stored.happiness[i] != _petProfileHappiness[i];
    }
    if (!changed) return;
    if (_statsPreferences.putBytes("petProfile", &stored, sizeof(stored)) != sizeof(stored))
    {
        Serial.println("[PET_PROFILE] Failed to persist pet profiles; will retry next interval.");
        return;
    }
    for (uint8_t i = 0; i < PET_COUNT; ++i)
    {
        _petProfileEnergy[i] = stored.energy[i];
        _petProfileHappiness[i] = stored.happiness[i];
    }
    Serial.println("[PET_PROFILE] Saved energy and happiness for all pets.");
}
void PashmakApp::persistStats(uint32_t now, bool force)
{
    constexpr uint32_t STATS_PERSIST_INTERVAL_MS = 60000UL;
    if (!_statsReady ||
        (!force && static_cast<uint32_t>(now - _lastStatsPersistAt) < STATS_PERSIST_INTERVAL_MS))
    {
        return;
    }

    const bool workSaved =
        _statsPreferences.putULong64("workMs", _workMs) == sizeof(uint64_t);
    const bool playSaved =
        _statsPreferences.putULong64("playMs", _playMs) == sizeof(uint64_t);
    const bool napSaved =
        _statsPreferences.putULong64("napMs", _napMs) == sizeof(uint64_t);
    const bool sessionsSaved =
        _statsPreferences.putUInt("workSessions", _completedWorkSessions) == sizeof(uint32_t);

    if (!(workSaved && playSaved && napSaved && sessionsSaved))
    {
        Serial.println("[STATS] Failed to persist one or more counters; will retry.");
        return;
    }

    _lastStatsPersistAt = now;
    Serial.printf("[STATS] Work: %llu min | Play: %llu min | Rest: %llu min | Work sessions: %lu\n",
                  static_cast<unsigned long long>(_workMs / 60000ULL),
                  static_cast<unsigned long long>(_playMs / 60000ULL),
                  static_cast<unsigned long long>(_napMs / 60000ULL),
                  static_cast<unsigned long>(_completedWorkSessions));
}

int32_t PashmakApp::makeDateKey(const DateTime &date)
{
    return static_cast<int32_t>(date.year()) * 10000 +
           static_cast<int32_t>(date.month()) * 100 +
           static_cast<int32_t>(date.day());
}

int8_t PashmakApp::findDailyRecord(int32_t dateKey) const
{
    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
    {
        if (_dailyHistory[i].dateKey == dateKey)
            return static_cast<int8_t>(i);
    }
    return -1;
}

void PashmakApp::loadDailyHistory()
{
    _dailyHistoryCount = 0;
    _currentDailyIndex = -1;
    if (!_statsReady)
        return;

    const size_t expected = sizeof(DailyHistoryStorage);
    if (_statsPreferences.getBytesLength("dailyHist") != expected)
    {
        Serial.println("[HISTORY] No valid saved daily history; starting fresh.");
        return;
    }

    DailyHistoryStorage stored{};
    if (_statsPreferences.getBytes("dailyHist", &stored, expected) != expected ||
        stored.version != 1 || stored.count > DAILY_HISTORY_DAYS)
    {
        Serial.println("[HISTORY] Saved history is invalid; starting fresh.");
        return;
    }

    _dailyHistoryCount = stored.count;
    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
    {
        if (!HistoryDate::isValidDateKey(stored.records[i].dateKey))
        {
            _dailyHistoryCount = 0;
            Serial.println("[HISTORY] Invalid date found; history reset.");
            return;
        }
        if (i > 0 && stored.records[i].dateKey <= stored.records[i - 1].dateKey)
        {
            _dailyHistoryCount = 0;
            Serial.println("[HISTORY] History order invalid; history reset.");
            return;
        }
    }

    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
        _dailyHistory[i] = stored.records[i];

    Serial.printf("[HISTORY] Loaded %u daily records.\n", _dailyHistoryCount);
}

void PashmakApp::syncDailyHistory(uint32_t now)
{
    (void)now;
    if (!_rtc.isAvailable())
    {
        _currentDailyIndex = -1;
        return;
    }

    const int32_t today = makeDateKey(_rtc.now());
    int8_t existing = findDailyRecord(today);
    if (existing >= 0)
    {
        _currentDailyIndex = existing;
        return;
    }

    if (_dailyHistoryCount > 0 &&
        today < _dailyHistory[_dailyHistoryCount - 1].dateKey)
    {
        _currentDailyIndex = -1;
        Serial.println("[HISTORY] RTC date moved backwards; daily recording paused to protect history.");
        return;
    }

    if (_dailyHistoryCount < DAILY_HISTORY_DAYS)
    {
        _currentDailyIndex = static_cast<int8_t>(_dailyHistoryCount++);
    }
    else
    {
        for (uint8_t i = 1; i < DAILY_HISTORY_DAYS; ++i)
            _dailyHistory[i - 1] = _dailyHistory[i];
        _currentDailyIndex = DAILY_HISTORY_DAYS - 1;
    }

    _dailyHistory[_currentDailyIndex] = DailyStats{};
    _dailyHistory[_currentDailyIndex].dateKey = today;
    persistDailyHistory(millis(), true);
    Serial.printf("[HISTORY] Started daily record %ld.\n", static_cast<long>(today));
}

void PashmakApp::recordDailyElapsed(uint32_t elapsedMs)
{
    if (_currentDailyIndex < 0 ||
        _currentDailyIndex >= static_cast<int8_t>(_dailyHistoryCount))
        return;

    DailyStats &day = _dailyHistory[_currentDailyIndex];
    switch (_mode)
    {
    case PetMode::Work: day.workMs += elapsedMs; break;
    case PetMode::Play: day.playMs += elapsedMs; break;
    case PetMode::Nap: day.napMs += elapsedMs; break;
    }
}

void PashmakApp::recordCompletedWorkSession()
{
    uint32_t newlyUnlocked = AchievementProgress::recordFocusSessions(_achievementProgress, _completedWorkSessions);
    if (_currentDailyIndex >= 0 &&
        _currentDailyIndex < static_cast<int8_t>(_dailyHistoryCount))
    {
        DailyStats &day = _dailyHistory[_currentDailyIndex];
        if (day.completedWorkSessions < UINT16_MAX)
            ++day.completedWorkSessions;
        persistDailyHistory(millis(), true);
        newlyUnlocked |= AchievementProgress::recordConsecutiveDays(_achievementProgress, consecutiveWorkDays());
    }
    persistAchievementProgress(millis(), newlyUnlocked != 0);
}

uint8_t PashmakApp::consecutiveWorkDays() const
{
    if (_dailyHistoryCount == 0 || _currentDailyIndex < 0 ||
        _currentDailyIndex >= static_cast<int8_t>(_dailyHistoryCount))
        return 0;

    uint8_t streak = 0;
    int32_t newerDateKey = 0;
    for (int16_t i = _currentDailyIndex; i >= 0; --i)
    {
        const DailyStats &record = _dailyHistory[i];
        if (record.completedWorkSessions == 0)
            break;
        if (newerDateKey != 0)
        {
            const int32_t year = record.dateKey / 10000;
            const int32_t month = (record.dateKey / 100) % 100;
            const int32_t day = record.dateKey % 100;
            int32_t expectedNextDate = 0;
            if (day < HistoryDate::daysInMonth(year, month))
                expectedNextDate = year * 10000 + month * 100 + day + 1;
            else if (month < 12)
                expectedNextDate = year * 10000 + (month + 1) * 100 + 1;
            else if (year < 2099)
                expectedNextDate = (year + 1) * 10000 + 101;
            if (expectedNextDate != newerDateKey)
                break;
        }
        ++streak;
        newerDateKey = record.dateKey;
        if (streak == UINT8_MAX)
            break;
    }
    return streak;
}

void PashmakApp::persistDailyHistory(uint32_t now, bool force)
{
    if (!_statsReady ||
        (!force && static_cast<uint32_t>(now - _lastHistoryPersistAt) < HISTORY_PERSIST_INTERVAL_MS))
        return;

    DailyHistoryStorage stored{};
    stored.version = 1;
    stored.count = _dailyHistoryCount;
    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
        stored.records[i] = _dailyHistory[i];

    const size_t written = _statsPreferences.putBytes("dailyHist", &stored, sizeof(stored));
    if (written == sizeof(stored))
    {
        _lastHistoryPersistAt = now;
        Serial.printf("[HISTORY] Saved %u daily records.\n", _dailyHistoryCount);
    }
    else
    {
        Serial.println("[HISTORY] Failed to persist daily history.");
    }
}

int8_t PashmakApp::findMonthlyRecord(int32_t monthKey) const
{
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
    {
        if (_monthlyHistory[i].monthKey == monthKey)
            return static_cast<int8_t>(i);
    }
    return -1;
}

void PashmakApp::loadMonthlyHistory()
{
    _monthlyHistoryCount = 0;
    _currentMonthlyIndex = -1;
    if (!_statsReady)
        return;

    const size_t expected = sizeof(MonthlyHistoryStorage);
    if (_statsPreferences.getBytesLength("monthHist") != expected)
    {
        Serial.println("[MONTHLY] No saved monthly history; starting fresh.");
        return;
    }

    MonthlyHistoryStorage stored{};
    if (_statsPreferences.getBytes("monthHist", &stored, expected) != expected ||
        stored.version != 1 || stored.count > MONTHLY_HISTORY_MONTHS)
    {
        Serial.println("[MONTHLY] Saved monthly history is invalid; starting fresh.");
        return;
    }

    for (uint8_t i = 0; i < stored.count; ++i)
    {
        const int32_t key = stored.records[i].monthKey;
        if (!HistoryDate::isValidMonthKey(key) ||
            (i > 0 && key <= stored.records[i - 1].monthKey))
        {
            Serial.println("[MONTHLY] Invalid month key/order; history reset.");
            return;
        }
    }

    _monthlyHistoryCount = stored.count;
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
        _monthlyHistory[i] = stored.records[i];

    Serial.printf("[MONTHLY] Loaded %u monthly records.\n", _monthlyHistoryCount);
}

void PashmakApp::syncMonthlyHistory(uint32_t now)
{
    (void)now;
    if (!_rtc.isAvailable())
    {
        _currentMonthlyIndex = -1;
        return;
    }

    const DateTime date = _rtc.now();
    const int32_t monthKey = static_cast<int32_t>(date.year()) * 100 + date.month();
    const int8_t existing = findMonthlyRecord(monthKey);
    if (existing >= 0)
    {
        _currentMonthlyIndex = existing;
        return;
    }

    if (_monthlyHistoryCount > 0 &&
        monthKey < _monthlyHistory[_monthlyHistoryCount - 1].monthKey)
    {
        _currentMonthlyIndex = -1;
        Serial.println("[MONTHLY] RTC month moved backwards; monthly recording paused.");
        return;
    }

    if (_monthlyHistoryCount < MONTHLY_HISTORY_MONTHS)
    {
        _currentMonthlyIndex = static_cast<int8_t>(_monthlyHistoryCount++);
    }
    else
    {
        for (uint8_t i = 1; i < MONTHLY_HISTORY_MONTHS; ++i)
            _monthlyHistory[i - 1] = _monthlyHistory[i];
        _currentMonthlyIndex = MONTHLY_HISTORY_MONTHS - 1;
    }

    _monthlyHistory[_currentMonthlyIndex] = MonthlyStats{};
    _monthlyHistory[_currentMonthlyIndex].monthKey = monthKey;
    persistMonthlyHistory(millis(), true);
    Serial.printf("[MONTHLY] Started monthly record %ld.\n", static_cast<long>(monthKey));
}

void PashmakApp::recordMonthlyElapsed(uint32_t elapsedMs)
{
    if (_currentMonthlyIndex < 0 ||
        _currentMonthlyIndex >= static_cast<int8_t>(_monthlyHistoryCount))
        return;

    MonthlyStats &record = _monthlyHistory[_currentMonthlyIndex];
    switch (_mode)
    {
    case PetMode::Work: record.workMs += elapsedMs; break;
    case PetMode::Play: record.playMs += elapsedMs; break;
    case PetMode::Nap: record.napMs += elapsedMs; break;
    }
}

void PashmakApp::recordMonthlyCompletedWorkSession()
{
    if (_currentMonthlyIndex < 0 ||
        _currentMonthlyIndex >= static_cast<int8_t>(_monthlyHistoryCount))
        return;

    MonthlyStats &record = _monthlyHistory[_currentMonthlyIndex];
    if (record.completedWorkSessions < UINT32_MAX)
        ++record.completedWorkSessions;
    persistMonthlyHistory(millis(), true);
}

void PashmakApp::persistMonthlyHistory(uint32_t now, bool force)
{
    if (!_statsReady ||
        (!force && static_cast<uint32_t>(now - _lastMonthlyPersistAt) < HISTORY_PERSIST_INTERVAL_MS))
        return;

    MonthlyHistoryStorage stored{};
    stored.version = 1;
    stored.count = _monthlyHistoryCount;
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
        stored.records[i] = _monthlyHistory[i];

    const size_t written = _statsPreferences.putBytes("monthHist", &stored, sizeof(stored));
    if (written == sizeof(stored))
    {
        _lastMonthlyPersistAt = now;
        Serial.printf("[MONTHLY] Saved %u monthly records.\n", _monthlyHistoryCount);
    }
    else
    {
        Serial.println("[MONTHLY] Failed to persist monthly history.");
    }
}

void PashmakApp::updateRtcReminder(uint32_t now, bool quietHours)
{
    const ReminderType reminder = _rtc.update(
        now, quietHours, _reminderSettings.waterMinutes, _reminderSettings.movementMinutes);
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

void PashmakApp::handleSerialCommands()
{
    while (Serial.available() > 0)
    {
        const char ch = static_cast<char>(Serial.read());

        if (ch == '\r')
            continue;

        if (ch != '\n')
        {
            if (!_serialCommandOverflow)
            {
                if (_serialCommandLength < sizeof(_serialCommandBuffer) - 1)
                {
                    _serialCommandBuffer[_serialCommandLength++] = ch;
                }
                else
                {
                    _serialCommandOverflow = true;
                }
            }
            continue;
        }

        if (_serialCommandOverflow)
        {
            Serial.println("[SERIAL] Command too long; ignored.");
        }
        else
        {
            _serialCommandBuffer[_serialCommandLength] = '\0';

            if (strcmp(_serialCommandBuffer, "stats") == 0)
            {
                Serial.println("[STATS_JSON_BEGIN]");
                Serial.println(statisticsToJson(7));
                Serial.println("[STATS_JSON_END]");
            }
            else if (strcmp(_serialCommandBuffer, "daily_csv") == 0)
            {
                Serial.println("[DAILY_CSV_BEGIN]");
                Serial.println("dateKey,workMs,playMs,napMs,completedWorkSessions");
                for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
                {
                    const DailyStats &day = _dailyHistory[i];
                    Serial.printf("%ld,%lu,%lu,%lu,%u\n",
                                  static_cast<long>(day.dateKey),
                                  static_cast<unsigned long>(day.workMs),
                                  static_cast<unsigned long>(day.playMs),
                                  static_cast<unsigned long>(day.napMs),
                                  static_cast<unsigned int>(day.completedWorkSessions));
                }
                Serial.println("[DAILY_CSV_END]");
            }
            else if (strcmp(_serialCommandBuffer, "monthly_csv") == 0)
            {
                Serial.println("[MONTHLY_CSV_BEGIN]");
                Serial.println("monthKey,workMs,playMs,napMs,completedWorkSessions");
                for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
                {
                    const MonthlyStats &month = _monthlyHistory[i];
                    Serial.printf("%ld,%llu,%llu,%llu,%lu\n",
                                  static_cast<long>(month.monthKey),
                                  static_cast<unsigned long long>(month.workMs),
                                  static_cast<unsigned long long>(month.playMs),
                                  static_cast<unsigned long long>(month.napMs),
                                  static_cast<unsigned long>(month.completedWorkSessions));
                }
                Serial.println("[MONTHLY_CSV_END]");
            }
            else if (strcmp(_serialCommandBuffer, "help") == 0)
            {
                Serial.println("Commands: stats, daily_csv, monthly_csv, help");
            }
            else if (_serialCommandLength > 0)
            {
                Serial.println("[SERIAL] Unknown command. Type help.");
            }
        }

        _serialCommandLength = 0;
        _serialCommandOverflow = false;
        _serialCommandBuffer[0] = '\0';
    }
}

void PashmakApp::update()
{
    // Measures cadence between app.update() entries. Since dashboard.update()
    // runs after this method in loop(), large gaps can expose slow network work.
    const uint32_t updateStartedAt = millis();
    const uint32_t loopGapMs = static_cast<uint32_t>(updateStartedAt - _lastUpdateAt);
    _lastUpdateAt = updateStartedAt;
    ++_loopMetricCount;
    _loopMetricTotalMs += loopGapMs;
    if (loopGapMs > _loopMetricMaxGapMs)
        _loopMetricMaxGapMs = loopGapMs;
    if (loopGapMs >= Config::FRAME_INTERVAL_MS)
        ++_loopMetricOverBudget;

    handleSerialCommands();
    const uint32_t now = millis();

    const uint32_t elapsed = _timerPaused ? 0U : static_cast<uint32_t>(now - _lastWorkTick);
    switch (_mode)
    {
    case PetMode::Work: _workMs += elapsed; break;
    case PetMode::Play: _playMs += elapsed; break;
    case PetMode::Nap: _napMs += elapsed; break;
    }
    _lastWorkTick = now;
    syncDailyHistory(now);
    syncMonthlyHistory(now);
    recordDailyElapsed(elapsed);
    recordMonthlyElapsed(elapsed);
    persistStats(now);
    persistDailyHistory(now);
    persistMonthlyHistory(now);

    updateAutomaticMode(now);
    int8_t currentHour = -1;
    if (_rtc.isAvailable())
        currentHour = static_cast<int8_t>(_rtc.now().hour());

    const bool quietHours = isQuietHour(currentHour);
    for (uint8_t i = 0; i < PET_COUNT; ++i)
        _behaviors[i].update(_mode, now, currentHour, quietHours);
    persistPetProfiles(now);
    persistAchievementProgress(now);
    updateRtcReminder(now, quietHours);
    if (quietHours)
        _reminderMessage = nullptr;

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
                                  : _behaviors[_activePetId].state().message;

        char dateTime[24] = "--:--  ----/--/--";
        if (_rtc.isAvailable())
        {
            const DateTime current = _rtc.now();
            snprintf(dateTime, sizeof(dateTime), "%02u:%02u  %04u-%02u-%02u",
                     current.hour(), current.minute(), current.year(),
                     current.month(), current.day());
        }
        const bool attentionActive =
            _attentionEffectUntil != 0 &&
            static_cast<int32_t>(now - _attentionEffectUntil) < 0;
        const uint8_t attentionPhase = attentionActive
            ? static_cast<uint8_t>(((now - _attentionEffectStartedAt) / 250UL) % 4UL)
            : 0;
        const uint32_t drawStartedAtUs = micros();
        _renderer.draw(_display, _mode, _frame, _activePetId,
                       _behaviors[_activePetId].state().mood, message, dateTime,
                       attentionActive, attentionPhase);
        const uint32_t drawDurationUs = static_cast<uint32_t>(micros() - drawStartedAtUs);

        ++_frameMetricCount;
        _frameMetricTotalUs += drawDurationUs;
        if (drawDurationUs > _frameMetricMaxUs)
            _frameMetricMaxUs = drawDurationUs;
        if (drawDurationUs >= Config::FRAME_INTERVAL_MS * 1000UL)
            ++_frameMetricOverBudget;

        _lastFrame = now;

        const uint32_t metricsNow = millis();
        if (static_cast<uint32_t>(metricsNow - _lastFrameMetricsAt) >= 10000UL)
        {
            const uint32_t averageUs = _frameMetricCount > 0
                ? static_cast<uint32_t>(_frameMetricTotalUs / _frameMetricCount)
                : 0;
            const uint32_t loopAverageGapMs = _loopMetricCount > 0
                ? static_cast<uint32_t>(_loopMetricTotalMs / _loopMetricCount)
                : 0;
            const uint32_t metricsWindowMs = static_cast<uint32_t>(metricsNow - _lastFrameMetricsAt);
            Serial.printf(
                "[TFT_FRAME] window=%lu ms frames=%lu avg=%lu us max=%lu us overBudget=%lu\n",
                static_cast<unsigned long>(metricsWindowMs),
                static_cast<unsigned long>(_frameMetricCount),
                static_cast<unsigned long>(averageUs),
                static_cast<unsigned long>(_frameMetricMaxUs),
                static_cast<unsigned long>(_frameMetricOverBudget));
            Serial.printf(
                "[LOOP_TIMING] window=%lu ms calls=%lu avgGap=%lu ms maxGap=%lu ms overBudget=%lu\n",
                static_cast<unsigned long>(metricsWindowMs),
                static_cast<unsigned long>(_loopMetricCount),
                static_cast<unsigned long>(loopAverageGapMs),
                static_cast<unsigned long>(_loopMetricMaxGapMs),
                static_cast<unsigned long>(_loopMetricOverBudget));

            _lastFrameMetricsAt = metricsNow;
            _loopMetricCount = 0;
            _loopMetricTotalMs = 0;
            _loopMetricMaxGapMs = 0;
            _loopMetricOverBudget = 0;
            _frameMetricCount = 0;
            _frameMetricTotalUs = 0;
            _frameMetricMaxUs = 0;
            _frameMetricOverBudget = 0;
        }
    }
}
