#include "external_ili9488.h"

#include <SPI.h>

extern "C" {
#include "doomgeneric.h"
}

static SPIClass extSpi(FSPI);
static bool extReady = false;
static bool extTransactionActive = false;

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

static inline uint16_t packRgb565(uint8_t r, uint8_t g, uint8_t b) {
#if EXT_TFT_SWAP_RB
    const uint8_t tmp = r;
    r = b;
    b = tmp;
#endif
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline uint16_t doomPixelToRgb565(uint32_t pixel) {
    const uint8_t r = static_cast<uint8_t>((pixel >> 16) & 0xff);
    const uint8_t g = static_cast<uint8_t>((pixel >> 8) & 0xff);
    const uint8_t b = static_cast<uint8_t>(pixel & 0xff);
    return packRgb565(r, g, b);
}

static inline void storeRgb565(uint8_t* out, uint16_t color) {
    out[0] = static_cast<uint8_t>(color >> 8);
    out[1] = static_cast<uint8_t>(color & 0xff);
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

static void fillRect565(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t r, uint8_t g, uint8_t b) {
    if (w == 0 || h == 0) {
        return;
    }

    static uint8_t row[EXT_TFT_W * 2];
    const uint16_t color = packRgb565(r, g, b);
    const size_t rowBytes = static_cast<size_t>(w) * 2;

    for (uint16_t px = 0; px < w; ++px) {
        storeRgb565(row + px * 2, color);
    }

    setAddressWindow(x, y, w, h);
    tftSelect();
    tftDataMode();
    for (uint16_t py = 0; py < h; ++py) {
        extSpi.writeBytes(row, rowBytes);
    }
    tftDeselect();
}

bool externalTftBegin() {
    Serial.println("[DOOM FFC #003] ST7796U init start");
    Serial.printf(
        "[DOOM FFC #003] pins CS=%d RST=%d DC=%d MOSI=%d SCK=%d BL=external/untouched\n",
        EXT_TFT_CS,
        EXT_TFT_RST,
        EXT_TFT_DC,
        EXT_TFT_MOSI,
        EXT_TFT_SCK
    );
    Serial.printf(
        "[DOOM FFC #003] init SPI=%lu bulk SPI=%lu MODE0 RGB565 MADCTL=0xE8 inversion=%d swapRB=%d present=%dx%d+%d,%d chunkRows=%d colorTest=0 fastScale=3:2\n",
        static_cast<unsigned long>(EXT_TFT_INIT_SPI_HZ),
        static_cast<unsigned long>(EXT_TFT_BULK_SPI_HZ),
        EXT_TFT_ENABLE_INVERSION,
        EXT_TFT_SWAP_RB,
        DOOM_PRESENT_W,
        DOOM_PRESENT_H,
        DOOM_PRESENT_X,
        DOOM_PRESENT_Y,
        EXT_TFT_CHUNK_ROWS
    );

    pinMode(EXT_TFT_CS, OUTPUT);
    pinMode(EXT_TFT_DC, OUTPUT);
    pinMode(EXT_TFT_RST, OUTPUT);
    pinMode(21, INPUT); // ST7796U BL is externally powered; do not drive it.

    digitalWrite(EXT_TFT_CS, HIGH);
    digitalWrite(EXT_TFT_DC, HIGH);

    digitalWrite(EXT_TFT_RST, LOW);
    delay(30);
    digitalWrite(EXT_TFT_RST, HIGH);
    delay(150);

    extSpi.begin(EXT_TFT_SCK, -1, EXT_TFT_MOSI, EXT_TFT_CS);
    beginSpiTransaction(EXT_TFT_INIT_SPI_HZ);

    writeCommand(0x01); // Software reset
    delay(120);
    writeCommand(0x11); // Sleep out
    delay(120);

    // ST7796U RGB565: 2 bytes/pixel. This is the key difference from the old
    // ILI9488/RGB666 path, which pushed 3 bytes/pixel and leaves the FFC panel black.
    const uint8_t pixfmt[] = {0x55};
    writeCommandData(0x3A, pixfmt, sizeof(pixfmt));

    // Validated KeiraOS FFC orientation for ST7796U.
    const uint8_t madctl[] = {0xE8};
    writeCommandData(0x36, madctl, sizeof(madctl));

#if EXT_TFT_ENABLE_INVERSION
    writeCommand(0x21); // INVON: required by this ST7796U IPS panel for normal colors.
#else
    writeCommand(0x20); // INVOFF: fallback if a panel revision does not need inversion.
#endif

    writeCommand(0x29); // Display on
    delay(80);

    beginSpiTransaction(EXT_TFT_BULK_SPI_HZ);
    extReady = true;

    // #003: color test removed. #002 already proved init, RGB565, inversion and frame path.
    externalTftClear(0, 0, 0);

    Serial.println("[DOOM FFC #003] ST7796U init done");
    return true;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady) {
        return;
    }
    fillRect565(0, 0, EXT_TFT_W, EXT_TFT_H, r, g, b);
}

static void presentGenericScaled(const uint32_t* framebuffer) {
    static uint8_t chunk[DOOM_PRESENT_W * 2 * EXT_TFT_CHUNK_ROWS];

    setAddressWindow(DOOM_PRESENT_X, DOOM_PRESENT_Y, DOOM_PRESENT_W, DOOM_PRESENT_H);
    tftSelect();
    tftDataMode();

    for (int y0 = 0; y0 < DOOM_PRESENT_H; y0 += EXT_TFT_CHUNK_ROWS) {
        int rows = EXT_TFT_CHUNK_ROWS;
        if (y0 + rows > DOOM_PRESENT_H) {
            rows = DOOM_PRESENT_H - y0;
        }

        for (int yy = 0; yy < rows; ++yy) {
            const int y = y0 + yy;
            const int sy = (y * DOOMGENERIC_RESY) / DOOM_PRESENT_H;
            uint8_t* out = chunk + static_cast<size_t>(yy) * DOOM_PRESENT_W * 2;

            for (int x = 0; x < DOOM_PRESENT_W; ++x) {
                const int sx = (x * DOOMGENERIC_RESX) / DOOM_PRESENT_W;
                const uint16_t color = doomPixelToRgb565(framebuffer[sy * DOOMGENERIC_RESX + sx]);
                storeRgb565(out + x * 2, color);
            }
        }

        extSpi.writeBytes(chunk, static_cast<size_t>(DOOM_PRESENT_W) * 2 * rows);
    }

    tftDeselect();
}

static void presentFast480x300_3to2(const uint32_t* framebuffer) {
    static uint8_t chunk[DOOM_PRESENT_W * 2 * EXT_TFT_CHUNK_ROWS];
    int chunkRows = 0;

    setAddressWindow(DOOM_PRESENT_X, DOOM_PRESENT_Y, DOOM_PRESENT_W, DOOM_PRESENT_H);
    tftSelect();
    tftDataMode();

    auto flushChunk = [&]() {
        if (chunkRows > 0) {
            extSpi.writeBytes(chunk, static_cast<size_t>(DOOM_PRESENT_W) * 2 * chunkRows);
            chunkRows = 0;
        }
    };

    auto appendScaledRow = [&](const uint32_t* srcRow) {
        uint8_t* out = chunk + static_cast<size_t>(chunkRows) * DOOM_PRESENT_W * 2;
        size_t oi = 0;

        // Exact 320 -> 480 nearest-neighbor scale without division:
        // two source pixels become three output pixels: A, A, B.
        for (int sx = 0; sx < DOOMGENERIC_RESX; sx += 2) {
            const uint16_t c0 = doomPixelToRgb565(srcRow[sx]);
            storeRgb565(out + oi, c0);
            oi += 2;
            storeRgb565(out + oi, c0);
            oi += 2;

            const uint16_t c1 = doomPixelToRgb565(srcRow[sx + 1]);
            storeRgb565(out + oi, c1);
            oi += 2;
        }

        ++chunkRows;
        if (chunkRows >= EXT_TFT_CHUNK_ROWS) {
            flushChunk();
        }
    };

    // Exact 200 -> 300 vertical scale without division:
    // even source rows are written twice, odd source rows once: A, A, B.
    for (int sy = 0; sy < DOOMGENERIC_RESY; ++sy) {
        const uint32_t* srcRow = framebuffer + static_cast<size_t>(sy) * DOOMGENERIC_RESX;
        appendScaledRow(srcRow);
        if ((sy & 1) == 0) {
            appendScaledRow(srcRow);
        }
    }

    flushChunk();
    tftDeselect();
}

void externalTftPresentDoomFrame(const uint32_t* framebuffer) {
    if (!extReady || framebuffer == nullptr) {
        return;
    }

    static bool firstFrame = true;
    static bool barsDrawn = false;
    if (firstFrame) {
        Serial.println("[DOOM FFC #003] first fast 480-wide game frame begin");
    }

    if (!barsDrawn) {
        if (DOOM_PRESENT_Y > 0) {
            fillRect565(0, 0, EXT_TFT_W, DOOM_PRESENT_Y, 0, 0, 0);
        }
        const uint16_t bottomY = DOOM_PRESENT_Y + DOOM_PRESENT_H;
        if (bottomY < EXT_TFT_H) {
            fillRect565(0, bottomY, EXT_TFT_W, EXT_TFT_H - bottomY, 0, 0, 0);
        }
        barsDrawn = true;
    }

#if (DOOM_PRESENT_W == 480) && (DOOM_PRESENT_H == 300)
    if (DOOMGENERIC_RESX == 320 && DOOMGENERIC_RESY == 200) {
        presentFast480x300_3to2(framebuffer);
    } else {
        presentGenericScaled(framebuffer);
    }
#else
    presentGenericScaled(framebuffer);
#endif

    if (firstFrame) {
        firstFrame = false;
        Serial.println("[DOOM FFC #003] first fast 480-wide game frame done");
    }
}
