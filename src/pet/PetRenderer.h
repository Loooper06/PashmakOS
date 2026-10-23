
#pragma once

#include <stdint.h>
#include "../core/PetState.h"

class Display;

class PetRenderer
{
public:
    PetRenderer();

    void draw(
        Display &display,
        PetMode mode,
        uint32_t frame,
        uint8_t petId,
        const char *message,
        const char *dateTime);

private:
    void drawDog(Display &display, PetMode mode, uint32_t frame, uint8_t petId);
    void drawHeader(Display &display);
    void drawFooter(Display &display, PetMode mode, uint8_t petId, const char *dateTime);
    void drawMessage(Display &display, const char *message);

    bool _initialized;
    bool _footerInitialized;
    const char *_lastMessage;
    bool _messageInitialized;
    char _lastDateTime[24] = "";
    PetMode _lastMode;
    bool _dogInitialized;
    PetMode _lastDogMode;
    uint8_t _lastPetId = 255;
    uint8_t _lastDogPetId = 255;
    int8_t _lastDogBob;
    int16_t _lastDogX = 160;
    int8_t _lastDogGazeX = 0;
    bool _lastDogMoving;
    int8_t _lastDogLegSwing = 0;
    int8_t _lastDogTailSwing = 0;
    uint8_t _lastDogAction = 255;
    bool _lastDogBlinking;
    int8_t _lastDogLeftEarX;
    int8_t _lastDogRightEarX;
};
