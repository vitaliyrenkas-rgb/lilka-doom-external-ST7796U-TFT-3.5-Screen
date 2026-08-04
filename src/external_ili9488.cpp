#include "external_ili9488.h"

#include <SPI.h>
#include <string.h>

extern "C" {
#include "doomgeneric.h"
}

static SPIClass extSpi(FSPI);
static bool extReady = false;
static bool extTransactionActive = false;

static bool scaleMapsReady = false;
static uint16_t srcXMap[DOOM_PRESENT_W];
// Offset in bytes into the proven packed RGB888 Doom framebuffer.
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

static void beginSpiTransaction(uint32_t hz) {
    if (extTransactionActive) {
        extSpi.endTransaction();
    }

    extSpi.beginTransaction(SPISettings(hz, MSBFIRST, SPI_MODE0));
    extTransactionActive = true;
}

static void writeCommand(uint8_t cmd) {
    tftSelect();
    tftCommandMode();
    extSpi.write(cmd);
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

static inline uint16_t packRgb565(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(
        (static_cast<uint16_t>(r & 0xF8) << 8) |
        (static_cast<uint16_t>(g & 0xFC) << 3) |
        (b >> 3)
    );
}

static inline void storePackedRgb888AsRgb565(const uint8_t* src, uint8_t* dst) {
    const uint16_t color = packRgb565(src[0], src[1], src[2]);
    dst[0] = static_cast<uint8_t>(color >> 8);
    dst[1] = static_cast<uint8_t>(color & 0xff);
}

static void fillRect565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r, uint8_t g, uint8_t b) {
    if (w == 0 || h == 0) {
        return;
    }

    static uint8_t row[EXT_TFT_W * 2];
    const uint16_t color = packRgb565(r, g, b);
    const uint8_t hi = static_cast<uint8_t>(color >> 8);
    const uint8_t lo = static_cast<uint8_t>(color & 0xff);
    const size_t rowBytes = static_cast<size_t>(w) * 2u;

    for (uint16_t px = 0; px < w; ++px) {
        row[px * 2 + 0] = hi;
        row[px * 2 + 1] = lo;
    }

    setAddressWindow(x, y, w, h);
    tftSelect();
    tftDataMode();
    for (uint16_t py = 0; py < h; ++py) {
        extSpi.writeBytes(row, rowBytes);
    }
    tftDeselect();
}

static void initST7796U() {
    // Exact controller sequence from the validated FFC CORE #013/#015B path.
    writeCommand(0x01); // Software reset
    delay(120);

    writeCommand(0x11); // Sleep out
    delay(120);

    const uint8_t extCommandPart1[] = {0xC3};
    writeCommandData(0xF0, extCommandPart1, sizeof(extCommandPart1));
    const uint8_t extCommandPart2[] = {0x96};
    writeCommandData(0xF0, extCommandPart2, sizeof(extCommandPart2));

    const uint8_t madctl[] = {0xE8};
    writeCommandData(0x36, madctl, sizeof(madctl));

    const uint8_t pixelFormat[] = {0x55}; // RGB565, 2 bytes/pixel
    writeCommandData(0x3A, pixelFormat, sizeof(pixelFormat));

    const uint8_t inversionControl[] = {0x01};
    writeCommandData(0xB4, inversionControl, sizeof(inversionControl));

    const uint8_t displayFunction[] = {0x80, 0x02, 0x3B};
    writeCommandData(0xB6, displayFunction, sizeof(displayFunction));

    const uint8_t displayOutputAdjust[] = {
        0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33,
    };
    writeCommandData(0xE8, displayOutputAdjust, sizeof(displayOutputAdjust));

    const uint8_t powerControl2[] = {0x06};
    writeCommandData(0xC1, powerControl2, sizeof(powerControl2));
    const uint8_t powerControl3[] = {0xA7};
    writeCommandData(0xC2, powerControl3, sizeof(powerControl3));
    const uint8_t vcomControl[] = {0x18};
    writeCommandData(0xC5, vcomControl, sizeof(vcomControl));
    delay(120);

    const uint8_t positiveGamma[] = {
        0xF0, 0x09, 0x0B, 0x06, 0x04, 0x15, 0x2F,
        0x54, 0x42, 0x3C, 0x17, 0x14, 0x18, 0x1B,
    };
    writeCommandData(0xE0, positiveGamma, sizeof(positiveGamma));

    const uint8_t negativeGamma[] = {
        0xE0, 0x09, 0x0B, 0x06, 0x04, 0x03, 0x2B,
        0x43, 0x42, 0x3B, 0x16, 0x14, 0x17, 0x1B,
    };
    writeCommandData(0xE1, negativeGamma, sizeof(negativeGamma));
    delay(120);

    const uint8_t disableExtCommandPart1[] = {0x3C};
    writeCommandData(0xF0, disableExtCommandPart1, sizeof(disableExtCommandPart1));
    const uint8_t disableExtCommandPart2[] = {0x69};
    writeCommandData(0xF0, disableExtCommandPart2, sizeof(disableExtCommandPart2));

    delay(120);
    writeCommand(0x13); // Normal display mode on
    delay(20);
    writeCommand(0x21); // Inversion on
    delay(20);
    writeCommand(0x29); // Display on
    delay(120);
}

bool externalTftBegin() {
    pinMode(EXT_TFT_CS, OUTPUT);
    pinMode(EXT_TFT_DC, OUTPUT);
    pinMode(EXT_TFT_RST, OUTPUT);
    pinMode(EXT_TFT_BL, INPUT); // BL is externally powered; never drive it.

    digitalWrite(EXT_TFT_CS, HIGH);
    digitalWrite(EXT_TFT_DC, HIGH);
    digitalWrite(EXT_TFT_RST, HIGH);

    delay(50);
    digitalWrite(EXT_TFT_RST, LOW);
    delay(80);
    digitalWrite(EXT_TFT_RST, HIGH);
    delay(150);

    extSpi.begin(EXT_TFT_SCK, -1, EXT_TFT_MOSI, EXT_TFT_CS);
    beginSpiTransaction(EXT_TFT_INIT_SPI_HZ);

    initST7796U();

    beginSpiTransaction(EXT_TFT_BULK_SPI_HZ);
    extReady = true;
    externalTftClear(0, 0, 0);
    return true;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady) {
        return;
    }

    fillRect565(0, 0, EXT_TFT_W, EXT_TFT_H, r, g, b);
}

static void drawSolidRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r, uint8_t g, uint8_t b) {
    fillRect565(x, y, w, h, r, g, b);
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

#define DOOM_TFT_SCALE_320X200_TO_400X300_RGB565 \
    (DOOMGENERIC_FRAMEBUFFER_RGB565_WIRE_ORDER && \
     DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL == 2 && \
     DOOMGENERIC_RESX == 320 && \
     DOOMGENERIC_RESY == 200 && \
     DOOM_PRESENT_W == 400 && \
     DOOM_PRESENT_H == 300)

#if DOOM_TFT_SCALE_320X200_TO_400X300_RGB565
static inline void expandDoomRow320To400Rgb565(
    const uint16_t* src,
    uint16_t* dst
) {
    // Exact 5:4 mapping: four source pixels become five TFT pixels
    // (0,0,1,2,3). Source words are already in wire byte order.
    for (int group = 0; group < 80; ++group) {
        const uint16_t p0 = src[0];
        const uint16_t p1 = src[1];
        const uint16_t p2 = src[2];
        const uint16_t p3 = src[3];

        dst[0] = p0;
        dst[1] = p0;
        dst[2] = p1;
        dst[3] = p2;
        dst[4] = p3;

        src += 4;
        dst += 5;
    }
}
#endif

void externalTftPresentDoomFrame(const uint32_t* framebuffer) {
    if (!extReady || framebuffer == nullptr) {
        return;
    }

    buildScaleMaps();

    static bool barsDrawn = false;
    if (!barsDrawn) {
        drawSolidRect(0, 0, EXT_TFT_W, DOOM_PRESENT_Y, 0, 0, 0);
        drawSolidRect(
            0,
            DOOM_PRESENT_Y + DOOM_PRESENT_H,
            EXT_TFT_W,
            EXT_TFT_H - DOOM_PRESENT_Y - DOOM_PRESENT_H,
            0,
            0,
            0
        );
        barsDrawn = true;
    }

    const uint8_t* packed = reinterpret_cast<const uint8_t*>(framebuffer);

    setAddressWindow(DOOM_PRESENT_X, DOOM_PRESENT_Y, DOOM_PRESENT_W, DOOM_PRESENT_H);

    tftSelect();
    tftDataMode();

#if DOOM_TFT_SCALE_320X200_TO_400X300_RGB565
    // Exact 3:2 vertical mapping. Eight source rows become twelve TFT rows:
    // A,A,B, C,C,D, E,E,F, G,G,H. One 9600-byte SPI call per chunk.
    static constexpr int SOURCE_ROWS_PER_CHUNK = 8;
    static constexpr int OUTPUT_ROWS_PER_CHUNK = 12;
    static constexpr size_t SOURCE_ROW_BYTES =
        static_cast<size_t>(DOOMGENERIC_RESX) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;
    static constexpr size_t OUTPUT_ROW_BYTES = static_cast<size_t>(DOOM_PRESENT_W) * 2u;
    alignas(4) static uint8_t rows[OUTPUT_ROWS_PER_CHUNK * OUTPUT_ROW_BYTES];

    for (int srcY = 0; srcY < DOOMGENERIC_RESY; srcY += SOURCE_ROWS_PER_CHUNK) {
        int outputRow = 0;

        for (int pair = 0; pair < SOURCE_ROWS_PER_CHUNK; pair += 2) {
            const uint16_t* srcA = reinterpret_cast<const uint16_t*>(
                packed + static_cast<size_t>(srcY + pair) * SOURCE_ROW_BYTES
            );
            const uint16_t* srcB = srcA + DOOMGENERIC_RESX;

            uint16_t* rowA = reinterpret_cast<uint16_t*>(
                rows + static_cast<size_t>(outputRow) * OUTPUT_ROW_BYTES
            );
            uint16_t* rowADuplicate = rowA + DOOM_PRESENT_W;
            uint16_t* rowB = rowADuplicate + DOOM_PRESENT_W;

            expandDoomRow320To400Rgb565(srcA, rowA);
            memcpy(rowADuplicate, rowA, OUTPUT_ROW_BYTES);
            expandDoomRow320To400Rgb565(srcB, rowB);

            outputRow += 3;
        }

        extSpi.writeBytes(rows, OUTPUT_ROWS_PER_CHUNK * OUTPUT_ROW_BYTES);
    }
#else
    static constexpr int CHUNK_ROWS = 4;
    alignas(4) static uint8_t rows[CHUNK_ROWS * DOOM_PRESENT_W * 2];
    const size_t rowBytes = static_cast<size_t>(DOOM_PRESENT_W) * 2u;

    for (int y = 0; y < DOOM_PRESENT_H; y += CHUNK_ROWS) {
        const int remainingRows = DOOM_PRESENT_H - y;
        const int rowsThisChunk = (remainingRows < CHUNK_ROWS) ? remainingRows : CHUNK_ROWS;

        for (int chunkY = 0; chunkY < rowsThisChunk; ++chunkY) {
            const uint16_t outY = static_cast<uint16_t>(y + chunkY);
            const uint8_t* src = packed + srcYOffsetMap[outY];
            uint16_t* row = reinterpret_cast<uint16_t*>(
                rows + static_cast<size_t>(chunkY) * rowBytes
            );

#if DOOMGENERIC_FRAMEBUFFER_RGB565_WIRE_ORDER
            const uint16_t* srcPixels = reinterpret_cast<const uint16_t*>(src);
            for (int x = 0; x < DOOM_PRESENT_W; ++x) {
                row[x] = srcPixels[srcXMap[x]];
            }
#else
            for (int x = 0; x < DOOM_PRESENT_W; ++x) {
                const uint8_t* sp = src + static_cast<size_t>(srcXMap[x]) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;
                storePackedRgb888AsRgb565(sp, reinterpret_cast<uint8_t*>(row + x));
            }
#endif
        }

        extSpi.writeBytes(rows, rowBytes * static_cast<size_t>(rowsThisChunk));
    }
#endif

    tftDeselect();
}
