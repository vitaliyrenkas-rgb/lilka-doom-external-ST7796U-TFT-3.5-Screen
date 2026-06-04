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

// Doom native framebuffer is 320x200. We present it as 480x300
// with 10 px black bars at top and bottom, preserving aspect ratio.
//TFT 3.5` Original Resolution - 8FPS
// #define DOOM_PRESENT_W 480
// #define DOOM_PRESENT_H 300
// #define DOOM_PRESENT_X 0
// #define DOOM_PRESENT_Y 10

//DOOM Original resolution- 18FPS
// #define DOOM_PRESENT_W 320
// #define DOOM_PRESENT_H 200
// #define DOOM_PRESENT_X 80
// #define DOOM_PRESENT_Y 60

//Compromized solution
// #define DOOM_PRESENT_W = 384
// #define DOOM_PRESENT_H = 240
// #define DOOM_PRESENT_X = 48
// #define DOOM_PRESENT_Y = 40

//Compromized Solution 2
#define DOOM_PRESENT_W 400
#define DOOM_PRESENT_H 250
#define DOOM_PRESENT_X 40
#define DOOM_PRESENT_Y 35

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
