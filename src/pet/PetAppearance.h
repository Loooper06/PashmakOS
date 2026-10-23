#pragma once
#include <stdint.h>
class Display;

enum class PetBodyPose : uint8_t { Stand, Walk, Sit, LieDown, Hop };

void drawPashmakBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream);
void drawMochiBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream);
void drawPixelBody(Display &d, PetBodyPose pose, int8_t legSwing, int8_t tailSwing, uint16_t fur, uint16_t furDark, uint16_t furLight, uint16_t cream);
