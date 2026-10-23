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

- The JSON snapshot is currently available over USB serial only; this does not start Wi-Fi, HTTP, or MQTT.
- History is retained in ESP32 Preferences. Daily history keeps up to 14 records and monthly history up to 12 records.
- The current schema is versioned so a future dashboard can detect incompatible changes.
