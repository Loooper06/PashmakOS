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

The `stats` response is a single JSON object with `version: 1`. The dashboard validates the payload before rendering it; an invalid response is treated as a failed refresh, and the last valid snapshot remains visible.

- `activePetId`: selected pet ID (0–2).
- `pets`: exactly three pet profiles. Each profile has `id`, `name`, `energy` (0–100), `bond` (0–100), `mood`, and `message`.
- `pet`: the selected pet's `energy`, `mood`, and `message`. This duplicates the selected profile's `energy`, `bond`, `mood`, and `message` for convenient dashboard rendering.
- `totals`: lifetime counters currently stored in Preferences.
- `recent`: aggregate of the most recent available daily records, up to the requested number of days (the serial command currently requests 7; the dashboard requests 14).
- `dailyHistory`: retained daily records, each with `dateKey` in `YYYYMMDD` format. The dashboard expects no more than 14 records.
- `monthlyHistory`: retained monthly records, each with `monthKey` in `YYYYMM` format. The dashboard expects no more than 12 records.

Persisted daily date keys are validated as real calendar dates (including leap years) before being accepted; monthly keys must contain a valid year and month. Invalid saved history is discarded safely rather than being exposed as malformed API data. The dashboard uses a 5-second request timeout and bounded retry backoff up to 60 seconds. A successful response resets the refresh interval to 10 seconds. During temporary failures it preserves the last valid snapshot and marks it stale. It also rejects malformed calendar keys, inconsistent selected-pet data, missing numeric counters, and invalid history entries before updating the UI. CSV export quotes cells and prefixes formula-like values to reduce spreadsheet formula-injection risk.

All duration values ending in `Ms` are milliseconds. Session counters are counts, not durations. JSON numeric values should be treated as unsigned counters where applicable.

Example shape (values are illustrative):

```json
{
  "version": 1,
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
- The ESP32 starts its own Wi-Fi access point; connect to SSID `PashmakOS` with password `pashmak123`, then open `http://192.168.4.1/` on a phone or computer. Internet access is not required while connected to this access point.
- The dashboard refreshes automatically every 10 seconds. The API returns lifetime totals plus up to 14 daily and 12 monthly records.
- Change the access-point SSID/password in `src/web/WebDashboard.cpp` before sharing the device. Use a password of at least 8 characters.
- History is retained in ESP32 Preferences. Daily history keeps up to 14 records and monthly history up to 12 records.
- The current schema is versioned so a future dashboard can detect incompatible changes.


## Timer settings API

The local dashboard exposes a separate timer configuration API; it does not change the statistics payload schema.

- `GET /api/timer-settings`: returns `workMinutes`, `playMinutes`, `napMinutes`, and `workCyclesBeforeLongRest`.
- `POST /api/timer-settings`: accepts the same four form fields as URL-encoded values and applies them immediately.
- Valid ranges: work 1–180 minutes, play 1–60 minutes, long rest 1–120 minutes, and 1–8 work sessions before a long rest.
- Settings are stored as a versioned Preferences blob in the existing `petstats` namespace. Invalid or incompatible saved settings fall back to defaults (25/2/5 minutes and 3 work sessions).
- Invalid or missing values return HTTP 400. A storage failure returns HTTP 500; an unavailable app returns HTTP 503.

## CSV export

The `daily_csv` and `monthly_csv` commands print header rows and comma-separated numeric records. Copy only the content between the matching begin/end markers into a `.csv` file. The `dateKey` format is `YYYYMMDD`; `monthKey` is `YYYYMM`. Durations are milliseconds.

## Pet interaction and bonding

- `POST /api/pet/interact` triggers a personality-specific reaction on the TFT and increases the selected pet's bond by 5, capped at 100.
- Each pet has an independent 8-second cooldown; requests during cooldown return HTTP 429 with `Retry-After: 8`. An unavailable app returns HTTP 503.
- Bond is stored in ESP32 Preferences under a per-pet key and loaded after reboot. Each accepted interaction persists only that pet's bond value; the cooldown limits write frequency.
- Reactions are temporary (about 6 seconds) and the pet's normal behavior message resumes afterward.
