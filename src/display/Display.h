
#pragma once

#include <TFT_eSPI.h>

class Display
{
public:
    Display();

    void begin();
    void clear(uint16_t color);

    void beginDogFrame(uint16_t background);
    void endDogFrame();

    void fillRect(
        int16_t x, int16_t y,
        int16_t w, int16_t h,
        uint16_t color);

    void fillCircle(
        int16_t x, int16_t y,
        int16_t r, uint16_t color);

    void fillTriangle(
        int16_t x0, int16_t y0,
        int16_t x1, int16_t y1,
        int16_t x2, int16_t y2,
        uint16_t color);

    void drawLine(
        int16_t x0, int16_t y0,
        int16_t x1, int16_t y1,
        uint16_t color);

    void drawText(
        const char *text,
        int16_t x, int16_t y,
        uint8_t size, uint16_t color);

    void fillEllipse(
        int16_t cx, int16_t cy,
        int16_t rx, int16_t ry,
        uint16_t color);

private:
    TFT_eSPI _tft;
    TFT_eSprite _dogSprite;

    bool _spriteReady;
    bool _drawingDog;

    static constexpr int16_t DOG_X = 70;
    static constexpr int16_t DOG_Y = 36;
    static constexpr int16_t DOG_W = 180;
    static constexpr int16_t DOG_H = 158;

    int16_t targetX(int16_t x) const;
    int16_t targetY(int16_t y) const;
};
