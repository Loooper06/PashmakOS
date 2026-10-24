# Changelog

All notable changes to PashmakOS are documented here.

## [Unreleased]

### Added
- Show each pet's bond tier and progress directly in the dashboard pet picker.
- Add a per-pet energy progress bar and mood summary to the dashboard picker, alongside bond progression and accessible state details.

## [1.0.0] — 2026-10-24

First consolidated stable candidate after firmware integration, automated CI validation, and user-reported hardware smoke testing.

### Added
- Automatic work/play cycles and longer rest/nap periods after configured work sessions.
- Three pet profiles: Pashmak, Mochi, and Pixel.
- DS3231 RTC service and scheduled reminders, including Persian RTL text rendering.
- Local Wi-Fi dashboard for pet status, energy, mood, lifetime counters, activity, and daily/monthly history.
- Configurable daily focus-session goal with progress feedback, completion status, and browser-local persistence.
- Dashboard achievement badges for focus-session milestones, daily goal completion, and balanced activity.
- Interactive pet bonding: personality-specific TFT reactions, per-pet bond progression (0–100), dashboard bond meter, and Preferences persistence with an 8-second per-pet cooldown.
- Four visible relationship tiers tied to bond milestones; each pet responds in its own voice and the dashboard shows the next milestone.
- Configurable quiet hours in the dashboard, with persistent start/end hours and suppression of spontaneous pet messages and RTC reminders.
- Configurable RTC water and movement reminder intervals, including independent disable controls and persistent settings.
- Local focus insights summarizing the latest seven retained daily records: average focus time, most focused day, days with focus, and completed sessions.
- Daily focus-goal streak based on the configured session target and consecutive retained calendar days, calculated locally in the dashboard.
- One-click JSON export of the current statistics snapshot, browser-local daily goal, device settings, and export timestamp.
- CSV export and serial statistics/diagnostic commands.
- Preferences/NVS persistence for counters and bounded history.
- Native tests for behavior timing, calendar validation, statistics aggregation, and JSON serialization.
- GitHub Actions checks for the ESP32 PlatformIO build, native tests, dashboard JavaScript, and statistics JSON contract.
- Setup, troubleshooting, API, roadmap, and hardware smoke-test documentation.

### Reliability and security improvements
- Validate statistics payloads and date keys before dashboard rendering.
- Preserve the last valid dashboard snapshot on transient request failures; add bounded timeouts/retries.
- Escape JSON strings and protect CSV exports from spreadsheet formula injection.
- Check persistence writes and retry failed persistence on a later cycle.
- Add accessible labels and responsive dashboard behavior.

### Release notes
- Target hardware: ESP32-WROOM-32, ILI9341 320×240 TFT, DS3231 RTC.
- The default local access-point password is a development credential. Change it in `src/web/WebDashboard.cpp` before sharing or deploying the device.
- This release record reflects the user's report that the physical hardware smoke-test checklist passed. CI results independently confirm successful firmware compilation and automated host-side checks.
- Firmware build success does not replace validation on other board revisions, displays, RTC modules, or wiring configurations.

