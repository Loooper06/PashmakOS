# Statistics JSON and Serial Commands

## Serial configuration

- Baud rate: `115200`
- Line ending: `Newline` or `Both NL & CR`
- Commands are parsed incrementally; receiving a command does not wait for a blocking line read.

## Commands

| Command | Behavior |
|---|---|
| `help` | Prints the supported commands |
| `stats` | Prints a JSON snapshot between `[STATS_JSON_BEGIN]` and `[STATS_JSON_END]` |
| `daily_csv` | Prints retained daily history as CSV between `[DAILY_CSV_BEGIN]` and `[DAILY_CSV_END]` |
| `monthly_csv` | Prints retained monthly history as CSV between `[MONTHLY_CSV_BEGIN]` and `[MONTHLY_CSV_END]` |

Unknown commands are ignored with a diagnostic message. Commands longer than the fixed input buffer are rejected.

## JSON schema

The `stats` response is a single JSON object with `version: 3`. The dashboard validates the payload before rendering it; an invalid response is treated as a failed refresh, and the last valid snapshot remains visible.

- `activePetId`: selected pet ID (0–2).
- `pets`: exactly three pet profiles. Each profile has `id`, `name`, `energy` (0–100), `happiness` (0–100), `bond` (0–100), `interactions` (0–65535), `achievementMask` (known per-pet achievement bits), `mood`, and `message`.
- `pet`: the selected pet's `energy`, `happiness`, `bond`, `interactions`, `achievementMask`, `mood`, and `message`. These values duplicate the selected profile for convenient dashboard rendering.
- `globalAchievementMask`: persisted global achievement bits for focus-session milestones and a three-consecutive-day focus streak.
- `totals`: lifetime counters currently stored in Preferences.
- `recent`: aggregate of the most recent available daily records, up to the requested number of days (the serial command currently requests 7; the dashboard requests 14).
- `dailyHistory`: retained daily records, each with `dateKey` in `YYYYMMDD` format. The dashboard expects no more than 14 records.
- `monthlyHistory`: retained monthly records, each with `monthKey` in `YYYYMM` format. The dashboard expects no more than 12 records.

Persisted daily date keys are validated as real calendar dates (including leap years) before being accepted; monthly keys must contain a valid year and month. Invalid saved history is discarded safely rather than being exposed as malformed API data. The dashboard uses a 5-second request timeout and bounded retry backoff up to 60 seconds. A successful response resets the refresh interval to 10 seconds. During temporary failures it preserves the last valid snapshot and marks it stale. It also rejects malformed calendar keys, inconsistent selected-pet data, missing numeric counters, and invalid history entries before updating the UI. CSV export quotes cells and prefixes formula-like values to reduce spreadsheet formula-injection risk.

All duration values ending in `Ms` are milliseconds. Session counters are counts, not durations. JSON numeric values should be treated as unsigned counters where applicable.

Example shape (values are illustrative):

```json
{
  "version": 3,
  "totals": {
    "workMs": 120000,
    "playMs": 30000,
    "napMs": 0,
    "completedWorkSessions": 2
  },
  "recent": {
    "requestedDays": 7,
    "daysIncluded": 1,
    "workMs": 120000,
    "playMs": 30000,
    "napMs": 0,
    "completedWorkSessions": 2
  },
  "dailyHistory": [],
  "monthlyHistory": []
}
```

## Integration notes

- The JSON snapshot is available over USB serial and the local Wi-Fi dashboard at `GET /api/stats`.
- The ESP32 starts its own Wi-Fi access point using credentials stored in Preferences. On a fresh device, the initial credentials are SSID `PashmakOS` and password `pashmak123`; change them in the dashboard's **Wi-Fi access point** section. SSIDs must be 1–32 printable ASCII characters (quotes and backslashes are not accepted); passwords must be 8–63 printable ASCII characters. Leave the password field blank to retain the current password. Saving credentials restarts the AP, disconnecting connected clients; reconnect using the new credentials and open `http://192.168.4.1/`. Internet access is not required.
- The dashboard refreshes automatically every 10 seconds. The API returns lifetime totals plus up to 14 daily and 12 monthly records.
- Wi-Fi credentials are stored in a versioned Preferences record and survive reboot. The current password is not returned by the settings API. Anyone connected to the AP can still access dashboard APIs, so choose a strong password and share it only with trusted users. The default credentials are intended for initial setup and should be changed.
- History is retained in ESP32 Preferences. Daily history keeps up to 14 records and monthly history up to 12 records.
- The current schema is versioned so a future dashboard can detect incompatible changes.


## Timer settings API

The local dashboard exposes a separate timer configuration API; it does not change the statistics payload schema.

- `GET /api/timer-settings`: returns `workMinutes`, `playMinutes`, `napMinutes`, and `workCyclesBeforeLongRest`.
- `POST /api/timer-settings`: accepts the same four form fields as URL-encoded values and applies them immediately.
- Valid ranges: work 1–180 minutes, play 1–60 minutes, long rest 1–120 minutes, and 1–8 work sessions before a long rest.
- Settings are stored as a versioned Preferences blob in the existing `petstats` namespace. Invalid or incompatible saved settings fall back to defaults (25/2/5 minutes and 3 work sessions).
- Invalid or missing values return HTTP 400. A storage failure returns HTTP 500; an unavailable app returns HTTP 503.

## Reminder schedule API

- `GET /api/reminder-settings`: returns `waterMinutes` and `movementMinutes`.
- `POST /api/reminder-settings`: accepts URL-encoded `waterMinutes` and `movementMinutes` and applies the schedule immediately.
- Water reminders accept 15–240 minutes, or `0` to disable. Movement reminders accept 15–180 minutes, or `0` to disable.
- Settings are stored in the `petstats` Preferences namespace as a separate versioned blob (`reminderCfg`). Missing, invalid, or incompatible stored data falls back to 60-minute water and 45-minute movement reminders.
- Quiet hours still suppress both reminders. Disabling a reminder resets its interval clock while disabled, so re-enabling starts a fresh interval rather than firing immediately. Invalid requests return HTTP 400, persistence failures return HTTP 500, and an unavailable app returns HTTP 503.

## Dashboard JSON export

The dashboard header's **Export JSON** action downloads a local snapshot containing:

- The current `/api/stats` payload, including pet profiles, lifetime counters, and retained daily/monthly history.
- The browser-local daily focus-session goal.
- Timer settings, quiet-hours settings, and reminder intervals last successfully loaded from or saved to the device. A setting is `null` if it could not be loaded during this page session.
- An ISO 8601 export timestamp and export schema version.

The file is generated in the browser; it is not uploaded to a server. It is an export for backup or analysis, not a restore/import file. Keep the JSON private if you do not want to share your activity history.


## CSV export

The `daily_csv` and `monthly_csv` commands print header rows and comma-separated numeric records. Copy only the content between the matching begin/end markers into a `.csv` file. The `dateKey` format is `YYYYMMDD`; `monthKey` is `YYYYMM`. Durations are milliseconds.

## Pet interaction and bonding

- `POST /api/pet/interact` triggers a personality-specific reaction on the TFT and increases the selected pet's bond by 5, capped at 100.
- Each pet has an independent 8-second cooldown; requests during cooldown return HTTP 429 with `Retry-After: 8`. An unavailable app returns HTTP 503.
- Bond is stored in ESP32 Preferences under a per-pet key and loaded after reboot. Each accepted interaction persists only that pet's bond value; the cooldown limits write frequency.
- Reactions are temporary (about 6 seconds) and the pet's normal behavior message resumes afterward.

## Quiet-hours API

- `GET /api/quiet-hours`: returns `enabled`, `startHour`, and `endHour`.
- `POST /api/quiet-hours` accepts URL-encoded `enabled` (`0` or `1`), `startHour`, and `endHour` fields. Hours are integers from 0 through 23; start and end must differ.
- Quiet hours are persisted in the `petstats` Preferences namespace using a separate versioned blob; this does not alter the timer-settings or statistics schema. Invalid or incompatible saved settings fall back to enabled 21:00–09:00 quiet hours.
- Quiet hours suppress spontaneous pet messages and RTC reminders, including clearing an already visible spontaneous event. Ranges that cross midnight are supported (for example, 21 to 9). With quiet hours disabled, the existing RTC daytime restriction is not applied.
- Invalid requests return HTTP 400, persistence failures return HTTP 500, and an unavailable app returns HTTP 503. RTC-dependent behavior requires a working DS3231.


## System diagnostics API

`GET /api/diagnostics` returns a non-persistent, read-only snapshot of runtime health. The response includes uptime in seconds, current/minimum free heap, largest allocatable heap block, free PSRAM, connected access-point clients, CPU frequency in MHz, and the ESP-IDF reset-reason code. Responses use `Cache-Control: no-store`. These values are live measurements; they are not a historical log and may change between requests. Reset-reason codes follow the ESP-IDF `esp_reset_reason()` enum for the target framework version.


## Settings backup and restore

The dashboard can export a versioned JSON backup containing timer-cycle settings, quiet-hours settings, and reminder intervals. Restore validates the file schema and all supported ranges before submitting values through the existing settings APIs. Because each settings group is persisted by its own endpoint, a network or storage error during restore can leave earlier groups restored; the UI reports this possibility. Files larger than 16 KiB are rejected. Wi-Fi SSID/password and activity history are intentionally excluded; the backup does not contain secrets or historical records.


## Persistent event log

`GET /api/events` returns the latest events in newest-first order. The fixed-capacity ring buffer retains 24 records in the `pashmak_log` Preferences namespace. Records include a sequence number, uptime seconds for the current boot, event code, and event-specific detail. Events cover boot, access-point startup failure, pet selection, and successful timer, quiet-hours, reminder, and Wi-Fi settings updates. Uptime is relative to the boot that recorded the event, not wall-clock time; no RTC timestamp is implied. Frequent pet interactions are intentionally omitted to reduce flash writes. The endpoint is read-only and sends `Cache-Control: no-store`.


## Firmware OTA update

- The dashboard provides a firmware update form that accepts a compiled ESP32 .bin image at POST /api/ota.
- The upload uses the ESP32 Arduino Update API, which checks the available OTA partition and validates the image while writing. The device restarts only after a successful finalization; failed or interrupted uploads must not trigger a restart.
- Use a firmware binary built for this exact board and partition layout. Keep the device powered and connected during upload.
- **Security:** dashboard APIs are available to any client connected to the device access point. Change the default SSID password to a strong, unique password before enabling firmware updates and only connect trusted devices. OTA is not exposed as an authenticated internet service.
- If the device does not return after an update, recover by reflashing over USB using PlatformIO.


Pet records in schema version 2 include `happiness` (integer, 0–100), alongside `energy` and `bond`. Version 2 adds this required field to both `pets[]` and the active `pet` record; clients must validate the version before consuming the payload.

## Persistent achievements

Achievement progress is stored locally in the ESP32 Preferences namespace as a versioned, validated record. Per-pet interaction counts and achievement masks remain separate for each pet; focus-session milestones and the three-consecutive-day streak use a global mask. Achievement bits are awarded once and persisted immediately when a new achievement unlocks. Other progress is coalesced into writes no more frequently than once per minute. Invalid records fall back to an empty progress record. The dashboard only renders achievements from the validated API masks, rather than inferring unlocked status from browser-local state.
