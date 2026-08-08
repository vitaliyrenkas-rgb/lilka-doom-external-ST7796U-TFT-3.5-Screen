#pragma once

#include <Arduino.h>

// External ST7796U 3.5" FFC TFT on Lilka expansion port.
// The file name remains external_ili9488.* deliberately so the proven Doom
// lifecycle, main.cpp, framebuffer path, controls, and profiler stay untouched.
//
// Validated FFC/ST7796U wiring:
//   SCK  -> GPIO12
//   MOSI -> GPIO14
//   CS   -> GPIO48
//   DC   -> RX / GPIO44
//   RST  -> GPIO47
//   MISO -> not connected
//   BL   -> external 5 V; firmware must not drive it

#define EXT_TFT_SCK  12
#define EXT_TFT_MOSI 14
#define EXT_TFT_CS   48
#define EXT_TFT_DC   44
#define EXT_TFT_RST  47
#define EXT_TFT_BL   21

#define EXT_TFT_W    480
#define EXT_TFT_H    320

// Validated ST7796U init timing; bulk path uses ESP32-S3 GP-SPI at 80 MHz.
#define EXT_TFT_INIT_SPI_HZ 15000000UL
#define EXT_TFT_BULK_SPI_HZ 80000000UL

// Doom native framebuffer: 320x200 packed wire-order RGB565.
// VANILLA 35 transport test: exact 5:4 horizontal and 5:4 vertical
// expansion to centered 400x250, with black borders on the 480x320 panel.
#define DOOM_PRESENT_W 400
#define DOOM_PRESENT_H 250
#define DOOM_PRESENT_X 40
#define DOOM_PRESENT_Y 35

enum class DoomDisplayMode : uint8_t {
    VANILLA_400X250 = 0,
    QUALITY_480X300 = 1,
};

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftSetDoomDisplayMode(DoomDisplayMode mode);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
