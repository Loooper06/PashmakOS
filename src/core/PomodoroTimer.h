#pragma once

#include <stdint.h>

namespace PomodoroTimer
{
    inline uint32_t elapsedMs(uint32_t now, uint32_t startedAt)
    {
        // Unsigned subtraction is defined modulo 2^32 and survives millis() rollover.
        return static_cast<uint32_t>(now - startedAt);
    }

    inline uint32_t remainingMs(uint32_t now, uint32_t startedAt, uint32_t durationMs)
    {
        const uint32_t elapsed = elapsedMs(now, startedAt);
        return elapsed >= durationMs ? 0U : durationMs - elapsed;
    }

    inline uint32_t remainingAtPause(uint32_t now, uint32_t startedAt, uint32_t durationMs)
    {
        return remainingMs(now, startedAt, durationMs);
    }

    inline uint32_t resumedStartAt(uint32_t now, uint32_t durationMs, uint32_t remaining)
    {
        const uint32_t safeRemaining = remaining > durationMs ? durationMs : remaining;
        return static_cast<uint32_t>(now - (durationMs - safeRemaining));
    }
}
