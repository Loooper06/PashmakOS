#pragma once

#define ILI9341_2_DRIVER

// TFT_eSPI's ILI9341 driver defines the native panel dimensions (240x320).
// Landscape dimensions are selected at runtime with setRotation(1).

#define TFT_MOSI 23
#define TFT_MISO -1
#define TFT_SCLK 18

#define TFT_CS 5
#define TFT_DC 27
#define TFT_RST 26

#define SPI_FREQUENCY 20000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
