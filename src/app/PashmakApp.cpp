#include "PashmakApp.h"
#include "Config.h"
#include <Preferences.h>

namespace
{
    void appendJsonString(String &json, const char *value)
    {
        static const char JSON_HEX_DIGITS[] = "0123456789ABCDEF";
        json += '"';
        if (value != nullptr)
        {
            const unsigned char *cursor =
                reinterpret_cast<const unsigned char *>(value);
            while (*cursor != '\0')
            {
                const unsigned char ch = *cursor++;
                switch (ch)
                {
                case '"': json += F("\\\""); break;
                case '\\': json += F("\\\\"); break;
                case '\b': json += F("\\b"); break;
                case '\f': json += F("\\f"); break;
                case '\n': json += F("\\n"); break;
                case '\r': json += F("\\r"); break;
                case '\t': json += F("\\t"); break;
                default:
                    if (ch < 0x20)
                    {
                        json += F("\\u00");
                        json += JSON_HEX_DIGITS[(ch >> 4) & 0x0F];
                        json += JSON_HEX_DIGITS[ch & 0x0F];
                    }
                    else
                    {
                        json += static_cast<char>(ch);
                    }
                    break;
                }
            }
        }
        json += '"';
    }

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

uint8_t PashmakApp::activePetId() const
{
    return _activePetId;
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
    StatsSummary result{};
    if (days == 0)
        return result;

    const uint8_t included = days < _dailyHistoryCount ? days : _dailyHistoryCount;
    const uint8_t first = _dailyHistoryCount - included;
    for (uint8_t i = first; i < _dailyHistoryCount; ++i)
    {
        const DailyStats &day = _dailyHistory[i];
        result.workMs += day.workMs;
        result.playMs += day.playMs;
        result.napMs += day.napMs;
        result.completedWorkSessions += day.completedWorkSessions;
        ++result.daysIncluded;
    }
    return result;
}

PashmakApp::StatsSummary PashmakApp::summarizeMonth(uint16_t year, uint8_t month) const
{
    StatsSummary result{};
    if (month < 1 || month > 12)
        return result;

    const int32_t monthKey = static_cast<int32_t>(year) * 100 + month;
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
    {
        const MonthlyStats &record = _monthlyHistory[i];
        if (record.monthKey != monthKey)
            continue;

        result.workMs = record.workMs;
        result.playMs = record.playMs;
        result.napMs = record.napMs;
        result.completedWorkSessions = record.completedWorkSessions;
        result.daysIncluded = 0; // Monthly aggregates track totals, not individual day coverage.
        return result;
    }
    return result;
}

String PashmakApp::statisticsToJson(uint8_t recentDays) const
{
    const StatsSummary recent = summarizeRecentDays(recentDays);

    String json;
    json.reserve(180 + static_cast<size_t>(_dailyHistoryCount) * 100 +
                 static_cast<size_t>(_monthlyHistoryCount) * 140);

    const PetBehaviorState &pet = _behaviors[_activePetId].state();
    const char *mood = "curious";
    switch (pet.mood)
    {
    case PetMood::Happy: mood = "happy"; break;
    case PetMood::Focused: mood = "focused"; break;
    case PetMood::Sleepy: mood = "sleepy"; break;
    case PetMood::Curious: mood = "curious"; break;
    case PetMood::Excited: mood = "excited"; break;
    case PetMood::Tired: mood = "tired"; break;
    case PetMood::Exhausted: mood = "exhausted"; break;
    }

    json += F("{\"version\":1,\"activePetId\":");
    json += String(_activePetId);
    json += F(",\"pets\":[");
    const char *petNames[] = {"Pashmak", "Mochi", "Pixel"};
    for (uint8_t i = 0; i < 3; ++i)
    {
        if (i > 0) json += ',';
        const PetBehaviorState &profile = _behaviors[i].state();
        const char *profileMood = "curious";
        switch (profile.mood)
        {
        case PetMood::Happy: profileMood = "happy"; break;
        case PetMood::Focused: profileMood = "focused"; break;
        case PetMood::Sleepy: profileMood = "sleepy"; break;
        case PetMood::Curious: profileMood = "curious"; break;
        case PetMood::Excited: profileMood = "excited"; break;
        case PetMood::Tired: profileMood = "tired"; break;
        case PetMood::Exhausted: profileMood = "exhausted"; break;
        }
        json += F("{\"id\":"); json += String(i);
        json += F(",\"name\":"); appendJsonString(json, petNames[i]);
        json += F(",\"energy\":"); json += String(profile.energy);
        json += F(",\"mood\":"); appendJsonString(json, profileMood);
        json += F(",\"message\":");
        appendJsonString(json, profile.message);
        json += '}';
    }
    json += F("],\"pet\":{\"energy\":");
    json += String(pet.energy);
    json += F(",\"mood\":");
    appendJsonString(json, mood);
    json += F(",\"message\":");
    appendJsonString(json, pet.message);
    json += F("},\"totals\":{\"workMs\":");
    json += String(static_cast<unsigned long long>(_workMs));
    json += F(",\"playMs\":");
    json += String(static_cast<unsigned long long>(_playMs));
    json += F(",\"napMs\":");
    json += String(static_cast<unsigned long long>(_napMs));
    json += F(",\"completedWorkSessions\":");
    json += String(static_cast<unsigned long>(_completedWorkSessions));
    json += F("},\"recent\":{\"requestedDays\":");
    json += String(recentDays);
    json += F(",\"daysIncluded\":");
    json += String(recent.daysIncluded);
    json += F(",\"workMs\":");
    json += String(static_cast<unsigned long long>(recent.workMs));
    json += F(",\"playMs\":");
    json += String(static_cast<unsigned long long>(recent.playMs));
    json += F(",\"napMs\":");
    json += String(static_cast<unsigned long long>(recent.napMs));
    json += F(",\"completedWorkSessions\":");
    json += String(static_cast<unsigned long>(recent.completedWorkSessions));
    json += F("},\"dailyHistory\":[");

    for (uint8_t i = 0; i < _dailyHistoryCount; ++i)
    {
        if (i > 0)
            json += ',';

        const DailyStats &day = _dailyHistory[i];
        json += F("{\"dateKey\":");
        json += String(static_cast<long>(day.dateKey));
        json += F(",\"workMs\":");
        json += String(static_cast<unsigned long>(day.workMs));
        json += F(",\"playMs\":");
        json += String(static_cast<unsigned long>(day.playMs));
        json += F(",\"napMs\":");
        json += String(static_cast<unsigned long>(day.napMs));
        json += F(",\"completedWorkSessions\":");
        json += String(day.completedWorkSessions);
        json += '}';
    }

    json += F("],\"monthlyHistory\":[");
    for (uint8_t i = 0; i < _monthlyHistoryCount; ++i)
    {
        if (i > 0)
            json += ',';

        const MonthlyStats &month = _monthlyHistory[i];
        json += F("{\"monthKey\":");
        json += String(static_cast<long>(month.monthKey));
        json += F(",\"workMs\":");
        json += String(static_cast<unsigned long long>(month.workMs));
        json += F(",\"playMs\":");
        json += String(static_cast<unsigned long long>(month.playMs));
        json += F(",\"napMs\":");
        json += String(static_cast<unsigned long long>(month.napMs));
        json += F(",\"completedWorkSessions\":");
        json += String(static_cast<unsigned long>(month.completedWorkSessions));
        json += '}';
    }

    json += F("]}");
    return json;
}

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
        _activePetId = _statsPreferences.getUChar("activePet", 0);
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

    for (uint8_t i = 0; i < 3; ++i)
        _behaviors[i].begin(now);
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
        nullptr);

    Serial.println("PashmakOS: automatic pet behavior enabled");
}

void PashmakApp::setMode(PetMode mode, uint32_t now)
{
    if (_mode == mode)
        return;

    _mode = mode;
    _modeStartedAt = now;
    persistStats(now, true);
    persistDailyHistory(now, true);
    persistMonthlyHistory(now, true);
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
            ++_completedWorkSessions;
            recordCompletedWorkSession();
            recordMonthlyCompletedWorkSession();
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
        if (stored.records[i].dateKey < 20000101 ||
            stored.records[i].dateKey > 20991231)
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
    if (_currentDailyIndex < 0 ||
        _currentDailyIndex >= static_cast<int8_t>(_dailyHistoryCount))
        return;

    DailyStats &day = _dailyHistory[_currentDailyIndex];
    if (day.completedWorkSessions < UINT16_MAX)
        ++day.completedWorkSessions;
    persistDailyHistory(millis(), true);
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
        const int32_t year = key / 100;
        const int32_t month = key % 100;
        if (year < 2000 || year > 2099 || month < 1 || month > 12 ||
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
                    Serial.printf("%ld,%lu,%lu,%lu,%u\\n",
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
                    Serial.printf("%ld,%llu,%llu,%llu,%lu\\n",
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
    handleSerialCommands();
    const uint32_t now = millis();

    const uint32_t elapsed = static_cast<uint32_t>(now - _lastWorkTick);
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

    for (uint8_t i = 0; i < 3; ++i)
        _behaviors[i].update(_mode, now, currentHour);
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
                                  : _behaviors[_activePetId].state().message;

        char dateTime[24] = "--:--  ----/--/--";
        if (_rtc.isAvailable())
        {
            const DateTime current = _rtc.now();
            snprintf(dateTime, sizeof(dateTime), "%02u:%02u  %04u-%02u-%02u",
                     current.hour(), current.minute(), current.year(),
                     current.month(), current.day());
        }
        _renderer.draw(_display, _mode, _frame, _activePetId, _behaviors[_activePetId].state().mood, message, dateTime);
        _lastFrame = now;
    }
}
