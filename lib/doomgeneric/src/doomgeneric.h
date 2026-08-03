#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdlib.h>
#include <stdint.h>

#define DOOMGENERIC_RESX 320
#define DOOMGENERIC_RESY 200

// Lilka external TFT FPS path: keep Doom native 8-bit framebuffer,
// but publish the ready frame as packed RGB888 bytes. The ILI9488
// RGB666 SPI path can stream these bytes directly without a second
// 32-bit -> 24-bit repack in the draw task.
#define DOOMGENERIC_FRAMEBUFFER_BPP 24
#define DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL (DOOMGENERIC_FRAMEBUFFER_BPP / 8)
#define DOOMGENERIC_FRAMEBUFFER_BYTES \
    (DOOMGENERIC_RESX * DOOMGENERIC_RESY * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL)
#define DOOMGENERIC_FRAMEBUFFER_RGB888_PACKED 1

extern uint32_t* DG_ScreenBuffer;

void doomgeneric_Create(int argc, char **argv);
void doomgeneric_Tick();


//Implement below functions for your platform
void DG_Init();
void DG_DrawFrame();
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs();
int DG_GetKey(int* pressed, unsigned char* key);
void DG_SetWindowTitle(const char * title);

#endif //DOOM_GENERIC
