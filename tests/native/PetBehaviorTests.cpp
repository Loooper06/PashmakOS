#include "../../src/pet/PetBehavior.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void testEnergyBoundsAndRates()
{
    PetBehavior pet;
    pet.begin(0);
    pet.update(PetMode::Work, 120000UL);
    assert(pet.state().energy == 79);

    pet.update(PetMode::Nap, 150000UL);
    assert(pet.state().energy == 80);

    pet.update(PetMode::Play, 210000UL);
    assert(pet.state().energy == 81);

    pet.update(PetMode::Nap, 20UL * 60UL * 1000UL);
    assert(pet.state().energy <= 100);
    assert(pet.state().energy >= 0);
}

static void testMillisRollover()
{
    const uint32_t start = UINT32_MAX - 60000UL;
    PetBehavior pet;
    pet.begin(start);
    pet.update(PetMode::Work, start + 120000UL);
    assert(pet.state().energy == 79);

    pet.update(PetMode::Nap, start + 150000UL);
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
    const char *baseMessage = pet.state().message;

    pet.update(PetMode::Work, 90000UL);
    assert(strcmp(pet.state().message, baseMessage) != 0);

    pet.update(PetMode::Work, 97000UL);
    assert(strcmp(pet.state().message, baseMessage) == 0);
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
