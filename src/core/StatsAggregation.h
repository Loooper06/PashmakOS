#pragma once

#include <stdint.h>

namespace StatsAggregation
{
    // Records must be in chronological order, oldest first.
    template <typename DailyRecord, typename Summary>
    Summary summarizeRecentDays(const DailyRecord *records, uint8_t count, uint8_t requestedDays)
    {
        Summary result{};
        if (records == nullptr || requestedDays == 0 || count == 0)
            return result;

        const uint8_t included = requestedDays < count ? requestedDays : count;
        const uint8_t first = count - included;
        for (uint8_t i = first; i < count; ++i)
        {
            result.workMs += records[i].workMs;
            result.playMs += records[i].playMs;
            result.napMs += records[i].napMs;
            result.completedWorkSessions += records[i].completedWorkSessions;
            ++result.daysIncluded;
        }
        return result;
    }

    template <typename MonthlyRecord, typename Summary>
    Summary summarizeMonth(const MonthlyRecord *records, uint8_t count, uint16_t year, uint8_t month)
    {
        Summary result{};
        if (records == nullptr || month < 1 || month > 12)
            return result;

        const int32_t monthKey = static_cast<int32_t>(year) * 100 + month;
        for (uint8_t i = 0; i < count; ++i)
        {
            const MonthlyRecord &record = records[i];
            if (record.monthKey != monthKey)
                continue;

            result.workMs = record.workMs;
            result.playMs = record.playMs;
            result.napMs = record.napMs;
            result.completedWorkSessions = record.completedWorkSessions;
            // Monthly aggregates track totals, not individual day coverage.
            result.daysIncluded = 0;
            return result;
        }
        return result;
    }
}
