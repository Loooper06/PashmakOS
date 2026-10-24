#include "../../src/pet/PetBehavior.h"
#include "../../src/core/JsonEscape.h"
#include "../../src/core/HistoryDate.h"
#include "../../src/core/StatsAggregation.h"
#include <string>
#include <assert.h>
#include <stdio.h>
#include <string.h>


class TestJsonString
{
public:
    std::string value;

    TestJsonString &operator+=(char ch)
    {
        value += ch;
        return *this;
    }

    TestJsonString &operator+=(const char *text)
    {
        value += text;
        return *this;
    }
};

static void testJsonStringEscaping()
{
    const char input[] =
        "quote:\" slash:\\ backspace:\b formfeed:\f newline:\n "
        "carriage:\r tab:\t controls:\x01\x1F utf8: \xC3\xA9";
    const std::string expected =
        "\"quote:\\\" slash:\\\\ backspace:\\b formfeed:\\f newline:\\n "
        "carriage:\\r tab:\\t controls:\\u0001\\u001F utf8: \xC3\xA9\"";

    TestJsonString output;
    appendJsonString(output, input);
    assert(output.value == expected);

    TestJsonString nullOutput;
    appendJsonString(nullOutput, nullptr);
    assert(nullOutput.value == "\"\"");
}

static void testEnergyBoundsAndRates()
{
    PetBehavior pet;
    pet.begin(0);
    pet.update(PetMode::Work, 120000U);
    assert(pet.state().energy == 79);

    // Switch modes at the same timestamp so no elapsed time is charged
    // to the newly selected mode before its interval actually begins.
    pet.update(PetMode::Nap, 120000U);
    assert(pet.state().energy == 79);
    pet.update(PetMode::Nap, 150000U);
    assert(pet.state().energy == 80);

    pet.update(PetMode::Play, 150000U);
    assert(pet.state().energy == 80);
    pet.update(PetMode::Play, 210000U);
    assert(pet.state().energy == 81);

    pet.update(PetMode::Nap, 210000U);
    pet.update(PetMode::Nap, 240000U);
    assert(pet.state().energy == 82);

    pet.update(PetMode::Nap, 1200000U);
    assert(pet.state().energy == 100);
}

static void testMillisRollover()
{
    const uint32_t start = UINT32_MAX - 60000UL;
    PetBehavior pet;
    pet.begin(start);
    const uint32_t workElapsed = static_cast<uint32_t>(start + 120000U);
    const uint32_t napStart = workElapsed;
    const uint32_t napElapsed = static_cast<uint32_t>(napStart + 30000U);

    pet.update(PetMode::Work, workElapsed);
    assert(pet.state().energy == 79);

    pet.update(PetMode::Nap, napStart);
    assert(pet.state().energy == 79);
    pet.update(PetMode::Nap, napElapsed);
    assert(pet.state().energy == 80);
}

static void testPersonalityProfiles()
{
    PetBehavior pashmak(0);
    PetBehavior mochi(1);
    PetBehavior pixel(2);
    pashmak.begin(0);
    mochi.begin(0);
    pixel.begin(0);
    pashmak.update(PetMode::Work, 1);
    mochi.update(PetMode::Work, 1);
    pixel.update(PetMode::Work, 1);

    assert(strcmp(pashmak.state().message, mochi.state().message) != 0);
    assert(strcmp(mochi.state().message, pixel.state().message) != 0);
}

static void testSpontaneousMessageExpires()
{
    PetBehavior pet;
    pet.begin(0);
    pet.update(PetMode::Work, 90000UL);
    const char *eventMessage = pet.state().message;
    assert(eventMessage != nullptr);

    pet.update(PetMode::Work, 97000UL);
    assert(strcmp(pet.state().message, eventMessage) != 0);
}

static void testTimeAwareMessages()
{
    PetBehavior pashmak(0);
    PetBehavior mochi(1);
    PetBehavior pixel(2);
    pashmak.update(PetMode::Work, 0, 8);
    mochi.update(PetMode::Work, 0, 8);
    pixel.update(PetMode::Work, 0, 8);

    assert(strstr(pashmak.state().message, "morning") != nullptr);
    assert(strstr(mochi.state().message, "morning") != nullptr);
    assert(strstr(pixel.state().message, "morning") != nullptr);
    assert(strcmp(pashmak.state().message, mochi.state().message) != 0);
    assert(strcmp(mochi.state().message, pixel.state().message) != 0);
}

static void testTimeAwareMessageDoesNotRepeat()
{
    PetBehavior pet(0);
    pet.update(PetMode::Work, 0, 8);
    const std::string first = pet.state().message;
    assert(first.find("morning") != std::string::npos);

    // The message interval expires while the RTC hour is unchanged. The
    // behavior engine should fall back to the personality pool, not repeat
    // the same time-aware line.
    pet.update(PetMode::Work, 45000UL, 8);
    assert(std::string(pet.state().message) != first);
}

static void testLowEnergyWarningsAreNotReplacedByRepeatFallback()
{
    PetBehavior tired(0);
    tired.begin(0);
    assert(tired.restoreEnergy(20, 0));
    tired.update(PetMode::Work, 45000UL, 8, true);
    assert(strcmp(tired.state().message, "I'm getting tired.") == 0);
    tired.update(PetMode::Work, 90000UL, 8, true);
    assert(strcmp(tired.state().message, "I'm getting tired.") == 0);

    PetBehavior exhausted(0);
    exhausted.begin(0);
    assert(exhausted.restoreEnergy(5, 0));
    exhausted.update(PetMode::Work, 45000UL, 8, true);
    assert(strcmp(exhausted.state().message, "I need a longer rest.") == 0);
    exhausted.update(PetMode::Work, 90000UL, 8, true);
    assert(strcmp(exhausted.state().message, "I need a longer rest.") == 0);
}

static void testSpontaneousEventsStayQuietDuringNap()
{
    PetBehavior pet(1);
    pet.begin(0);
    pet.update(PetMode::Nap, 90000UL);
    const std::string napMessage = pet.state().message;
    assert(napMessage == "A cozy little pause.");

    // The overdue event is deferred during Nap, then becomes eligible only
    // after its newly scheduled interval has elapsed in an active mode.
    pet.update(PetMode::Work, 179999UL);
    assert(std::string(pet.state().message) != "Just checking in, friend.");
    pet.update(PetMode::Work, 180000UL);
    assert(std::string(pet.state().message) == "Just checking in, friend.");
}

static void testEventExpiryAcrossMillisRollover()
{
    const uint32_t start = UINT32_MAX - 20000UL;
    PetBehavior pet;
    pet.begin(start);
    const char *baseMessage = pet.state().message;
    const uint32_t eventAt = static_cast<uint32_t>(start + 90000UL);
    pet.update(PetMode::Work, eventAt);
    assert(strcmp(pet.state().message, baseMessage) != 0);

    const uint32_t expiryAt = static_cast<uint32_t>(eventAt + 7000UL);
    pet.update(PetMode::Work, expiryAt);
    assert(strcmp(pet.state().message, baseMessage) == 0);
}

static void testEventExpiryRestoresLatestModeMessage()
{
    PetBehavior pet(1);
    pet.begin(0);
    pet.update(PetMode::Work, 90000UL);
    assert(strcmp(pet.state().message, "Just checking in, friend.") == 0);

    // A mode change updates the baseline while the temporary event remains visible.
    pet.update(PetMode::Play, 91000UL);
    assert(strcmp(pet.state().message, "Just checking in, friend.") == 0);

    // Expiry must restore the newest baseline, not the message from before the event.
    pet.update(PetMode::Play, 97000UL);
    assert(strcmp(pet.state().message, "A little stretch together?") == 0);
}

static void testEnergyLowerBoundAndRecovery()
{
    PetBehavior pet;
    pet.begin(0);
    pet.update(PetMode::Work, 12000000UL);
    assert(pet.state().energy == 0);
    assert(pet.state().mood == PetMood::Exhausted);

    pet.update(PetMode::Nap, 12000000UL);
    pet.update(PetMode::Nap, 12030000UL);
    assert(pet.state().energy == 1);
    assert(pet.state().energy <= 100);
}

static void testQuietHoursSuppressSpontaneousEvents()
{
    PetBehavior pet(0);
    pet.begin(0);
    pet.update(PetMode::Work, 90000UL);
    assert(strcmp(pet.state().message, "Hey! Still there, Engineer?") == 0);

    // Quiet hours immediately clear an already visible spontaneous interruption.
    pet.update(PetMode::Work, 91000UL, 22, true);
    assert(strcmp(pet.state().message, "Let's Focus, Engineer!") == 0);

    // New spontaneous events remain suppressed for the entire quiet window.
    pet.update(PetMode::Work, 300000UL, 23, true);
    const char *message = pet.state().message;
    assert(strcmp(message, "Hey! Still there, Engineer?") != 0);
    assert(strcmp(message, "A tiny break can help.") != 0);
    assert(strcmp(message, "How about a sip of water?") != 0);
    assert(strcmp(message, "You're making progress!") != 0);
    assert(strcmp(message, "Remember to relax your shoulders.") != 0);
    assert(strcmp(message, "One step at a time.") != 0);
}

static void testPetInteractionReactionsAndExpiry()
{
    PetBehavior pashmak(0);
    PetBehavior mochi(1);
    PetBehavior pixel(2);
    pashmak.begin(0);
    mochi.begin(0);
    pixel.begin(0);

    pashmak.interact(PetMode::Work, 10, 5);
    mochi.interact(PetMode::Work, 10, 75);
    pixel.interact(PetMode::Nap, 10, 30);
    assert(strcmp(pashmak.state().message, "Happy tail wag for you!") == 0);
    assert(strcmp(mochi.state().message, "My favorite human!") == 0);
    assert(strcmp(pixel.state().message, "Quiet ping received.") == 0);

    pashmak.update(PetMode::Work, 1000);
    assert(strcmp(pashmak.state().message, "Happy tail wag for you!") == 0);
    pashmak.update(PetMode::Work, 6010);
    assert(strcmp(pashmak.state().message, "Let's Focus, Engineer!") == 0);
}

static void testBondRelationshipTiers()
{
    PetBehavior pashmak(0);
    PetBehavior mochi(1);
    PetBehavior pixel(2);
    pashmak.begin(0);
    mochi.begin(0);
    pixel.begin(0);

    pashmak.interact(PetMode::Work, 1, 24);
    assert(strcmp(pashmak.state().message, "Happy tail wag for you!") == 0);
    pashmak.interact(PetMode::Work, 2, 25);
    assert(strcmp(pashmak.state().message, "I'm happy to see you again!") == 0);
    pashmak.interact(PetMode::Work, 3, 49);
    assert(strcmp(pashmak.state().message, "I'm happy to see you again!") == 0);
    pashmak.interact(PetMode::Work, 4, 50);
    assert(strcmp(pashmak.state().message, "We make a great team!") == 0);
    pashmak.interact(PetMode::Work, 5, 74);
    assert(strcmp(pashmak.state().message, "We make a great team!") == 0);
    pashmak.interact(PetMode::Work, 6, 75);
    assert(strcmp(pashmak.state().message, "You're my best friend!") == 0);
    pashmak.interact(PetMode::Work, 7, 100);
    assert(strcmp(pashmak.state().message, "You're my best friend!") == 0);

    mochi.interact(PetMode::Work, 8, 50);
    assert(strcmp(mochi.state().message, "Our bond feels cozy.") == 0);
    pixel.interact(PetMode::Work, 9, 25);
    assert(strcmp(pixel.state().message, "Familiar human detected.") == 0);
}

static void testInvalidPersonalityFallsBackSafely()
{
    PetBehavior fallback(0);
    PetBehavior invalid(255);
    fallback.begin(0);
    invalid.begin(0);
    fallback.update(PetMode::Work, 1);
    invalid.update(PetMode::Work, 1);
    assert(strcmp(fallback.state().message, invalid.state().message) == 0);
}

struct TestDailyStats
{
    int32_t dateKey;
    uint64_t workMs;
    uint64_t playMs;
    uint64_t napMs;
    uint32_t completedWorkSessions;
};

struct TestMonthlyStats
{
    int32_t monthKey;
    uint64_t workMs;
    uint64_t playMs;
    uint64_t napMs;
    uint32_t completedWorkSessions;
};

struct TestStatsSummary
{
    uint64_t workMs = 0;
    uint64_t playMs = 0;
    uint64_t napMs = 0;
    uint32_t completedWorkSessions = 0;
    uint8_t daysIncluded = 0;
};

static void testRecentStatisticsAggregation()
{
    const TestDailyStats days[] = {
        {20261020, 100, 10, 1, 1},
        {20261021, 200, 20, 2, 2},
        {20261022, 300, 30, 3, 3},
        {20261023, 400, 40, 4, 4},
    };

    const TestStatsSummary lastTwo =
        StatsAggregation::summarizeRecentDays<TestDailyStats, TestStatsSummary>(days, 4, 2);
    assert(lastTwo.workMs == 700);
    assert(lastTwo.playMs == 70);
    assert(lastTwo.napMs == 7);
    assert(lastTwo.completedWorkSessions == 7);
    assert(lastTwo.daysIncluded == 2);

    const TestStatsSummary allDays =
        StatsAggregation::summarizeRecentDays<TestDailyStats, TestStatsSummary>(days, 4, 99);
    assert(allDays.workMs == 1000);
    assert(allDays.playMs == 100);
    assert(allDays.napMs == 10);
    assert(allDays.completedWorkSessions == 10);
    assert(allDays.daysIncluded == 4);

    const TestStatsSummary noDays =
        StatsAggregation::summarizeRecentDays<TestDailyStats, TestStatsSummary>(days, 4, 0);
    assert(noDays.workMs == 0);
    assert(noDays.daysIncluded == 0);

    const TestStatsSummary empty =
        StatsAggregation::summarizeRecentDays<TestDailyStats, TestStatsSummary>(nullptr, 4, 7);
    assert(empty.workMs == 0);
    assert(empty.daysIncluded == 0);
}

static void testMonthlyStatisticsAggregation()
{
    const TestMonthlyStats months[] = {
        {202608, 1000, 100, 10, 2},
        {202609, 2000, 200, 20, 4},
        {202610, 3000, 300, 30, 6},
    };

    const TestStatsSummary october =
        StatsAggregation::summarizeMonth<TestMonthlyStats, TestStatsSummary>(months, 3, 2026, 10);
    assert(october.workMs == 3000);
    assert(october.playMs == 300);
    assert(october.napMs == 30);
    assert(october.completedWorkSessions == 6);
    assert(october.daysIncluded == 0);

    const TestStatsSummary missing =
        StatsAggregation::summarizeMonth<TestMonthlyStats, TestStatsSummary>(months, 3, 2026, 11);
    assert(missing.workMs == 0);
    assert(missing.completedWorkSessions == 0);

    const TestStatsSummary invalidMonth =
        StatsAggregation::summarizeMonth<TestMonthlyStats, TestStatsSummary>(months, 3, 2026, 13);
    assert(invalidMonth.workMs == 0);

    const TestStatsSummary empty =
        StatsAggregation::summarizeMonth<TestMonthlyStats, TestStatsSummary>(nullptr, 3, 2026, 10);
    assert(empty.workMs == 0);
}

static void testHistoryDateValidation()
{
    assert(HistoryDate::isValidDateKey(20261024));
    assert(HistoryDate::isValidDateKey(20240229));
    assert(!HistoryDate::isValidDateKey(20230229));
    assert(!HistoryDate::isValidDateKey(20260431));
    assert(!HistoryDate::isValidDateKey(20261301));
    assert(!HistoryDate::isValidDateKey(20260015));
    assert(!HistoryDate::isValidDateKey(19991231));
    assert(!HistoryDate::isValidDateKey(21000101));

    assert(HistoryDate::isValidMonthKey(202610));
    assert(HistoryDate::isValidMonthKey(200001));
    assert(!HistoryDate::isValidMonthKey(202600));
    assert(!HistoryDate::isValidMonthKey(202613));
    assert(!HistoryDate::isValidMonthKey(199912));
}

int main()
{
    testJsonStringEscaping();
    testHistoryDateValidation();
    testRecentStatisticsAggregation();
    testMonthlyStatisticsAggregation();
    testEnergyBoundsAndRates();
    testMillisRollover();
    testPersonalityProfiles();
    testSpontaneousMessageExpires();
    testTimeAwareMessages();
    testTimeAwareMessageDoesNotRepeat();
    testLowEnergyWarningsAreNotReplacedByRepeatFallback();
    testSpontaneousEventsStayQuietDuringNap();
    testEventExpiryAcrossMillisRollover();
    testEventExpiryRestoresLatestModeMessage();
    testEnergyLowerBoundAndRecovery();
    testInvalidPersonalityFallsBackSafely();
    testPetInteractionReactionsAndExpiry();
    testBondRelationshipTiers();
    testQuietHoursSuppressSpontaneousEvents();
    puts("PetBehavior native tests: PASS");
    return 0;
}
