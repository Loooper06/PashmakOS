# PashmakOS hardware smoke-test checklist

This checklist is intended for the ESP32-WROOM-32 + ILI9341 TFT + DS3231 RTC target. It complements CI; it does not replace native tests or prove behavior until the checks are performed on the physical device.

## 1. Before flashing

- [ ] Confirm the board, TFT, and RTC wiring against the current pin definitions; do not change pins based on this checklist.
- [ ] Build the current branch successfully with PlatformIO (`pio run -e esp32dev`) or use the firmware artifact from a successful CI build.
- [ ] Flash the firmware and open the serial monitor at **115200 baud**, line ending **Newline** or **Both NL & CR**.
- [ ] On first boot, confirm the serial output reports whether Preferences loaded, RTC availability, and the automatic behavior startup message.
- [ ] If the RTC is unavailable, note that daily/monthly history recording is expected to pause; do not treat that as a Wi-Fi or JSON failure.

## 2. Validate the serial statistics API

1. Send `help`; confirm the supported commands are listed.
2. Send `stats`; copy the content between `[STATS_JSON_BEGIN]` and `[STATS_JSON_END]` into a JSON validator.
3. Confirm the JSON parses and has `version: 1`, `activePetId` in `0..2`, three entries in `pets`, a matching selected `pet`, `totals`, `recent`, `dailyHistory`, and `monthlyHistory`.
4. Confirm every pet energy value is between 0 and 100, messages display correctly, and no quotes, backslashes, or line breaks break JSON.
5. Send `daily_csv` and `monthly_csv`; confirm each export has the matching begin/end markers and a header followed by records.
6. Repeat `stats` after switching pets in the dashboard; confirm `activePetId` and the selected `pet` agree.

**Pass criteria:** each JSON response parses, the schema is complete, selected-pet fields are consistent, and CSV markers/columns are intact.

## 3. Check persistence across a normal reboot

1. With a valid RTC date/time, leave the device running long enough for non-zero activity to accrue.
2. Record `totals` and the current daily/monthly record from `stats`.
3. Trigger a mode change if the UI/input path supports it; confirm the serial output shows persistence activity.
4. Reset the ESP32 normally, then wait for boot and RTC initialization.
5. Send `stats` again. Compare lifetime counters and retained daily/monthly history with the recorded values. A small amount of additional elapsed time or a newly started current-day/month record may be expected; historical records and previously persisted values must not disappear.
6. Switch to another pet, reboot, and confirm the selected pet ID is restored to a valid value. If selected-pet persistence is not expected in the current build, record the observed behavior instead of assuming it passed.
7. Do not use a factory erase between these steps; that intentionally clears NVS.

**Pass criteria:** previously committed lifetime counters and valid history survive reboot, selected pet remains valid, and malformed history is never exposed as invalid API data.

**Power-loss limitation:** a normal reset does not prove resilience to sudden power loss during an NVS write. Test power interruption only with a safe, controlled setup and repeat several times; document any lost interval or invalid-record recovery.

## 4. Wi-Fi dashboard and reconnect behavior

1. Connect a phone or computer to the device AP documented in `docs/STATISTICS_API.md`, then open `http://192.168.4.1/`.
2. Confirm the page loads, current pet selection matches serial `stats`, and totals/history/chart data appear.
3. Change the selected pet and confirm the visible pet details update without an inconsistent API snapshot.
4. Export daily and monthly CSV files; open them in a spreadsheet or text editor and verify headers, dates, and numeric values.
5. Temporarily disconnect the client from the AP, wait through at least one refresh timeout, reconnect, and reload or wait for refresh.
6. Confirm the UI indicates stale/failed updates while disconnected and recovers to fresh data after reconnection. The last valid snapshot should not be replaced with empty or malformed data.
7. Repeat with a browser on a narrow mobile viewport. Check for horizontal overflow, clipped controls, unreadable chart labels, and inaccessible buttons.

**Pass criteria:** dashboard loads reliably, exports remain valid, temporary client disconnects do not erase the last valid snapshot, and a reconnect restores live updates. For loop timing, compare idle vs. refresh/export windows; investigate repeatable `maxGap` values at or above 100 ms and any increase in `overBudget`. These are investigation thresholds, not automatic proof of a visible fault.

## 5. TFT, behavior, and RTC reminder checks

- [ ] Observe the TFT for at least 10 minutes with Wi-Fi enabled. Note flicker, partial redraws, stalls, or watchdog resets.
- [ ] Capture at least three paired `[TFT_FRAME]` and `[LOOP_TIMING]` serial reports while the dashboard is idle, refreshing normally, and exporting CSV. Record average/max draw duration plus average/max loop gap and each `overBudget` count.
- [x] User-reported physical observation: flicker is resolved both during normal display operation and while Wi-Fi/dashboard requests are active. Keep this separate from timing acceptance; a visually stable display does not by itself prove that the main loop never stalls.
- [ ] Confirm Work → Play → Work transitions occur at the configured durations. Allow enough time for the configured session duration; do not infer timing from a short observation.
- [ ] Confirm energy remains within 0–100 and changes at the documented mode-specific rates.
- [ ] Confirm Pashmak, Mochi, and Pixel have distinguishable message/personality profiles.
- [ ] Observe focused, curious, excited, tired, and exhausted moods on the TFT; confirm eyebrow/eye/mouth cues change without clipping or leaving drawing artifacts.
- [ ] Confirm spontaneous messages appear and expire, returning to the current mode's baseline message.
- [ ] With a valid RTC, observe time-aware messages/reminders at relevant hours and confirm they expire back to the current pet message.
- [ ] Check that display updates remain responsive while dashboard requests and CSV exports are being served.

Record the firmware commit, board, test date/time, RTC status, observed serial output, pass/fail per item, and any reproduction steps. Do not mark a physical check as passed based only on CI.

## 6. Test record template

| Field | Value |
|---|---|
| Firmware commit | |
| ESP32 board / TFT | |
| RTC present and time valid | |
| Serial JSON / CSV | Pass / Fail |
| Normal reboot persistence | Pass / Fail |
| Wi-Fi disconnect/reconnect | Pass / Fail |
| Mobile dashboard | Pass / Fail |
| TFT flicker / responsiveness | User reports Pass in normal + dashboard-active modes; repeatable timing measurements pending |
| Behavior transitions / energy | Pass / Fail |
| RTC reminders | Pass / Fail |
| Notes / logs | |


## Per-pet persistence check

1. Select each pet in turn and let its energy reach visibly different values; use the dashboard/API to record each value.
2. Leave the device powered for at least 60 seconds after the last energy change so the versioned `petProfile` Preferences blob can be saved.
3. Restart the ESP32 and confirm all three pet energy values are restored independently, and that the selected pet is restored as before.
4. Confirm `[PET_PROFILE] Loaded version 1 energy for all pets.` appears on a valid saved profile and `[PET_PROFILE] Saved energy for all pets.` appears after a save interval.
5. Confirm existing global work/play/nap totals and daily/monthly history remain unchanged by the profile-storage change.
6. If testing malformed stored data, do so only with a deliberate test fixture; verify invalid version/count/ID/energy causes all pet energies to fall back to 80 rather than partially applying a profile.

Profile writes are intentionally limited to one Preferences blob per minute. A sudden power loss may lose energy changes made since the most recent successful save; this is the trade-off used to bound NVS write frequency.
