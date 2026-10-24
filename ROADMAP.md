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

## Phase 1 — Data integrity and runtime resilience (in progress)
- [x] Shared JSON string escaping handles quotes, backslashes, common escapes, and remaining control bytes; native edge-case tests cover UTF-8 preservation and null input.
- [x] Persisted daily/monthly history rejects impossible calendar keys; native tests cover leap years, month lengths, and year/month bounds.
- [x] Dashboard validates the versioned response, counters, selected-pet consistency, and calendar keys before rendering.
- [x] Last valid dashboard snapshot survives request failures; requests time out and retries use bounded backoff.
- [x] Behavior timing uses rollover-safe elapsed/deadline arithmetic and a wide energy-time accumulator.
- [x] PlatformIO CI and native behavior tests are enabled; CI run `40049214618` passed firmware build, native behavior tests, dashboard JavaScript checks, and statistics JSON contract checks (commit `d824a49`).
- [x] Extract recent-day and monthly statistics aggregation into portable core helpers; native tests cover latest-N selection, full-range aggregation, empty/zero requests, missing months, and invalid month inputs.
- [x] Extract the JSON encoder into a portable serializer shared by firmware and native tests; compile a fixture, parse its emitted JSON, and validate the complete payload shape, wide counters, and escaped message contents.
- [ ] Complete a physical Wi-Fi disconnect/reconnect smoke test after the latest dashboard changes; procedure is documented in `docs/HARDWARE_SMOKE_TEST.md`.
Acceptance: valid API JSON for all message contents; intended energy rates; dashboard usable through brief disconnects; CI passes and hardware checks are recorded.

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

## Phase 5 — Dashboard quality and API contract (in progress)
- [x] Document `/api/stats` JSON schema and versioning.
- [x] Add stale-data status and bounded retry/backoff.
- [x] Validate incoming statistics defensively and preserve the last valid snapshot.
- [x] Quote CSV cells and neutralize formula-like values during export.
- [ ] Review mobile layout, accessible labels, empty states, and chart explanations on a phone.
- [ ] Verify pet selection and history exports on a small screen.
Acceptance: documented schema; failures do not erase useful data; dashboard readable on mobile.

## Phase 6 — User-aware features
1. Use RTC time-of-day to adapt message tone and reminder windows.
2. Add optional quiet hours only after defining safe configuration/storage.
3. Derive lightweight insights from local statistics; do not transmit personal data by default.
4. Keep behavior local-first and document what is stored in Preferences.
Acceptance: no cloud dependency or undisclosed telemetry; reminders respect quiet hours.

## Phase 7 — Testability and release readiness
1. Native host-side tests cover energy rates/bounds, mode changes, millis() rollover, distinct message profiles, spontaneous-message expiry, JSON escaping, and history calendar validation. Statistics aggregation and the shared statistics JSON serializer are covered by portable native tests; a physical-device serialization check remains part of the hardware smoke test.
2. [x] Add API contract checks and a documented serial/hardware smoke-test procedure (`docs/HARDWARE_SMOKE_TEST.md`).
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