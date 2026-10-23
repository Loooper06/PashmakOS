
#include "Display.h"
#include "Config.h"

Display::Display()
    : _dogSprite(&_tft),
      _spriteReady(false),
      _drawingDog(false)
{
}

void Display::begin()
{
    _tft.init();
    _tft.invertDisplay(true);
    _tft.setRotation(Config::TFT_ROTATION);

    Serial.println("\n===== TFT_eSPI Diagnostics =====");
    Serial.printf("TFT width: %d\n", _tft.width());
    Serial.printf("TFT height: %d\n", _tft.height());
    Serial.printf(
        "SPI configured: %d MHz\n",
        SPI_FREQUENCY / 1000000);

    _tft.fillScreen(0x1082);

    _dogSprite.setColorDepth(16);
    _spriteReady =
        (_dogSprite.createSprite(DOG_W, DOG_H) != nullptr);

    Serial.printf(
        "Sprite created: %s\n",
        _spriteReady ? "YES" : "NO");

    Serial.printf(
        "Sprite dimensions: %d x %d\n",
        DOG_W, DOG_H);

    Serial.println("================================");
}

int16_t Display::targetX(int16_t x) const
{
    return _drawingDog ? x + _dogOffsetX : x;
}

int16_t Display::targetY(int16_t y) const
{
    return _drawingDog ? y - DOG_Y : y;
}

void Display::setDogPosition(int16_t centerX, int16_t verticalOffset)
{
    _dogOffsetX = centerX - 160;
    if (_dogOffsetX < -68) _dogOffsetX = -68;
    if (_dogOffsetX > 68) _dogOffsetX = 68;
    _dogOffsetY = verticalOffset;
}

void Display::beginDogFrame(uint16_t background)
{
    _drawingDog = _spriteReady;

    if (_spriteReady)
    {
        _dogSprite.fillSprite(background);
    }
    else
    {
        // Fallback: draw directly to the display.
        _tft.fillRect(
            DOG_X, DOG_Y + _dogOffsetY,
            DOG_W, DOG_H,
            background);
    }
}

void Display::endDogFrame()
{
    // Place the reduced sprite at its actual screen position.
    if (_drawingDog)
    {
        // The canvas stays at one screen position. The character is moved
        // inside the canvas, so no old sprite area needs erasing and no
        // background flash occurs between frames.
        _dogSprite.pushSprite(DOG_X, DOG_Y + _dogOffsetY);
    }

    _drawingDog = false;
}

void Display::clear(uint16_t color)
{
    _tft.fillScreen(color);
}

void Display::fillRect(
    int16_t x, int16_t y,
    int16_t w, int16_t h,
    uint16_t color)
{
    if (_drawingDog)
    {
        _dogSprite.fillRect(
            targetX(x), targetY(y),
            w, h, color);
    }
    else
    {
        _tft.fillRect(x, y, w, h, color);
    }
}

void Display::fillCircle(
    int16_t x, int16_t y,
    int16_t r, uint16_t color)
{
    if (_drawingDog)
    {
        _dogSprite.fillCircle(
            targetX(x), targetY(y),
            r, color);
    }
    else
    {
        _tft.fillCircle(x, y, r, color);
    }
}

void Display::fillTriangle(
    int16_t x0, int16_t y0,
    int16_t x1, int16_t y1,
    int16_t x2, int16_t y2,
    uint16_t color)
{
    if (_drawingDog)
    {
        _dogSprite.fillTriangle(
            targetX(x0), targetY(y0),
            targetX(x1), targetY(y1),
            targetX(x2), targetY(y2),
            color);
    }
    else
    {
        _tft.fillTriangle(
            x0, y0,
            x1, y1,
            x2, y2,
            color);
    }
}

void Display::drawLine(
    int16_t x0, int16_t y0,
    int16_t x1, int16_t y1,
    uint16_t color)
{
    if (_drawingDog)
    {
        _dogSprite.drawLine(
            targetX(x0), targetY(y0),
            targetX(x1), targetY(y1),
            color);
    }
    else
    {
        _tft.drawLine(
            x0, y0, x1, y1, color);
    }
}

void Display::drawText(
    const char *text,
    int16_t x, int16_t y,
    uint8_t size, uint16_t color)
{
    if (text == nullptr)
        return;

    if (_drawingDog)
    {
        _dogSprite.setTextSize(size);
        _dogSprite.setTextColor(color);
        _dogSprite.setTextWrap(false);
        _dogSprite.setCursor(targetX(x), targetY(y));
        _dogSprite.print(text);
    }
    else
    {
        _tft.setTextSize(size);
        _tft.setTextColor(color);
        _tft.setTextWrap(false);
        _tft.setCursor(x, y);
        _tft.print(text);
    }
}

void Display::fillEllipse(
    int16_t cx, int16_t cy,
    int16_t rx, int16_t ry,
    uint16_t color)
{
    if (rx <= 0 || ry <= 0)
        return;

    const int32_t rx2 =
        static_cast<int32_t>(rx) * rx;

    const int32_t ry2 =
        static_cast<int32_t>(ry) * ry;

    for (int16_t dy = -ry; dy <= ry; ++dy)
    {
        const int32_t rem =
            ry2 - static_cast<int32_t>(dy) * dy;

        if (rem < 0)
            continue;

        const int32_t x2 =
            static_cast<int64_t>(rx2) * rem / ry2;

        int32_t lo = 0;
        int32_t hi = rx;
        int16_t dx = 0;

        while (lo <= hi)
        {
            const int32_t mid = (lo + hi) / 2;

            if (static_cast<int64_t>(mid) * mid <= x2)
            {
                dx = static_cast<int16_t>(mid);
                lo = mid + 1;
            }
            else
            {
                hi = mid - 1;
            }
        }

        const int16_t y = cy + dy;
        const int16_t x = cx - dx;
        const int16_t width = 2 * dx + 1;

        if (_drawingDog)
        {
            _dogSprite.drawFastHLine(
                targetX(x),
                targetY(y),
                width,
                color);
        }
        else
        {
            _tft.drawFastHLine(
                x, y, width, color);
        }
    }
}
