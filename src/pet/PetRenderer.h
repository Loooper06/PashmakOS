
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
    int8_t _lastDogBob;
    bool _lastDogMoving;
    bool _lastDogBlinking;
    int8_t _lastDogLeftEarX;
    int8_t _lastDogRightEarX;
};
