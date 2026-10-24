# PashmakOS setup and troubleshooting

Target hardware: ESP32-WROOM-32 development board, ILI9341 TFT (320×240 landscape), and DS3231 RTC. Verify the actual module pin labels and board variant before wiring; never rely on physical header position alone.

## Pin assignments used by this firmware

| Signal | ESP32 GPIO | Notes |
|---|---:|---|
| TFT SCLK | 18 | SPI clock |
| TFT MOSI | 23 | SPI data to display |
| TFT MISO | Not connected | Configured as `-1` |
| TFT CS | 5 | Chip select |
| TFT DC | 27 | Data/command |
| TFT RST | 26 | Reset |
| TFT backlight control | 21 | Firmware config assigns this pin; verify your display module's backlight circuit and polarity |
| DS3231 SDA | 16 | I²C data |
| DS3231 SCL | 17 | I²C clock |

Legacy button definitions are GPIO32, GPIO33, and GPIO25, but the current firmware comments identify them as not used for mode switching. Do not assume these buttons provide user input in this build.

Power and logic levels depend on the exact ESP32 board and TFT/RTC modules. Confirm module documentation, common ground, and 3.3 V GPIO compatibility before connecting. Do not feed 5 V into ESP32 GPIO pins.

## Build and flash

1. Install PlatformIO in VS Code or use the PlatformIO CLI.
2. Open the repository and build the `esp32dev` environment: `pio run -e esp32dev`.
3. Connect the ESP32 over USB and upload: `pio run -e esp32dev -t upload`.
4. Open the serial monitor at **115200 baud**.
5. For a successful startup, look for the automatic behavior startup line and the status lines for Preferences and RTC.

The GitHub Actions workflow also builds this environment and runs native behavior, dashboard JavaScript, and statistics JSON contract checks.

## RTC setup

- Connect DS3231 SDA to GPIO16, SCL to GPIO17, plus the module's documented power and ground connections.
- On startup, the firmware logs whether the DS3231 was detected and its reported date/time.
- If the RTC reports lost backup power, this firmware seeds the clock from the firmware build timestamp. That is only a fallback; it is not the current network time. Set/verify the RTC with a trusted clock before relying on daily history or time-aware reminders.
- If the RTC is missing, daily/monthly history recording and time-based reminders cannot operate as intended. Check wiring, common ground, power, SDA/SCL assignments, and backup battery.

## Local dashboard

- The ESP32 starts a local access point with the default SSID `PashmakOS` and password `pashmak123`.
- Connect a phone or computer to that network and open `http://192.168.4.1/`.
- Internet access is not required for the local dashboard.
- The default AP password is for development only. Change the SSID/password in `src/web/WebDashboard.cpp` before sharing or deploying the device; use a unique password of at least 8 characters.
- API schema, serial commands, and CSV export details are documented in `docs/STATISTICS_API.md`.

## Preferences and persistence

Lifetime counters, selected pet ID, daily history, monthly history, and RTC reminder state use ESP32 Preferences/NVS. Normal resets should preserve successfully written values. This does not guarantee that a sudden power interruption during a write cannot lose the most recent update.

Daily history retains up to 14 days and monthly history up to 12 months. Invalid version/size/date/order data is rejected rather than exposed to the dashboard. If Preferences cannot be opened, the serial log warns that lifetime statistics are RAM-only.

## Troubleshooting

| Symptom | Checks |
|---|---|
| TFT stays blank | Confirm panel type, CS/DC/RST/SCLK/MOSI wiring, power, and backlight circuit. Check serial output and verify the configured ILI9341 driver. |
| TFT colors/orientation are wrong | Confirm the module is ILI9341-compatible and the configured rotation is intentional. |
| RTC not detected | Check DS3231 power, ground, SDA GPIO16, SCL GPIO17, and backup battery. |
| Date/history is wrong | Verify RTC date/time first; history is grouped by RTC calendar date/month. |
| Dashboard does not load | Confirm connection to the PashmakOS AP and browse to `http://192.168.4.1/`; check serial diagnostics. |
| Dashboard shows stale data | Confirm the device is still reachable and `/api/stats` returns valid JSON; see `docs/STATISTICS_API.md`. |
| Counters reset after reboot | Check whether Preferences opened successfully at boot, inspect `[STATS]` persistence errors, and repeat the normal reboot test in `docs/HARDWARE_SMOKE_TEST.md`. |
| Daily/monthly history is empty | Confirm RTC availability and valid date/time; a missing RTC prevents calendar history from being recorded. |

## Before calling a build release-ready

Run every item in `docs/HARDWARE_SMOKE_TEST.md` on the actual board and record the firmware commit and observed results. CI success is necessary but does not prove display quality, RTC accuracy, Wi-Fi recovery, or persistence on physical hardware.
