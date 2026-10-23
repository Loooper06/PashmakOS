#include <cstring>
#include "PetRenderer.h"
#include "../display/Display.h"

namespace
{
    constexpr int16_t SCREEN_W = 320;
    constexpr int16_t SCREEN_H = 240;

    constexpr int16_t HEADER_H = 36;
    constexpr int16_t FOOTER_Y = 194;
    constexpr int16_t FOOTER_H = SCREEN_H - FOOTER_Y;

    constexpr uint16_t BG = 0x1082;
    constexpr uint16_t PANEL = 0x2127;
    constexpr uint16_t WHITE = 0xFFFF;
    constexpr uint16_t PURPLE = 0xB81F;
    constexpr uint16_t PINK = 0xF2B5;

    constexpr uint16_t SHADOW = 0x0861;
    constexpr uint16_t FUR = 0xFBAA;
    constexpr uint16_t FUR_LIGHT = 0xFE0F;
    constexpr uint16_t FUR_DARK = 0xD4A4;
    constexpr uint16_t CREAM = 0xFF1A;
    constexpr uint16_t CREAM_DARK = 0xF6B7;
    constexpr uint16_t EAR_PINK = 0xF2B5;
    constexpr uint16_t BLUSH = 0xF1B7;
    constexpr uint16_t EYE = 0x18C3;
    constexpr uint16_t NOSE = 0x3928;
    constexpr uint16_t MOUTH = 0x5129;
    constexpr uint16_t TONGUE = 0xE8B7;

    struct DogPose
    {
        int8_t bob;
        int8_t leftEarX;
        int8_t rightEarX;
        bool blinking;
        bool moving;
    };

    DogPose getDogPose(PetMode mode, uint32_t frame)
    {
        DogPose pose{};

        const bool sleeping = mode == PetMode::Nap;
        const uint32_t blinkPhase = frame % 43;

        pose.blinking = sleeping ||
                        blinkPhase < 2 ||
                        (blinkPhase >= 4 && blinkPhase < 6);

        if (sleeping)
        {
            pose.bob = 2;
        }
        else if (mode == PetMode::Play)
        {
            pose.bob = (frame % 4 < 2) ? -2 : 2;
        }
        else
        {
            pose.bob = (frame % 6 < 3) ? 0 : 1;
        }

        const uint8_t earPhase = (frame / 5) % 12;

        if (!sleeping)
        {
            if (earPhase >= 2 && earPhase < 5)
                pose.leftEarX = -2;

            if (earPhase >= 7 && earPhase < 10)
                pose.rightEarX = 2;
        }

        pose.moving = frame % 8 < 4;

        return pose;
    }
}

PetRenderer::PetRenderer()
    : _initialized(false),
      _footerInitialized(false),
      _lastMode(PetMode::Work),
      _dogInitialized(false),
      _lastDogMode(PetMode::Work),
      _lastDogBob(0),
      _lastDogLeftEarX(0),
      _lastDogRightEarX(0),
      _lastDogMoving(false),
      _lastDogBlinking(false),
      _lastMessage(nullptr),
      _messageInitialized(false)
{
}

void PetRenderer::drawMessage(Display &d, const char *message)
{
    if (message == nullptr)
        return;

    if (_messageInitialized &&
        _lastMessage != nullptr &&
        strcmp(_lastMessage, message) == 0)
    {
        return;
    }

    // Keep the message inside the header's right-hand area.
    // The built-in font is 6 px wide at text size 1, so this
    // area safely fits short messages without covering the title.
    constexpr int16_t MESSAGE_X = 145;
    constexpr int16_t MESSAGE_Y = 5;
    constexpr int16_t MESSAGE_W = 140;
    constexpr int16_t MESSAGE_H = 27;

    d.fillRect(MESSAGE_X, MESSAGE_Y, MESSAGE_W, MESSAGE_H, PANEL);
    d.drawText(message, MESSAGE_X + 2, 13, 1, WHITE);

    _lastMessage = message;
    _messageInitialized = true;
}

void PetRenderer::drawHeader(Display &d)
{
    d.fillRect(0, 0, SCREEN_W, HEADER_H, PANEL);

    d.drawText("PashmakOS", 12, 10, 2, PURPLE);

    d.fillCircle(294, 18, 8, PINK);
    d.fillCircle(291, 15, 2, WHITE);
}

void PetRenderer::drawDog(Display &d, PetMode mode, uint32_t frame)
{
    const bool sleeping = mode == PetMode::Nap;
    const bool playing = mode == PetMode::Play;

    const DogPose pose = getDogPose(mode, frame);

    const int16_t cx = 160;
    const int16_t cy = 112 + pose.bob;

    const int16_t lx = pose.leftEarX;
    const int16_t rx = pose.rightEarX;

    d.beginDogFrame(BG);

    d.fillEllipse(cx, 180, 48, 7, SHADOW);

    d.fillEllipse(119 + lx, 80, 20, 27, FUR_DARK);
    d.fillTriangle(101 + lx, 84, 105 + lx, 57, 130 + lx, 65, FUR_DARK);
    d.fillEllipse(201 + rx, 80, 20, 27, FUR_DARK);
    d.fillTriangle(190 + rx, 65, 215 + rx, 57, 219 + rx, 84, FUR_DARK);

    d.fillEllipse(119 + lx, 80, 17, 24, FUR);
    d.fillTriangle(104 + lx, 81, 108 + lx, 61, 128 + lx, 68, FUR);
    d.fillEllipse(201 + rx, 80, 17, 24, FUR);
    d.fillTriangle(192 + rx, 68, 212 + rx, 61, 216 + rx, 81, FUR);

    d.fillEllipse(119 + lx, 79, 11, 16, EAR_PINK);
    d.fillTriangle(110 + lx, 78, 112 + lx, 65, 124 + lx, 70, EAR_PINK);
    d.fillEllipse(201 + rx, 79, 11, 16, EAR_PINK);
    d.fillTriangle(196 + rx, 70, 208 + rx, 65, 210 + rx, 78, EAR_PINK);

    d.fillEllipse(116 + lx, 73, 3, 5, CREAM_DARK);
    d.fillEllipse(198 + rx, 73, 3, 5, CREAM_DARK);
    d.fillEllipse(123 + lx, 96, 13, 12, FUR);
    d.fillEllipse(197 + rx, 96, 13, 12, FUR);

    d.fillEllipse(cx, cy, 43, 43, FUR_DARK);
    d.fillEllipse(cx, cy - 2, 41, 41, FUR);
    d.fillEllipse(cx, cy - 27, 7, 15, CREAM);
    d.fillEllipse(cx, cy - 15, 5, 13, CREAM);
    d.fillEllipse(cx - 23, cy + 15, 22, 19, CREAM);
    d.fillEllipse(cx + 23, cy + 15, 22, 19, CREAM);
    d.fillEllipse(cx, cy + 27, 25, 13, CREAM);

    d.fillTriangle(cx - 12, cy - 37, cx - 5, cy - 29, cx - 17, cy - 27, FUR_LIGHT);
    d.fillTriangle(cx + 3, cy - 38, cx + 12, cy - 29, cx + 1, cy - 27, FUR_LIGHT);
    d.fillEllipse(cx - 16, cy - 13, 7, 2, FUR_DARK);
    d.fillEllipse(cx + 16, cy - 13, 7, 2, FUR_DARK);

    if (pose.blinking)
    {
        d.drawLine(cx - 23, cy - 3, cx - 17, cy + 1, EYE);
        d.drawLine(cx - 17, cy + 1, cx - 11, cy - 3, EYE);
        d.drawLine(cx + 11, cy - 3, cx + 17, cy + 1, EYE);
        d.drawLine(cx + 17, cy + 1, cx + 23, cy - 3, EYE);
    }
    else
    {
        d.fillEllipse(cx - 17, cy - 3, 10, 13, FUR_DARK);
        d.fillEllipse(cx + 17, cy - 3, 10, 13, FUR_DARK);
        d.fillEllipse(cx - 17, cy - 4, 8, 11, EYE);
        d.fillEllipse(cx + 17, cy - 4, 8, 11, EYE);
        d.fillEllipse(cx - 17, cy - 3, 5, 8, MOUTH);
        d.fillEllipse(cx + 17, cy - 3, 5, 8, MOUTH);
        d.fillCircle(cx - 19, cy - 8, 3, WHITE);
        d.fillCircle(cx + 15, cy - 8, 3, WHITE);
        d.fillCircle(cx - 14, cy - 1, 1, WHITE);
        d.fillCircle(cx + 20, cy - 1, 1, WHITE);
    }

    d.fillEllipse(cx - 29, cy + 13, 7, 4, BLUSH);
    d.fillEllipse(cx + 29, cy + 13, 7, 4, BLUSH);
    d.fillCircle(cx - 31, cy + 11, 1, CREAM);
    d.fillCircle(cx + 27, cy + 11, 1, CREAM);
    d.fillEllipse(cx - 9, cy + 17, 13, 10, CREAM);
    d.fillEllipse(cx + 9, cy + 17, 13, 10, CREAM);
    d.fillTriangle(cx - 5, cy + 10, cx + 5, cy + 10, cx, cy + 16, NOSE);
    d.fillCircle(cx, cy + 11, 3, NOSE);
    d.fillCircle(cx - 1, cy + 10, 1, WHITE);

    if (playing)
    {
        d.drawLine(cx, cy + 15, cx, cy + 21, MOUTH);
        d.fillEllipse(cx, cy + 25, 8, 8, MOUTH);
        d.fillEllipse(cx, cy + 27, 5, 5, TONGUE);
        d.drawLine(cx, cy + 23, cx, cy + 27, MOUTH);
        d.fillCircle(111, 113, 2, CREAM);
        d.fillCircle(209, 113, 2, CREAM);
        d.drawLine(112, 103, 112, 109, WHITE);
        d.drawLine(109, 106, 115, 106, WHITE);
        d.drawLine(208, 103, 208, 109, WHITE);
        d.drawLine(205, 106, 211, 106, WHITE);
    }
    else
    {
        d.drawLine(cx, cy + 16, cx, cy + 21, MOUTH);
        d.drawLine(cx, cy + 21, cx - 5, cy + 18, MOUTH);
        d.drawLine(cx - 5, cy + 18, cx - 8, cy + 18, MOUTH);
        d.drawLine(cx, cy + 21, cx + 5, cy + 18, MOUTH);
        d.drawLine(cx + 5, cy + 18, cx + 8, cy + 18, MOUTH);
    }

    if (sleeping)
    {
        d.drawText("Z", 207, 65, 2, WHITE);
        d.drawText("z", 224, 51, 1, WHITE);
    }

    d.endDogFrame();
}

void PetRenderer::drawFooter(Display &d, PetMode mode)
{
    d.fillRect(0, FOOTER_Y, SCREEN_W, FOOTER_H, PANEL);

    switch (mode)
    {
    case PetMode::Work:
        d.drawText("FOCUS TIME", 10, 201, 2, WHITE);
        break;
    case PetMode::Nap:
        d.drawText("REST TIME", 10, 201, 2, WHITE);
        break;
    case PetMode::Play:
        d.drawText("MOVE A BIT", 10, 201, 2, WHITE);
        break;
    }

    d.drawText("Automatic behavior", 158, 210, 1, WHITE);
    d.drawText("PashmakOS v0.1", 158, 224, 1, WHITE);
}

void PetRenderer::draw(Display &d, PetMode mode, uint32_t frame, const char *message)
{
    if (!_initialized)
    {
        d.clear(BG);
        drawHeader(d);
        _initialized = true;
    }

    if (!_footerInitialized || mode != _lastMode)
    {
        drawFooter(d, mode);
        _lastMode = mode;
        _footerInitialized = true;
    }

    drawMessage(d, message);

    const DogPose pose = getDogPose(mode, frame);
    const bool dogChanged =
        !_dogInitialized ||
        mode != _lastDogMode ||
        pose.bob != _lastDogBob ||
        pose.moving != _lastDogMoving ||
        pose.blinking != _lastDogBlinking ||
        pose.leftEarX != _lastDogLeftEarX ||
        pose.rightEarX != _lastDogRightEarX;

    if (!dogChanged)
        return;

    drawDog(d, mode, frame);

    _lastDogMode = mode;
    _lastDogBob = pose.bob;
    _lastDogMoving = pose.moving;
    _lastDogBlinking = pose.blinking;
    _lastDogLeftEarX = pose.leftEarX;
    _lastDogRightEarX = pose.rightEarX;
    _dogInitialized = true;
}
