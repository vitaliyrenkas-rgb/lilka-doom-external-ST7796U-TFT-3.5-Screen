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

// Validated ST7796U timing from FFC CORE #013/#015B.
#define EXT_TFT_INIT_SPI_HZ 15000000UL
#define EXT_TFT_BULK_SPI_HZ 125000000UL

// Keep the proven ILI9488 Doom presentation geometry unchanged.
// Doom native framebuffer: 320x200 packed RGB888.
// External presentation: exact 5:4 expansion to 400x250, centered.
#define DOOM_PRESENT_W 400
#define DOOM_PRESENT_H 250
#define DOOM_PRESENT_X 40
#define DOOM_PRESENT_Y 35

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
