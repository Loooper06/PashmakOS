# PashmakOS Development Roadmap

Repository: Loooper06/PashmakOS
Working branch: feature/auto-pet-behavior
Target: ESP32-WROOM-32 + ILI9341 TFT (320x240 landscape), DS3231 RTC

## Current baseline (2026-10-24)
- Automatic work/play/nap cycle and pet behavior engine.
- Three pet profiles (Pashmak, Mochi, Pixel) with independently updated behavior state and dashboard selection.
- Pet mood/message passed to the existing renderer; timed spontaneous pet messages.
- Local Wi-Fi dashboard with live status, energy, totals, daily/monthly history, activity trend, and CSV export.
- Refresh overlap protection and connection/last-update status.
- Preferences-backed totals/history, RTC-aware date/month tracking, serial diagnostics/export commands.
- Latest dashboard commit at roadmap creation: 6d74bd2bc7b184cc54eb22d8599ef7054d386365.

## Phase 1 — Data integrity and runtime resilience (next)
1. Correctly escape JSON strings (quotes, backslashes, control characters) for all pet messages.
2. Harden behavior timing against long update gaps and millis() rollover where applicable.
3. Make dashboard API handling defensive and preserve the last known snapshot during temporary failures.
4. Add concise serial diagnostics without flooding the serial port.
5. Build with PlatformIO CI and inspect JSON output; hardware verification remains separate.
Acceptance: valid API JSON for all message contents; intended energy rates; dashboard usable through brief disconnects; CI passes.

## Phase 2 — Behavior quality and pet personality
1. Give each pet a distinct message pool/personality profile without duplicating the behavior engine.
2. Add RTC time-aware, non-intrusive messages based on hour and current mode.
3. Prevent repeated messages and overlapping spontaneous events/reminders.
4. Improve/document energy and mood thresholds.
5. Add deterministic checks for timing, energy bounds, event expiry, and mode changes.
Acceptance: no blocking delays, energy stays 0–100, no event spam, stable timing over millis() rollover.

## Phase 3 — TFT experience without renderer regressions
1. Preserve the current renderer API/layout unless a task explicitly requires a fix.
2. Make pet changes and mood/message changes coherent using existing renderer inputs.
3. Verify RTC reminders expire and return to the current pet message.
4. Measure frame timing and check flicker with Wi-Fi enabled.
5. Keep display/network work non-blocking.
Acceptance: no new physical-display flicker/frame stalls; reminders and messages restore correctly.

## Phase 4 — Multi-pet data model and persistence
1. Persist per-pet energy/personality state with versioned storage and safe defaults.
2. Validate selected pet IDs on load and API requests.
3. Decide/document whether lifetime totals are global or per-pet; do not silently change existing totals.
4. Add migration/version handling before changing Preferences layouts.
5. Bound NVS writes and verify restart/power-loss behavior.
Acceptance: intended state survives restart; invalid data falls back safely; no excessive flash writes.

## Phase 5 — Dashboard quality and API contract
1. Document /api/stats JSON schema and versioning.
2. Improve mobile layout, accessible labels, empty states, and chart explanations.
3. Add stale-data indication and bounded retry/backoff.
4. Harden CSV export for missing values and spreadsheet compatibility.
5. Keep pet selection/history responsive on small screens.
Acceptance: documented schema; failures do not erase useful data; dashboard readable on mobile.

## Phase 6 — User-aware features
1. Use RTC time-of-day to adapt message tone and reminder windows.
2. Add optional quiet hours only after defining safe configuration/storage.
3. Derive lightweight insights from local statistics; do not transmit personal data by default.
4. Keep behavior local-first and document what is stored in Preferences.
Acceptance: no cloud dependency or undisclosed telemetry; reminders respect quiet hours.

## Phase 7 — Testability and release readiness
1. Native host-side tests now cover energy rates/bounds, mode changes, millis() rollover, distinct message profiles, and spontaneous-message expiry. JSON serialization and statistics aggregation tests remain to be added.
2. Add API contract checks and a documented serial smoke-test procedure.
3. Document wiring, pin conflicts, first boot, AP credentials, RTC setup, and troubleshooting.
4. Record measured RAM/flash usage and TFT frame timing.
5. Create a release checklist; tag only after CI and physical-device checks.
Acceptance: repeatable tests, passing CI, clear setup docs, no unverified claims about device behavior.

## Iteration protocol
- Work from user-provided test results and repository state; no button-click gating and no unrequested scheduled repository writes.
- Bundle multiple related improvements in each implementation step.
- After each commit, inspect source/diff, check CI, and list remaining physical ESP32 tests.
- Do not casually change the renderer or pin assignments.
- Update this roadmap only when a phase completes or priorities materially change.