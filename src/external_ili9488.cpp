#include "external_ili9488.h"

#include <SPI.h>

extern "C" {
#include "doomgeneric.h"
}

static SPIClass extSpi(FSPI);
static bool extReady = false;

static bool scaleMapsReady = false;
static uint16_t srcXMap[DOOM_PRESENT_W];
// Offset in bytes into packed RGB888 source framebuffer.
static size_t srcYOffsetMap[DOOM_PRESENT_H];

static inline void tftSelect() {
    digitalWrite(EXT_TFT_CS, LOW);
}

static inline void tftDeselect() {
    digitalWrite(EXT_TFT_CS, HIGH);
}

static inline void tftCommandMode() {
    digitalWrite(EXT_TFT_DC, LOW);
}

static inline void tftDataMode() {
    digitalWrite(EXT_TFT_DC, HIGH);
}

static void writeCommand(uint8_t cmd) {
    tftSelect();
    tftCommandMode();
    extSpi.write(cmd);
    tftDeselect();
}

static void writeData8(uint8_t data) {
    tftSelect();
    tftDataMode();
    extSpi.write(data);
    tftDeselect();
}

static void writeCommandData(uint8_t cmd, const uint8_t* data, size_t len) {
    tftSelect();
    tftCommandMode();
    extSpi.write(cmd);
    if (len > 0) {
        tftDataMode();
        extSpi.writeBytes(data, len);
    }
    tftDeselect();
}

static void setAddressWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    const uint16_t x2 = x + w - 1;
    const uint16_t y2 = y + h - 1;

    uint8_t col[] = {
        static_cast<uint8_t>(x >> 8),
        static_cast<uint8_t>(x & 0xff),
        static_cast<uint8_t>(x2 >> 8),
        static_cast<uint8_t>(x2 & 0xff),
    };
    uint8_t row[] = {
        static_cast<uint8_t>(y >> 8),
        static_cast<uint8_t>(y & 0xff),
        static_cast<uint8_t>(y2 >> 8),
        static_cast<uint8_t>(y2 & 0xff),
    };

    writeCommandData(0x2A, col, sizeof(col)); // CASET
    writeCommandData(0x2B, row, sizeof(row)); // PASET
    writeCommand(0x2C); // RAMWR
}

bool externalTftBegin() {
    pinMode(EXT_TFT_CS, OUTPUT);
    pinMode(EXT_TFT_DC, OUTPUT);
    pinMode(EXT_TFT_RST, OUTPUT);
    digitalWrite(EXT_TFT_CS, HIGH);
    digitalWrite(EXT_TFT_DC, HIGH);

    digitalWrite(EXT_TFT_RST, LOW);
    delay(30);
    digitalWrite(EXT_TFT_RST, HIGH);
    delay(150);

    // Use a conservative SPI speed for the first Doom multiboot proof.
    // After picture is stable, this can be raised and benchmarked.
    extSpi.begin(EXT_TFT_SCK, -1, EXT_TFT_MOSI, EXT_TFT_CS);
    extSpi.beginTransaction(SPISettings(60000000, MSBFIRST, SPI_MODE0));

    writeCommand(0x01); // Software reset
    delay(120);
    writeCommand(0x11); // Sleep out
    delay(120);

    // ILI9488 SPI commonly expects RGB666 / 18-bit pixel writes.
    const uint8_t pixfmt[] = {0x66};
    writeCommandData(0x3A, pixfmt, sizeof(pixfmt));

    // Landscape 480x320. This matched the KeiraOS external TFT milestone.
    const uint8_t madctl[] = {0xE8};
    writeCommandData(0x36, madctl, sizeof(madctl));

    writeCommand(0x20); // Display inversion off
    writeCommand(0x29); // Display on
    delay(50);

    extReady = true;
    externalTftClear(0, 0, 0);
    return true;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady) {
        return;
    }

    static uint8_t row[EXT_TFT_W * 3];
    for (int x = 0; x < EXT_TFT_W; ++x) {
        row[x * 3 + 0] = r;
        row[x * 3 + 1] = g;
        row[x * 3 + 2] = b;
    }

    setAddressWindow(0, 0, EXT_TFT_W, EXT_TFT_H);
    tftSelect();
    tftDataMode();
    for (int y = 0; y < EXT_TFT_H; ++y) {
        extSpi.writeBytes(row, sizeof(row));
    }
    tftDeselect();
}

static void drawSolidRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r, uint8_t g, uint8_t b) {
    static uint8_t row[EXT_TFT_W * 3];
    const size_t bytes = static_cast<size_t>(w) * 3;
    for (uint16_t px = 0; px < w; ++px) {
        row[px * 3 + 0] = r;
        row[px * 3 + 1] = g;
        row[px * 3 + 2] = b;
    }

    setAddressWindow(x, y, w, h);
    tftSelect();
    tftDataMode();
    for (uint16_t py = 0; py < h; ++py) {
        extSpi.writeBytes(row, bytes);
    }
    tftDeselect();
}

static void buildScaleMaps() {
    if (scaleMapsReady) {
        return;
    }

    for (uint16_t x = 0; x < DOOM_PRESENT_W; ++x) {
        srcXMap[x] = (static_cast<uint32_t>(x) * DOOMGENERIC_RESX) / DOOM_PRESENT_W;
    }

    const size_t srcStride = static_cast<size_t>(DOOMGENERIC_RESX) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;

    for (uint16_t y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint16_t sy = (static_cast<uint32_t>(y) * DOOMGENERIC_RESY) / DOOM_PRESENT_H;
        srcYOffsetMap[y] = static_cast<size_t>(sy) * srcStride;
    }

    scaleMapsReady = true;
}

#define DOOM_TFT_NATIVE_DIRECT_RGB666 \
    (DOOMGENERIC_FRAMEBUFFER_RGB888_PACKED && \
     DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL == 3 && \
     DOOM_PRESENT_W == DOOMGENERIC_RESX && \
     DOOM_PRESENT_H == DOOMGENERIC_RESY)

void externalTftPresentDoomFrame(const uint32_t* framebuffer) {
    if (!extReady || framebuffer == nullptr) {
        return;
    }

    buildScaleMaps();


    static bool barsDrawn = false;
    if (!barsDrawn) {
        drawSolidRect(0, 0, EXT_TFT_W, DOOM_PRESENT_Y, 0, 0, 0);
        drawSolidRect(
            0, DOOM_PRESENT_Y + DOOM_PRESENT_H, EXT_TFT_W, EXT_TFT_H - DOOM_PRESENT_Y - DOOM_PRESENT_H, 0, 0, 0
        );
        barsDrawn = true;
    }

    const uint8_t* packed = reinterpret_cast<const uint8_t*>(framebuffer);

    setAddressWindow(DOOM_PRESENT_X, DOOM_PRESENT_Y, DOOM_PRESENT_W, DOOM_PRESENT_H);

    tftSelect();
    tftDataMode();

#if DOOM_TFT_NATIVE_DIRECT_RGB666
    for (int y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint8_t* src = packed + srcYOffsetMap[y];
        extSpi.writeBytes(const_cast<uint8_t*>(src), DOOM_PRESENT_W * 3);
    }
#else
    static uint8_t row[DOOM_PRESENT_W * 3];

    for (int y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint8_t* src = packed + srcYOffsetMap[y];

        for (int x = 0; x < DOOM_PRESENT_W; ++x) {
            const uint8_t* sp = src + static_cast<size_t>(srcXMap[x]) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;

            row[x * 3 + 0] = sp[0];
            row[x * 3 + 1] = sp[1];
            row[x * 3 + 2] = sp[2];
        }

        extSpi.writeBytes(row, sizeof(row));
    }
#endif

    tftDeselect();
}
