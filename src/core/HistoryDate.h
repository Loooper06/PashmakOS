#pragma once

#include <stdint.h>

namespace HistoryDate
{
    constexpr bool isLeapYear(int32_t year)
    {
        return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0));
    }

    constexpr uint8_t daysInMonth(int32_t year, int32_t month)
    {
        switch (month)
        {
        case 1: case 3: case 5: case 7: case 8: case 10: case 12:
            return 31;
        case 4: case 6: case 9: case 11:
            return 30;
        case 2:
            return isLeapYear(year) ? 29 : 28;
        default:
            return 0;
        }
    }

    constexpr bool isValidDateKey(int32_t dateKey)
    {
        const int32_t year = dateKey / 10000;
        const int32_t month = (dateKey / 100) % 100;
        const int32_t day = dateKey % 100;
        return year >= 2000 && year <= 2099 &&
               month >= 1 && month <= 12 &&
               day >= 1 && day <= daysInMonth(year, month);
    }

    constexpr bool isValidMonthKey(int32_t monthKey)
    {
        const int32_t year = monthKey / 100;
        const int32_t month = monthKey % 100;
        return year >= 2000 && year <= 2099 && month >= 1 && month <= 12;
    }
}
