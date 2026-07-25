#pragma once

#include <Arduino.h>

// External ST7796U 3.5" FFC TFT on Lilka expansion port.
// NOTE: The legacy filename is kept for this functional patch only; after the
// 480-wide/performance test passes, move it to external_st7796u.* in the new FFC repo.
//
// Validated FFC/ST7796U wiring:
//   SCK  -> GPIO12
//   MOSI -> GPIO14
//   CS   -> GPIO48
//   DC   -> RX / GPIO44
//   RST  -> GPIO47
//   MISO -> not connected for bring-up
//   BL   -> GPIO21 physically, but firmware must not drive it here
//   VCC/LED -> externally powered as in KeiraOS FFC baseline

#define EXT_TFT_SCK  12
#define EXT_TFT_MOSI 14
#define EXT_TFT_CS   48
#define EXT_TFT_DC   44
#define EXT_TFT_RST  47

#define EXT_TFT_W    480
#define EXT_TFT_H    320

#ifndef EXT_TFT_INIT_SPI_HZ
#define EXT_TFT_INIT_SPI_HZ 15000000UL
#endif

#ifndef EXT_TFT_BULK_SPI_HZ
#define EXT_TFT_BULK_SPI_HZ 125000000UL
#endif

#ifndef EXT_TFT_CHUNK_ROWS
#define EXT_TFT_CHUNK_ROWS 8
#endif

// ST7796U IPS panel color policy.
// #002 proved INVON fixes the panel colors while RGB565 transport stays valid.
#ifndef EXT_TFT_ENABLE_INVERSION
#define EXT_TFT_ENABLE_INVERSION 1
#endif

// Leave RGB565 byte/color order unchanged by default. If a hardware revision
// shows red/blue swapped instead of true inversion, set this to 1 in platformio.ini.
#ifndef EXT_TFT_SWAP_RB
#define EXT_TFT_SWAP_RB 0
#endif

// DOOM FFC #003 target:
// Doom native framebuffer is 320x200; present it as 480x300 with 10 px black
// bars at top/bottom. The .cpp uses an exact 3:2 scaler fast path for this mode.
#ifndef DOOM_PRESENT_W
#define DOOM_PRESENT_W 480
#endif
#ifndef DOOM_PRESENT_H
#define DOOM_PRESENT_H 300
#endif
#ifndef DOOM_PRESENT_X
#define DOOM_PRESENT_X 0
#endif
#ifndef DOOM_PRESENT_Y
#define DOOM_PRESENT_Y 10
#endif

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
