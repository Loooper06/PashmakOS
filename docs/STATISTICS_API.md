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

The `stats` response is a single JSON object with `version: 1`:

- `totals`: lifetime counters currently stored in Preferences.
- `recent`: aggregate of the most recent available daily records, up to the requested number of days (currently 7).
- `dailyHistory`: retained daily records, each with `dateKey` in `YYYYMMDD` format.
- `monthlyHistory`: retained monthly records, each with `monthKey` in `YYYYMM` format.

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


## CSV export

The `daily_csv` and `monthly_csv` commands print header rows and comma-separated numeric records. Copy only the content between the matching begin/end markers into a `.csv` file. The `dateKey` format is `YYYYMMDD`; `monthKey` is `YYYYMM`. Durations are milliseconds.
