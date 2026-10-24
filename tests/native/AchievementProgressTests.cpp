#include "../../src/core/AchievementProgress.h"
#include <cassert>
#include <cstdint>

int main()
{
    using namespace AchievementProgress;
    Storage progress{};
    assert(isValid(progress));
    assert(progress.globalMask == 0);

    assert(recordInteraction(progress, 0, 5) == (1UL << FirstInteraction));
    assert(progress.interactions[0] == 1);
    assert(recordInteraction(progress, 0, 10) == 0);
    for (uint8_t i = 2; i < 10; ++i)
        recordInteraction(progress, 0, 45);
    assert((progress.petMasks[0] & (1UL << TenInteractions)) != 0);
    assert(recordInteraction(progress, 0, 50) == (1UL << BondFifty));
    for (uint8_t i = 11; i < 25; ++i)
        recordInteraction(progress, 0, 50);
    assert((progress.petMasks[0] & (1UL << TwentyFiveInteractions)) != 0);
    assert(recordInteraction(progress, 0, 100) == (1UL << BondHundred));
    assert(recordInteraction(progress, 0, 100) == 0);
    assert(progress.interactions[1] == 0);
    assert(recordInteraction(progress, PET_COUNT, 50) == 0);
    assert(recordInteraction(progress, 1, 101) == 0);
    assert(progress.interactions[1] == 0);

    assert(recordFocusSessions(progress, 1) == (1UL << FirstFocusSession));
    assert(recordFocusSessions(progress, 4) == 0);
    assert(recordFocusSessions(progress, 5) == (1UL << FiveFocusSessions));
    assert(recordFocusSessions(progress, 25) == (1UL << TwentyFiveFocusSessions));
    assert(recordFocusSessions(progress, 100) == (1UL << HundredFocusSessions));
    assert(recordFocusSessions(progress, 200) == 0);

    assert(recordConsecutiveDays(progress, 2) == 0);
    assert(recordConsecutiveDays(progress, 3) == (1UL << ThreeDayRhythm));
    assert(recordConsecutiveDays(progress, 5) == 0);

    Storage copy = progress;
    assert(equals(progress, copy));
    copy.interactions[2] = 1;
    assert(!equals(progress, copy));
    copy = progress;
    copy.version = 99;
    assert(!isValid(copy));
    copy = progress;
    copy.petCount = 2;
    assert(!isValid(copy));
    copy = progress;
    copy.petMasks[0] |= 1UL << 7;
    assert(!isValid(copy));
    copy = progress;
    copy.globalMask |= 1UL << 8;
    assert(!isValid(copy));
    copy = progress;
    copy.reserved[1] = 1;
    assert(!isValid(copy));

    Storage saturated{};
    saturated.interactions[2] = UINT16_MAX;
    recordInteraction(saturated, 2, 0);
    assert(saturated.interactions[2] == UINT16_MAX);
    return 0;
}
