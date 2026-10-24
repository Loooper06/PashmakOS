#pragma once

#include <stdint.h>
#include "PetBehavior.h"

// Resolves visual effects to one active overlay. Base facial features remain
// mood-driven; only transient overlays compete for the effect layer.
enum class PetVisualEffect : uint8_t
{
    None,
    Attention,
    SleepCue,
    Happy,
    Curious,
    Sad
};

struct PetEffectFrame
{
    PetVisualEffect effect;
    uint8_t phase;
};

class PetEffectController
{
public:
    PetEffectFrame resolve(PetMood mood, bool attentionActive, uint32_t frame, uint8_t attentionPhase) const
    {
        if (attentionActive)
            return {PetVisualEffect::Attention, attentionPhase};

        if (mood == PetMood::Exhausted)
            return {PetVisualEffect::Sad, static_cast<uint8_t>((frame / 6U) % 4U)};

        if (mood == PetMood::Sleepy || mood == PetMood::Tired)
            return {PetVisualEffect::SleepCue, static_cast<uint8_t>((frame / 8U) % 4U)};

        if (mood == PetMood::Happy || mood == PetMood::Excited)
            return {PetVisualEffect::Happy, static_cast<uint8_t>((frame / 6U) % 4U)};

        if (mood == PetMood::Curious)
            return {PetVisualEffect::Curious, static_cast<uint8_t>((frame / 6U) % 4U)};

        return {PetVisualEffect::None, 0};
    }
};
