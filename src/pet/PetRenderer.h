
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
        const char *message);

private:
    void drawDog(Display &display, PetMode mode, uint32_t frame);
    void drawHeader(Display &display);
    void drawFooter(Display &display, PetMode mode);
    void drawMessage(Display &display, const char *message);

    bool _initialized;
    bool _footerInitialized;
    const char *_lastMessage;
    bool _messageInitialized;
    PetMode _lastMode;
    bool _dogInitialized;
    PetMode _lastDogMode;
    int8_t _lastDogBob;
    bool _lastDogMoving;
    bool _lastDogBlinking;
    int8_t _lastDogLeftEarX;
    int8_t _lastDogRightEarX;
};
