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

    // Buttons
    constexpr uint8_t BTN_MODE = 32;
    constexpr uint8_t BTN_NAP = 33;
    constexpr uint8_t BTN_PLAY = 25;

    // Display
    constexpr uint16_t SCREEN_WIDTH = 320;
    constexpr uint16_t SCREEN_HEIGHT = 240;
    constexpr uint8_t TFT_ROTATION = 1;
    constexpr uint32_t TFT_SPI_FREQUENCY = 27000000;

    // Application
    constexpr uint32_t FRAME_INTERVAL_MS = 100;
    constexpr uint32_t BUTTON_DEBOUNCE_MS = 35;
}