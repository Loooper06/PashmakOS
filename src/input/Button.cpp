
#include "Button.h"
#include "Config.h"

Button::Button(uint8_t pin)
    : _pin(pin) {}

void Button::begin()
{
    pinMode(_pin, INPUT_PULLUP);
    _stableState = digitalRead(_pin);
    _lastReading = _stableState;
}

void Button::update(uint32_t now)
{
    _pressed = false;

    const bool reading = digitalRead(_pin);

    if (reading != _lastReading)
    {
        _lastReading = reading;
        _lastChange = now;
    }

    if ((uint32_t)(now - _lastChange) <
        Config::BUTTON_DEBOUNCE_MS)
    {
        return;
    }

    if (reading != _stableState)
    {
        _stableState = reading;

        if (_stableState == LOW)
        {
            _pressed = true;
        }
    }
}

bool Button::wasPressed() const
{
    return _pressed;
}
