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
// Full-width ST7796U test: exact 3:2 horizontal and 3:2 vertical
// expansion to 480x300, with 10-pixel top/bottom bars.
#define DOOM_PRESENT_W 480
#define DOOM_PRESENT_H 300
#define DOOM_PRESENT_X 0
#define DOOM_PRESENT_Y 10

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
