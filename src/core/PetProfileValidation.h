#pragma once

#include <stddef.h>
#include <stdint.h>

namespace PetProfileValidation
{
template <typename Profile, size_t PetCount>
bool isValid(const Profile &stored, uint32_t expectedVersion)
{
    if (stored.version != expectedVersion || stored.count != PetCount)
        return false;

    for (size_t i = 0; i < PetCount; ++i)
    {
        if (stored.petIds[i] != i || stored.energy[i] > 100)
            return false;
    }

    return true;
}
} // namespace PetProfileValidation
