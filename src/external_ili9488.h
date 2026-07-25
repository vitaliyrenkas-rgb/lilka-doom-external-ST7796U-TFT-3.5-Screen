#pragma once

#include <Arduino.h>

// External ST7796U 3.5" FFC TFT on Lilka expansion port.
// NOTE: The legacy filename is kept to avoid touching Doom/main includes in this
// first bring-up patch.
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
#define EXT_TFT_CHUNK_ROWS 4
#endif

// DOOM FFC #001 bring-up target:
// first get a correct picture, then bring back 400x250/5:4 expander and FPS work.
// Doom native framebuffer is 320x200; present it centered on 480x320.
#ifndef DOOM_PRESENT_W
#define DOOM_PRESENT_W 320
#endif
#ifndef DOOM_PRESENT_H
#define DOOM_PRESENT_H 200
#endif
#ifndef DOOM_PRESENT_X
#define DOOM_PRESENT_X 80
#endif
#ifndef DOOM_PRESENT_Y
#define DOOM_PRESENT_Y 60
#endif

bool externalTftBegin();
void externalTftClear(uint8_t r, uint8_t g, uint8_t b);
void externalTftPresentDoomFrame(const uint32_t* framebuffer);
