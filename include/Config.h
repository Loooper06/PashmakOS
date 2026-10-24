#pragma once

#include <Arduino.h>

namespace Config
{
    // TFT — ESP32-WROOM-32
    constexpr uint8_t TFT_PIN_SCLK = 18;
    constexpr uint8_t TFT_PIN_MOSI = 23;
    constexpr int8_t TFT_PIN_MISO = -1;
    constexpr uint8_t TFT_PIN_CS = 5;
    constexpr uint8_t TFT_PIN_DC = 27;
    constexpr uint8_t TFT_PIN_RST = 26;
    constexpr int8_t TFT_PIN_BL = 21;

    // DS3231 I2C. GPIO21 is reserved for TFT backlight in this build.
    constexpr uint8_t RTC_PIN_SDA = 16;
    constexpr uint8_t RTC_PIN_SCL = 17;

    // Legacy button pin assignments (not used for mode switching).
    constexpr uint8_t BTN_MODE = 32;
    constexpr uint8_t BTN_NAP = 33;
    constexpr uint8_t BTN_PLAY = 25;

    // Display
    constexpr uint16_t SCREEN_WIDTH = 320;
    constexpr uint16_t SCREEN_HEIGHT = 240;
    constexpr uint8_t TFT_ROTATION = 1;
    constexpr uint32_t TFT_SPI_FREQUENCY = 27000000;

    // Application timing
    constexpr uint32_t FRAME_INTERVAL_MS = 100;
    constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;

    // Automatic pet behavior (milliseconds)
    constexpr uint32_t WORK_SESSION_DURATION_MS = 25UL * 60UL * 1000UL;
    constexpr uint32_t PLAY_SESSION_DURATION_MS = 2UL * 60UL * 1000UL;
    constexpr uint32_t NAP_SESSION_DURATION_MS = 5UL * 60UL * 1000UL;
    constexpr uint32_t REMINDER_DISPLAY_DURATION_MS = 12000UL;
    constexpr uint8_t WORK_CYCLES_BEFORE_LONG_REST = 3;
    constexpr uint32_t PET_MESSAGE_INTERVAL_MS = 45000UL;

    // Reminder scheduling
    constexpr uint32_t RTC_POLL_INTERVAL_MS = 1000;
    constexpr uint32_t WATER_REMINDER_INTERVAL_SECONDS = 60UL * 60UL;
    constexpr uint32_t MOVEMENT_REMINDER_INTERVAL_SECONDS = 45UL * 60UL;
}