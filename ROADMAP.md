# PashmakOS Development Roadmap

Repository: Loooper06/PashmakOS
Integration base: `main`; implement changes on short-lived feature branches and merge through pull requests.
Target: ESP32-WROOM-32 + ILI9341 TFT (320x240 landscape), DS3231 RTC

## Current baseline (2026-10-24)
- Automatic work/play/nap cycle and pet behavior engine.
- Three pet profiles (Pashmak, Mochi, Pixel) with independently updated behavior state and dashboard selection.
- Dashboard pet interaction with personality-specific TFT reactions, persistent per-pet bond levels, and cooldown protection.
- Pet mood/message passed to the existing renderer; timed spontaneous pet messages.
- Local Wi-Fi dashboard with live status, energy, totals, daily/monthly history, activity trend, and CSV export.
- Refresh overlap protection and connection/last-update status.
- Preferences-backed totals/history, RTC-aware date/month tracking, serial diagnostics/export commands.
- Latest integrated dashboard baseline: `a3145ec7d5e10844a0edb2a83375a577e0b8308a` (mobile layout and responsive CSS regression-check fix).

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
- [x] Add CI regression checks for the dashboard HTTP/API contract and client-side failure recovery.
- [ ] Measure full-loop cadence under dashboard idle/request/export load using `[LOOP_TIMING]` reports; frame draw duration alone does not capture time spent serving network requests.
Acceptance: valid API JSON for all message contents; intended energy rates; dashboard usable through brief disconnects; CI passes and hardware checks are recorded.

## Phase 2 — Behavior quality and pet personality

Progress update (2026-10-25): time-aware messages now respect quiet hours and Nap, and morning messages differ between Work and Play. Native regression coverage protects these rules. Full phase acceptance is still pending.
1. Give each pet a distinct message pool/personality profile without duplicating the behavior engine.
2. Add RTC time-aware, non-intrusive messages based on hour and current mode.
3. [x] Prevent the same RTC time-aware base message from repeating on consecutive message intervals; add a native regression test. Spontaneous events are also deferred during Nap, with native coverage. Event/reminder overlap still needs a device-level review.
4. Improve/document energy and mood thresholds.
5. [x] Add deterministic native regression checks for timing, energy bounds, event expiry, and mode changes, including restoration of the latest baseline message after a temporary event.
Acceptance: no blocking delays, energy stays 0–100, no event spam, stable timing over millis() rollover.

## Phase 3 — TFT experience without renderer regressions
Progress update (2026-10-24): mood-specific facial expressions are implemented using existing `PetMood` input; the renderer API and layout are unchanged. Physical TFT timing/flicker validation remains pending.
1. Preserve the current renderer API/layout unless a task explicitly requires a fix.
2. [x] Make mood changes visible through distinct focused, curious, sleepy/tired, excited, and exhausted facial cues using existing renderer inputs.
3. Verify RTC reminders expire and return to the current pet message.
4. [x] Add serial frame-duration diagnostics (10-second average/maximum and over-budget count); [x] add loop-cadence diagnostics to reveal delays between `app.update()` calls while the dashboard is being serviced. Physical flicker check is user-confirmed resolved both with Wi-Fi idle and while serving dashboard requests; collect loop timing measurements under load next.
5. Keep display/network work non-blocking.
Acceptance: no new physical-display flicker/frame stalls; reminders and messages restore correctly.

## Phase 4 — Multi-pet data model and persistence (in progress)
- [x] Persist each pet's energy in a versioned Preferences blob; validate schema version, record count, stable pet IDs, and energy range, with safe defaults on invalid data.
- [x] Bound profile persistence to one blob write per minute, independent of the number of pets.
- [ ] Verify per-pet energy survives reboot and invalid profile data safely falls back on physical hardware.
- [x] Add source-level regression coverage that an invalid persisted active-pet ID falls back to pet 0 before renderer access; API selection range checks are also covered in CI.
- [ ] Verify selected-pet persistence across reboot and invalid stored ID recovery on physical hardware.
- Decide/document whether lifetime totals are global or per-pet; current lifetime and history counters remain global and are intentionally unchanged.
- Add migration/version handling before changing Preferences layouts; profile storage starts at version 1 and existing statistics/history keys are untouched.
Acceptance: intended state survives restart; invalid data falls back safely; no excessive flash writes.

## Phase 5 — Dashboard quality and API contract (in progress)
- [x] Document `/api/stats` JSON schema and versioning.
- [x] Add stale-data status and bounded retry/backoff.
- [x] Validate incoming statistics defensively and preserve the last valid snapshot.
- [x] Quote CSV cells and neutralize formula-like values during export.
- [x] Source review confirms responsive viewport/grid behavior, horizontally scrollable history tables, empty-state messages, and accessible pet-picker labels; activity trend and each duration bar now have assistive-technology labels.
- [x] Add native source-level regression checks for HTTP status codes, strict pet-ID validation, selection acknowledgement, request timeout, stale snapshot retention, bounded retries, and refresh overlap protection; run them in CI.
- [ ] Review mobile layout, accessible labels, empty states, and chart explanations on a phone.
- [ ] Verify pet selection and history exports on a small screen.
Acceptance: documented schema; failures do not erase useful data; dashboard readable on mobile.

## Phase 6 — User-aware features
1. Use RTC time-of-day to adapt message tone and reminder windows.
2. [x] Add configurable quiet hours with a versioned Preferences blob, dashboard controls, midnight-crossing ranges, and suppression of spontaneous messages/RTC reminders.
3. [x] Derive lightweight focus insights from the latest seven retained daily records locally; do not transmit personal data.
   - Shows average daily focus, strongest focus day, days with recorded work, and completed sessions.
   - Values are derived from the existing statistics API; no new persistence fields or API schema changes.
   - [x] Show a daily-goal streak using the browser's configured session target and consecutive recorded calendar days (history is limited to 14 days).
4. [x] Keep behavior local-first; reminder schedules and quiet hours persist in versioned Preferences blobs with no cloud dependency.
5. Continue measuring reminder/event overlap and full-loop cadence under dashboard load.
Acceptance: no cloud dependency or undisclosed telemetry; reminders respect quiet hours.

## Phase 7 — Testability and release readiness
1. Native host-side tests cover energy rates/bounds, mode changes, millis() rollover, distinct message profiles, spontaneous-message expiry, JSON escaping, and history calendar validation. Statistics aggregation and the shared statistics JSON serializer are covered by portable native tests; a physical-device serialization check remains part of the hardware smoke test.
2. [x] Add API contract checks and a documented serial/hardware smoke-test procedure (`docs/HARDWARE_SMOKE_TEST.md`).
3. [x] Document wiring, pin conflicts, first boot, AP credentials, RTC setup, and troubleshooting in `docs/SETUP.md`.
4. Record measured RAM/flash usage, TFT frame timing, and loop-cadence measurements under dashboard load.
5. Create a release checklist; tag only after CI and physical-device checks.
Acceptance: repeatable tests, passing CI, clear setup docs, no unverified claims about device behavior.

## Iteration protocol
- Work from user-provided test results and repository state; no button-click gating and no unrequested scheduled repository writes.
- Bundle multiple related improvements in each implementation step.
- After each commit, inspect source/diff, check CI, and list remaining physical ESP32 tests.
- Do not casually change the renderer or pin assignments.
- Update this roadmap only when a phase completes or priorities materially change.

---

## Roadmap for the next product features

This sequence supplements the existing engineering roadmap above. Each feature phase gets its own branch and pull request. A phase is marked complete only after both CI jobs pass, the user confirms the physical ESP32 test, and the PR is merged.

| Feature phase | Scope | Status |
|---:|---|---|
| 1 | Distinct personalities: voice plus per-pet energy dynamics | Completed (PR #43) |
| 2 | Direct interaction with the pet | In progress (PR pending) |
| 3 | Pet needs: energy, happiness, and bond model | Planned |
| 4 | Achievements and persistent progression | Planned |
| 5 | Complete Pomodoro flow and work/rest cycle controls | Planned |
| 6 | Persistent settings through the dashboard | Planned |
| 7 | Activity statistics and history improvements | Planned |
| 8 | Day/night behavior using the RTC | Planned |
| 9 | Dashboard health and network recovery improvements | Planned |
| 10 | Customizable themes and accessories | Planned |
| 11 | Stability and recovery regression suite | Planned |

### Feature phase 1 — acceptance criteria

- [x] Pashmak, Mochi, and Pixel differ in energy consumption and recovery, not only in their messages.
- [x] Energy remains within 0–100 for long delays and all supported modes.
- [x] Mode switches and millis() rollover remain safe.
- [x] Native behavior/dashboard CI and ESP32 firmware build both pass.
- [x] User confirms the physical ESP32/TFT behavior.
- [x] Phase completed and PR #43 merged.

### Feature phase 2 — direct interaction with the pet

- [x] Dashboard offers distinct affection, playful, and comfort actions.
- [x] Each action produces a personality-specific message and mood reaction on the TFT.
- [x] Interaction actions use the existing per-pet cooldown and bond progression.
- [x] Unsupported or missing action values are rejected by the HTTP API.
- [x] Native tests cover typed reactions and interaction expiry; dashboard contract tests cover action validation and button wiring.
- [ ] Both CI jobs pass and the user confirms the physical ESP32/TFT behavior.
- [ ] Merge the implementation PR before marking this phase complete.
