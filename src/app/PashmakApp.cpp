#include "PashmakApp.h"
#include "Config.h"

namespace
{
    // Automatic behavior schedule. These are intentionally easy to tune.
    constexpr uint32_t WORK_SESSION_MS = 25UL * 60UL * 1000UL;
    constexpr uint32_t PLAY_SESSION_MS = 2UL * 60UL * 1000UL;
    constexpr uint32_t NAP_SESSION_MS = 5UL * 60UL * 1000UL;
    constexpr uint8_t WORK_CYCLES_BEFORE_NAP = 3;
}

PashmakApp::PashmakApp() = default;

void PashmakApp::begin()
{
    Serial.begin(115200);
    _display.begin();

    const uint32_t now = millis();
    _lastWorkTick = now;
    _lastFrame = now;
    _modeStartedAt = now;

    _behavior.begin(now);

    _renderer.draw(
        _display,
        _mode,
        _frame,
        _behavior.state().message);

    Serial.println("PashmakOS: automatic pet behavior enabled");
}

void PashmakApp::setMode(PetMode mode, uint32_t now)
{
    if (_mode == mode)
        return;

    _mode = mode;
    _modeStartedAt = now;
    Serial.printf("Pet mode changed to: %u\n", static_cast<unsigned>(_mode));
}

void PashmakApp::updateAutomaticMode(uint32_t now)
{
    const uint32_t modeElapsed = static_cast<uint32_t>(now - _modeStartedAt);

    switch (_mode)
    {
    case PetMode::Work:
        if (modeElapsed >= WORK_SESSION_MS)
        {
            _completedWorkCycles++;

            if (_completedWorkCycles >= WORK_CYCLES_BEFORE_NAP)
            {
                _completedWorkCycles = 0;
                _longRestPending = true;
                setMode(PetMode::Nap, now);
            }
            else
            {
                setMode(PetMode::Play, now);
            }
        }
        break;

    case PetMode::Play:
        if (modeElapsed >= PLAY_SESSION_MS)
            setMode(PetMode::Work, now);
        break;

    case PetMode::Nap:
        if (modeElapsed >= NAP_SESSION_MS)
        {
            _longRestPending = false;
            setMode(PetMode::Work, now);
        }
        break;
    }
}

void PashmakApp::update()
{
    const uint32_t now = millis();

    // Account for elapsed work time without blocking the main loop.
    if (_mode == PetMode::Work)
        _workMs += static_cast<uint32_t>(now - _lastWorkTick);
    _lastWorkTick = now;

    updateAutomaticMode(now);
    _behavior.update(_mode, now);

    if (static_cast<uint32_t>(now - _lastFrame) >= Config::FRAME_INTERVAL_MS)
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
