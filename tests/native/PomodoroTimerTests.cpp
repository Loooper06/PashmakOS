#include "../../src/core/PomodoroTimer.h"

#include <cassert>
#include <cstdint>
#include <limits>

int main()
{
    using PomodoroTimer::elapsedMs;
    using PomodoroTimer::remainingAtPause;
    using PomodoroTimer::remainingMs;
    using PomodoroTimer::resumedStartAt;

    constexpr uint32_t duration = 25UL * 60UL * 1000UL;
    assert(remainingMs(1000, 1000, duration) == duration);
    assert(remainingMs(61000, 1000, duration) == duration - 60000UL);
    assert(remainingMs(1000 + duration, 1000, duration) == 0);
    assert(remainingMs(1001 + duration, 1000, duration) == 0);

    // millis() rollover: start just before UINT32_MAX, then advance 1500 ms.
    const uint32_t start = std::numeric_limits<uint32_t>::max() - 999U;
    const uint32_t now = 500U;
    assert(elapsedMs(now, start) == 1500U);
    assert(remainingMs(now, start, 5000U) == 3500U);
    assert(remainingAtPause(now, start, 5000U) == 3500U);

    const uint32_t resumedAt = resumedStartAt(10000U, 5000U, 3500U);
    assert(resumedAt == 8500U);
    assert(remainingMs(10000U, resumedAt, 5000U) == 3500U);
    assert(remainingMs(11000U, resumedAt, 5000U) == 2500U);

    // A corrupt/oversized remaining value is clamped to the configured duration.
    assert(resumedStartAt(10000U, 5000U, 9000U) == 10000U);
    assert(remainingMs(10000U, resumedStartAt(10000U, 5000U, 9000U), 5000U) == 5000U);
    assert(remainingMs(10000U, resumedStartAt(10000U, 5000U, 0U), 5000U) == 0U);
    return 0;
}
