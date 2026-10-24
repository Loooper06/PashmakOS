#pragma once

#include <stdint.h>

namespace AchievementProgress
{
    constexpr uint32_t STORAGE_VERSION = 1;
    constexpr uint8_t PET_COUNT = 3;
    constexpr uint32_t PET_ACHIEVEMENT_MASK = 0x1FU;
    constexpr uint32_t GLOBAL_ACHIEVEMENT_MASK = 0x1FU;

    enum PetAchievement : uint8_t
    {
        FirstInteraction = 0,
        TenInteractions = 1,
        TwentyFiveInteractions = 2,
        BondFifty = 3,
        BondHundred = 4
    };

    enum GlobalAchievement : uint8_t
    {
        FirstFocusSession = 0,
        FiveFocusSessions = 1,
        TwentyFiveFocusSessions = 2,
        HundredFocusSessions = 3,
        ThreeDayRhythm = 4
    };

    struct Storage
    {
        uint32_t version = STORAGE_VERSION;
        uint8_t petCount = PET_COUNT;
        uint8_t reserved[3]{};
        uint16_t interactions[PET_COUNT]{};
        uint32_t petMasks[PET_COUNT]{};
        uint32_t globalMask = 0;
    };

    inline bool isValid(const Storage &stored)
    {
        if (stored.version != STORAGE_VERSION || stored.petCount != PET_COUNT ||
            stored.reserved[0] != 0 || stored.reserved[1] != 0 || stored.reserved[2] != 0 ||
            (stored.globalMask & ~GLOBAL_ACHIEVEMENT_MASK) != 0)
            return false;
        for (uint8_t i = 0; i < PET_COUNT; ++i)
            if ((stored.petMasks[i] & ~PET_ACHIEVEMENT_MASK) != 0)
                return false;
        return true;
    }

    inline bool equals(const Storage &a, const Storage &b)
    {
        if (a.version != b.version || a.petCount != b.petCount || a.globalMask != b.globalMask)
            return false;
        for (uint8_t i = 0; i < PET_COUNT; ++i)
            if (a.interactions[i] != b.interactions[i] || a.petMasks[i] != b.petMasks[i])
                return false;
        return true;
    }

    inline uint32_t recordInteraction(Storage &progress, uint8_t petId, uint8_t bond)
    {
        if (petId >= PET_COUNT || bond > 100)
            return 0;
        if (progress.interactions[petId] < UINT16_MAX)
            ++progress.interactions[petId];
        const uint16_t count = progress.interactions[petId];
        uint32_t eligible = 0;
        if (count >= 1) eligible |= 1UL << FirstInteraction;
        if (count >= 10) eligible |= 1UL << TenInteractions;
        if (count >= 25) eligible |= 1UL << TwentyFiveInteractions;
        if (bond >= 50) eligible |= 1UL << BondFifty;
        if (bond >= 100) eligible |= 1UL << BondHundred;
        const uint32_t newlyUnlocked = eligible & ~progress.petMasks[petId];
        progress.petMasks[petId] |= eligible;
        return newlyUnlocked;
    }

    inline uint32_t recordFocusSessions(Storage &progress, uint32_t completedSessions)
    {
        uint32_t eligible = 0;
        if (completedSessions >= 1) eligible |= 1UL << FirstFocusSession;
        if (completedSessions >= 5) eligible |= 1UL << FiveFocusSessions;
        if (completedSessions >= 25) eligible |= 1UL << TwentyFiveFocusSessions;
        if (completedSessions >= 100) eligible |= 1UL << HundredFocusSessions;
        const uint32_t newlyUnlocked = eligible & ~progress.globalMask;
        progress.globalMask |= eligible;
        return newlyUnlocked;
    }

    inline uint32_t recordConsecutiveDays(Storage &progress, uint8_t consecutiveDays)
    {
        if (consecutiveDays < 3)
            return 0;
        const uint32_t bit = 1UL << ThreeDayRhythm;
        const uint32_t newlyUnlocked = (progress.globalMask & bit) != 0 ? 0 : bit;
        progress.globalMask |= bit;
        return newlyUnlocked;
    }
}
