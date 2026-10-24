#include "../../src/pet/PetBehavior.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

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

int main()
{
    testEnergyBoundsAndRates();
    testMillisRollover();
    testPersonalityProfiles();
    testSpontaneousMessageExpires();
    puts("PetBehavior native tests: PASS");
    return 0;
}
