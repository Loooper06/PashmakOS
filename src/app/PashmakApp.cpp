
#include "PashmakApp.h"
#include "Config.h"

PashmakApp::PashmakApp()
    : _modeButton(Config::BTN_MODE),
      _napButton(Config::BTN_NAP),
      _playButton(Config::BTN_PLAY) {}

void PashmakApp::begin()
{
    Serial.begin(115200);

    _modeButton.begin();
    _napButton.begin();
    _playButton.begin();

    _display.begin();

    const uint32_t now = millis();

    _lastWorkTick = now;
    _lastFrame = now;

    _renderer.draw(
        _display,
        _mode,
        _frame,
        _behavior.state().message);

    Serial.println("PashmakOS v0.1 booted!");
}

void PashmakApp::setMode(PetMode mode)
{
    _mode = mode;
}

void PashmakApp::update()
{
    const uint32_t now = millis();

    _behavior.begin(now);

    _modeButton.update(now);
    _napButton.update(now);
    _playButton.update(now);
    _behavior.update(_mode, now);

    const PetBehaviorState &behavior = _behavior.state();

    if (_modeButton.wasPressed())
    {
        if (_mode == PetMode::Work)
        {
            setMode(PetMode::Play);
        }
        else
        {
            setMode(PetMode::Work);
        }
    }

    if (_napButton.wasPressed())
    {
        setMode(
            _mode == PetMode::Nap
                ? PetMode::Work
                : PetMode::Nap);
    }

    if (_playButton.wasPressed())
    {
        setMode(
            _mode == PetMode::Play
                ? PetMode::Work
                : PetMode::Play);
    }

    if (_mode == PetMode::Work)
    {
        _workMs += (uint32_t)(now - _lastWorkTick);
    }

    _lastWorkTick = now;

    if ((uint32_t)(now - _lastFrame) >=
        Config::FRAME_INTERVAL_MS)
    {

        ++_frame;

        _renderer.draw(
            _display,
            _mode,
            _frame,
            _behavior.state().message);

        _lastFrame = now;
    }
}
