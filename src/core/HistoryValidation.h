#pragma once
#include <stdint.h>
#include "HistoryDate.h"

namespace HistoryValidation
{
    constexpr uint64_t DAY_MS = 86400000ULL;
    constexpr uint32_t SESSIONS_PER_DAY_LIMIT = 1440U;

    template <typename Record>
    bool validDailyRecord(const Record &record)
    {
        if (!HistoryDate::isValidDateKey(record.dateKey)) return false;
        const uint64_t work = record.workMs, play = record.playMs, nap = record.napMs;
        if (work > DAY_MS || play > DAY_MS || nap > DAY_MS) return false;
        if (work > DAY_MS - play || work + play > DAY_MS - nap) return false;
        return record.completedWorkSessions <= SESSIONS_PER_DAY_LIMIT;
    }

    template <typename Record>
    bool validDailyHistory(const Record *records, uint8_t count, uint8_t capacity)
    {
        if (count > capacity || (count > 0 && records == nullptr)) return false;
        for (uint8_t i = 0; i < count; ++i)
            if (!validDailyRecord(records[i]) ||
                (i > 0 && records[i].dateKey <= records[i - 1].dateKey)) return false;
        return true;
    }

    template <typename Record>
    bool validMonthlyRecord(const Record &record)
    {
        if (!HistoryDate::isValidMonthKey(record.monthKey)) return false;
        const int32_t year = record.monthKey / 100, month = record.monthKey % 100;
        const uint64_t maxMs = static_cast<uint64_t>(HistoryDate::daysInMonth(year, month)) * DAY_MS;
        const uint32_t maxSessions = static_cast<uint32_t>(HistoryDate::daysInMonth(year, month)) * SESSIONS_PER_DAY_LIMIT;
        const uint64_t work = record.workMs, play = record.playMs, nap = record.napMs;
        if (work > maxMs || play > maxMs || nap > maxMs) return false;
        if (work > maxMs - play || work + play > maxMs - nap) return false;
        return record.completedWorkSessions <= maxSessions;
    }

    template <typename Record>
    bool validMonthlyHistory(const Record *records, uint8_t count, uint8_t capacity)
    {
        if (count > capacity || (count > 0 && records == nullptr)) return false;
        for (uint8_t i = 0; i < count; ++i)
            if (!validMonthlyRecord(records[i]) ||
                (i > 0 && records[i].monthKey <= records[i - 1].monthKey)) return false;
        return true;
    }
}
