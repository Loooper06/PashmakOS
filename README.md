# PashmakOS

**Version:** 1.0.0 — see [CHANGELOG.md](CHANGELOG.md) for release notes.

A local-first ESP32 desk companion with automatic work/play/nap cycles, three pet personalities, an ILI9341 TFT interface, DS3231 RTC reminders, and a local Wi-Fi statistics dashboard.

## Current capabilities

- Automatic Work → Play → Work cycles, with a longer nap after the configured number of work sessions.
- Dashboard-configurable work, play, and long-rest durations plus the number of work sessions before a long rest; settings persist in ESP32 Preferences.
- Dashboard-configurable water and movement reminder intervals; either reminder can be disabled independently.
- Three pet profiles: Pashmak, Mochi, and Pixel, each with independent behavior state, energy, mood, and message.
- Pet bonding with four relationship stages (New companion, Familiar friend, Close companion, Best friend), personality-specific interaction reactions, and progress to the next milestone.
- Pet picker cards show each companion's current bond tier and 0–100 progress without switching profiles.
- Pet picker cards show each companion's current mood, an energy progress bar, and bond-tier progress, so all three profiles can be compared at a glance.
- Time-aware messages and RTC reminders with configurable water/movement intervals and quiet hours.
- Local dashboard with pet selection, energy/mood/message, lifetime counters, recent activity, daily/monthly history, and CSV export.
- Mobile-friendly dashboard layout with full-width pet cards, compact controls, single-column settings, and safe full-width status panels on narrow screens.
- Preferences/NVS-backed lifetime counters and bounded daily/monthly history.
- Serial diagnostics and `stats`, `daily_csv`, and `monthly_csv` commands.
- Native C++ tests and GitHub Actions checks for the ESP32 build, pet behavior, dashboard JavaScript, and statistics JSON contract.

## Hardware target

- ESP32-WROOM-32 development board
- ILI9341-compatible 320×240 TFT in landscape orientation
- DS3231 RTC module

See [hardware setup and troubleshooting](docs/SETUP.md) for the pin map, build/flash instructions, RTC setup, local dashboard access, and troubleshooting. Verify your exact module documentation before wiring.

## Build and test

Build firmware with PlatformIO:

```sh
pio run -e esp32dev
```

Upload to the connected board:

```sh
pio run -e esp32dev -t upload
```

Run native behavior tests:

```sh
sh tests/native/run.sh
```

Dashboard JavaScript and statistics JSON checks run in CI:

```sh
python3 tests/native/check_dashboard_js.py
python3 tests/native/check_stats_api.py
```

## Local dashboard and serial commands

The firmware creates a local Wi-Fi access point. The current development defaults are SSID `PashmakOS` and password `pashmak123`; connect to it and open `http://192.168.4.1/`. **Change the default AP password in `src/web/WebDashboard.cpp` before sharing or deploying the device.**

Open the serial monitor at **115200 baud**. Supported commands are documented in [Statistics API and serial commands](docs/STATISTICS_API.md). The `stats` command emits a JSON snapshot between `[STATS_JSON_BEGIN]` and `[STATS_JSON_END]`.

## Hardware validation and release status

The v1.0.0 release notes are recorded in [CHANGELOG.md](CHANGELOG.md), and the project version is tracked in [VERSION](VERSION). The project owner confirmed that the latest CI build and device test succeeded. The [hardware smoke-test checklist](docs/HARDWARE_SMOKE_TEST.md) remains the source of truth for recording individual scenarios and timing measurements; update it as those results are documented. CI alone cannot verify physical TFT behavior, RTC accuracy, Wi-Fi recovery, or persistence after power interruption.

See [ROADMAP.md](ROADMAP.md) for remaining enhancements and performance measurements. Change the default access-point password before sharing or deploying the device.

## Data and privacy

The dashboard is served locally by the ESP32. The documented architecture does not require a cloud service for normal operation. Lifetime statistics, selected pet, daily/monthly history, and some RTC reminder state are stored in ESP32 Preferences/NVS.
