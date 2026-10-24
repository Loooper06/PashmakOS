#include "../../src/core/HistoryValidation.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
struct Daily { int32_t dateKey; uint32_t workMs, playMs, napMs; uint16_t completedWorkSessions; };
struct Monthly { int32_t monthKey; uint64_t workMs, playMs, napMs; uint32_t completedWorkSessions; };
int main()
{
    const Daily valid[]={{20260228,3600000U,1800000U,600000U,4},{20260301,0U,0U,0U,0}};
    assert(HistoryValidation::validDailyHistory(valid,2,14));
    assert(HistoryValidation::validDailyHistory(static_cast<const Daily *>(nullptr),0,14));
    assert(!HistoryValidation::validDailyHistory(valid,15,14));
    const Daily leapInvalid[]={{20260229,0U,0U,0U,0}};
    assert(!HistoryValidation::validDailyHistory(leapInvalid,1,14));
    const Daily duplicate[]={{20261024,100U,0U,0U,1},{20261024,200U,0U,0U,1}};
    assert(!HistoryValidation::validDailyHistory(duplicate,2,14));
    const Daily overDay[]={{20261024,80000000U,5000000U,0U,1}};
    assert(!HistoryValidation::validDailyHistory(overDay,1,14));
    const Daily overSessions[]={{20261024,0U,0U,0U,1441}};
    assert(!HistoryValidation::validDailyHistory(overSessions,1,14));

    const Monthly months[]={{202602,20ULL*3600000ULL,10ULL*3600000ULL,5ULL*3600000ULL,100},{202603,0ULL,0ULL,0ULL,0}};
    assert(HistoryValidation::validMonthlyHistory(months,2,12));
    const Monthly impossibleFebruary[]={{202602,28ULL*HistoryValidation::DAY_MS,1ULL,0ULL,1}};
    assert(!HistoryValidation::validMonthlyHistory(impossibleFebruary,1,12));
    const Monthly invalidMonth[]={{202613,0ULL,0ULL,0ULL,0}};
    assert(!HistoryValidation::validMonthlyHistory(invalidMonth,1,12));
    const Monthly unordered[]={{202610,0ULL,0ULL,0ULL,0},{202609,0ULL,0ULL,0ULL,0}};
    assert(!HistoryValidation::validMonthlyHistory(unordered,2,12));
    puts("History validation: PASS");
}
