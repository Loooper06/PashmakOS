
#pragma once

#include <Arduino.h>

class Button
{
public:
    explicit Button(uint8_t pin);

    void begin();
    void update(uint32_t now);

    bool wasPressed() const;

private:
    uint8_t _pin;
    bool _stableState = HIGH;
    bool _lastReading = HIGH;
    bool _pressed = false;
    uint32_t _lastChange = 0;
};
