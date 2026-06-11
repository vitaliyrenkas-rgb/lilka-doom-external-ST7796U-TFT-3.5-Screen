#pragma once

#include <Arduino.h>

// External ILI9488 3.5" TFT on Lilka expansion port.
// Wiring validated in KeiraOS external TFT branch:
//   SCK  -> GPIO12
//   MOSI -> GPIO14
//   CS   -> GPIO48
//   DC   -> RX  / GPIO44
//   RST  -> TX  / GPIO43
//   MISO -> not connected
//   LED/BL -> external/current-limited path, not controlled here

#define EXT_TFT_SCK  12
#define EXT_TFT_MOSI 14
#define EXT_TFT_CS   48
#define EXT_TFT_DC   44
#define EXT_TFT_RST  43

#define EXT_TFT_W    480
#define EXT_TFT_H    320

// Doom native framebuffer is 320x200. Keep the active render area smaller
// when needed; black borders are intentional if they buy FPS.
#define DOOM_TFT_PROFILE_FAST_320     1 // 320x200 centered, safest FPS target
#define DOOM_TFT_PROFILE_MID_360      2 // 360x225 centered, possible sweet spot
#define DOOM_TFT_PROFILE_BALANCED_400 3 // 400x250 centered, current visual target
#define DOOM_TFT_PROFILE_BIG_480      4 // 480x300, likely heavy on SPI

#ifndef DOOM_TFT_PROFILE
#define DOOM_TFT_PROFILE DOOM_TFT_PROFILE_BALANCED_400
#endif

#if DOOM_TFT_PROFILE == DOOM_TFT_PROFILE_FAST_320
#define DOOM_PRESENT_W 320
#define DOOM_PRESENT_H 200
#define DOOM_PRESENT_X 80
#define DOOM_PRESENT_Y 60
#elif DOOM_TFT_PROFILE == DOOM_TFT_PROFILE_MID_360
#define DOOM_PRESENT_W 360
#define DOOM_PRESENT_H 225
#define DOOM_PRESENT_X 60
#define DOOM_PRESENT_Y 47
#elif DOOM_TFT_PROFILE == DOOM_TFT_PROFILE_BALANCED_400
#define DOOM_PRESENT_W 400
#define DOOM_PRESENT_H 250
#define DOOM_PRESENT_X 40
#define DOOM_PRESENT_Y 35
#elif DOOM_TFT_PROFILE == DOOM_TFT_PROFILE_BIG_480
#define DOOM_PRESENT_W 480
#define DOOM_PRESENT_H 300
#define DOOM_PRESENT_X 0
#define DOOM_PRESENT_Y 10
#else
#error "Unknown DOOM_TFT_PROFILE"
#endif

#define DOOM_TFT_PIXFMT_RGB565 565 // preferred FPS path: COLMOD 0x55, 2 bytes/pixel
#define DOOM_TFT_PIXFMT_RGB666 666 // diagnostic fallback: COLMOD 0x66, 3 bytes/pixel

#ifndef DOOM_TFT_PIXFMT
// RGB565 / COLMOD 0x55 produced a gray external TFT image on the current ILI9488 module.
// Keep the FPS diet profile/scaling-map changes, but fall back to the known-good RGB666 stream.
#define DOOM_TFT_PIXFMT DOOM_TFT_PIXFMT_RGB666
#endif

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
