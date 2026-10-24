#pragma once

#include <stdint.h>
#include "JsonEscape.h"

namespace StatsJson
{
    struct Counters
    {
        uint64_t workMs;
        uint64_t playMs;
        uint64_t napMs;
        uint32_t completedWorkSessions;
    };

    struct Pet
    {
        uint8_t id;
        uint8_t energy;
        const char *name;
        const char *mood;
        const char *message;
    };

    struct Recent
    {
        uint8_t requestedDays;
        uint8_t daysIncluded;
        Counters totals;
    };

    struct DailyRecord
    {
        int32_t dateKey;
        uint32_t workMs;
        uint32_t playMs;
        uint32_t napMs;
        uint16_t completedWorkSessions;
    };

    struct MonthlyRecord
    {
        int32_t monthKey;
        uint64_t workMs;
        uint64_t playMs;
        uint64_t napMs;
        uint32_t completedWorkSessions;
    };

    struct Payload
    {
        uint8_t activePetId;
        const Pet *pets;
        uint8_t petCount;
        Pet activePet;
        Counters totals;
        Recent recent;
        const DailyRecord *dailyHistory;
        uint8_t dailyCount;
        const MonthlyRecord *monthlyHistory;
        uint8_t monthlyCount;
    };

    template <typename StringLike>
    void appendUnsigned(StringLike &json, uint64_t value)
    {
        char digits[20];
        uint8_t length = 0;
        do
        {
            digits[length++] = static_cast<char>('0' + (value % 10));
            value /= 10;
        } while (value != 0);

        while (length > 0)
            json += digits[--length];
    }

    template <typename StringLike>
    void appendSigned(StringLike &json, int64_t value)
    {
        if (value < 0)
        {
            json += '-';
            // Avoid signed overflow for the minimum representable value.
            appendUnsigned(json, static_cast<uint64_t>(-(value + 1)) + 1U);
        }
        else
        {
            appendUnsigned(json, static_cast<uint64_t>(value));
        }
    }

    template <typename StringLike>
    void appendCounters(StringLike &json, const Counters &counters)
    {
        json += F("{\"workMs\":");
        appendUnsigned(json, counters.workMs);
        json += F(",\"playMs\":");
        appendUnsigned(json, counters.playMs);
        json += F(",\"napMs\":");
        appendUnsigned(json, counters.napMs);
        json += F(",\"completedWorkSessions\":");
        appendUnsigned(json, counters.completedWorkSessions);
        json += '}';
    }

    template <typename StringLike>
    void appendPet(StringLike &json, const Pet &pet, bool includeId)
    {
        json += '{';
        if (includeId)
        {
            json += F("\"id\":");
            appendUnsigned(json, pet.id);
            json += ',';
            json += F("\"name\":");
            appendJsonString(json, pet.name);
            json += ',';
        }
        json += F("\"energy\":");
        appendUnsigned(json, pet.energy);
        json += F(",\"mood\":");
        appendJsonString(json, pet.mood);
        json += F(",\"message\":");
        appendJsonString(json, pet.message);
        json += '}';
    }

    template <typename StringLike>
    void appendStatisticsJson(StringLike &json, const Payload &payload)
    {
        json += F("{\"version\":1,\"activePetId\":");
        appendUnsigned(json, payload.activePetId);
        json += F(",\"pets\":[");
        for (uint8_t i = 0; i < payload.petCount; ++i)
        {
            if (i != 0)
                json += ',';
            appendPet(json, payload.pets[i], true);
        }

        json += F("],\"pet\":");
        appendPet(json, payload.activePet, false);
        json += F(",\"totals\":");
        appendCounters(json, payload.totals);
        json += F(",\"recent\":{\"requestedDays\":");
        appendUnsigned(json, payload.recent.requestedDays);
        json += F(",\"daysIncluded\":");
        appendUnsigned(json, payload.recent.daysIncluded);
        json += ',';
        appendCounters(json, payload.recent.totals);
        json += F("},\"dailyHistory\":[");

        for (uint8_t i = 0; i < payload.dailyCount; ++i)
        {
            if (i != 0)
                json += ',';
            const DailyRecord &day = payload.dailyHistory[i];
            json += F("{\"dateKey\":");
            appendSigned(json, day.dateKey);
            json += F(",\"workMs\":");
            appendUnsigned(json, day.workMs);
            json += F(",\"playMs\":");
            appendUnsigned(json, day.playMs);
            json += F(",\"napMs\":");
            appendUnsigned(json, day.napMs);
            json += F(",\"completedWorkSessions\":");
            appendUnsigned(json, day.completedWorkSessions);
            json += '}';
        }

        json += F("],\"monthlyHistory\":[");
        for (uint8_t i = 0; i < payload.monthlyCount; ++i)
        {
            if (i != 0)
                json += ',';
            const MonthlyRecord &month = payload.monthlyHistory[i];
            json += F("{\"monthKey\":");
            appendSigned(json, month.monthKey);
            json += F(",\"workMs\":");
            appendUnsigned(json, month.workMs);
            json += F(",\"playMs\":");
            appendUnsigned(json, month.playMs);
            json += F(",\"napMs\":");
            appendUnsigned(json, month.napMs);
            json += F(",\"completedWorkSessions\":");
            appendUnsigned(json, month.completedWorkSessions);
            json += '}';
        }
        json += F("]}");
    }
}
