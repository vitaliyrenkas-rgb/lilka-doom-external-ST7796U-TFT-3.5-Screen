#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdlib.h>
#include <stdint.h>

#define DOOMGENERIC_RESX 320
#define DOOMGENERIC_RESY 200

// ST7796U FPS path: publish each ready Doom frame directly as RGB565
// bytes in wire order (high byte, low byte). The external presenter can
// scale by copying 16-bit words without a second RGB888 -> RGB565 pass.
#define DOOMGENERIC_FRAMEBUFFER_BPP 16
#define DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL (DOOMGENERIC_FRAMEBUFFER_BPP / 8)
#define DOOMGENERIC_FRAMEBUFFER_BYTES \
    (DOOMGENERIC_RESX * DOOMGENERIC_RESY * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL)
#define DOOMGENERIC_FRAMEBUFFER_RGB565_WIRE_ORDER 1

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
