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

static inline uint16_t packRgb565Raw(uint8_t r, uint8_t g, uint8_t b) {
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

static inline uint16_t packTft565(uint8_t r, uint8_t g, uint8_t b) {
#if EXT_TFT_SWAP_RB
    return packRgb565Raw(b, g, r);
#else
    return packRgb565Raw(r, g, b);
#endif
}

static inline uint16_t doomPixelToRgb565(uint32_t pixel) {
    const uint8_t r = static_cast<uint8_t>((pixel >> 16) & 0xff);
    const uint8_t g = static_cast<uint8_t>((pixel >> 8) & 0xff);
    const uint8_t b = static_cast<uint8_t>(pixel & 0xff);
    return packTft565(r, g, b);
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
    const uint16_t color = packTft565(r, g, b);
    const uint8_t hi = static_cast<uint8_t>(color >> 8);
    const uint8_t lo = static_cast<uint8_t>(color & 0xff);
    const size_t rowBytes = static_cast<size_t>(w) * 2;

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

static void externalTftColorTest() {
    Serial.println("[DOOM FFC #002] color test begin: expected RED/GREEN/BLUE/WHITE/BLACK left-to-right");

    const uint16_t bandW = EXT_TFT_W / 5;
    fillRect565(0 * bandW, 0, bandW, EXT_TFT_H, 255, 0, 0);
    fillRect565(1 * bandW, 0, bandW, EXT_TFT_H, 0, 255, 0);
    fillRect565(2 * bandW, 0, bandW, EXT_TFT_H, 0, 0, 255);
    fillRect565(3 * bandW, 0, bandW, EXT_TFT_H, 255, 255, 255);
    fillRect565(4 * bandW, 0, EXT_TFT_W - 4 * bandW, EXT_TFT_H, 0, 0, 0);

    // Corner markers make rotation/MADCTL mistakes obvious without a full test app.
    fillRect565(0, 0, 24, 24, 255, 255, 255);
    fillRect565(EXT_TFT_W - 24, 0, 24, 24, 255, 255, 0);
    fillRect565(0, EXT_TFT_H - 24, 24, 24, 0, 255, 255);
    fillRect565(EXT_TFT_W - 24, EXT_TFT_H - 24, 24, 24, 255, 0, 255);

    delay(350);
    Serial.println("[DOOM FFC #002] color test done");
}

bool externalTftBegin() {
    Serial.println("[DOOM FFC #002] ST7796U init start");
    Serial.printf(
        "[DOOM FFC #002] pins CS=%d RST=%d DC=%d MOSI=%d SCK=%d BL=external/untouched\n",
        EXT_TFT_CS,
        EXT_TFT_RST,
        EXT_TFT_DC,
        EXT_TFT_MOSI,
        EXT_TFT_SCK
    );
    Serial.printf(
        "[DOOM FFC #002] init SPI=%lu bulk SPI=%lu MODE0 RGB565 MADCTL=0xE8 inversion=%d swapRB=%d present=%dx%d+%d,%d\n",
        static_cast<unsigned long>(EXT_TFT_INIT_SPI_HZ),
        static_cast<unsigned long>(EXT_TFT_BULK_SPI_HZ),
        EXT_TFT_ENABLE_INVERSION,
        EXT_TFT_SWAP_RB,
        DOOM_PRESENT_W,
        DOOM_PRESENT_H,
        DOOM_PRESENT_X,
        DOOM_PRESENT_Y
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
    writeCommand(0x21); // INVON: required by many ST7796 IPS panels for normal colors.
#else
    writeCommand(0x20); // INVOFF: fallback if a panel revision does not need inversion.
#endif

    writeCommand(0x29); // Display on
    delay(80);

    beginSpiTransaction(EXT_TFT_BULK_SPI_HZ);
    extReady = true;

    externalTftColorTest();
    externalTftClear(0, 0, 0);

    Serial.println("[DOOM FFC #002] ST7796U init done");
    return true;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady) {
        return;
    }
    fillRect565(0, 0, EXT_TFT_W, EXT_TFT_H, r, g, b);
}

void externalTftPresentDoomFrame(const uint32_t* framebuffer) {
    if (!extReady || framebuffer == nullptr) {
        return;
    }

    static bool firstFrame = true;
    static bool barsDrawn = false;
    if (firstFrame) {
        Serial.println("[DOOM FFC #002] first 480-wide game frame begin");
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
                const uint32_t pixel = framebuffer[sy * DOOMGENERIC_RESX + sx];
                const uint16_t color = doomPixelToRgb565(pixel);
                out[x * 2 + 0] = static_cast<uint8_t>(color >> 8);
                out[x * 2 + 1] = static_cast<uint8_t>(color & 0xff);
            }
        }

        extSpi.writeBytes(chunk, static_cast<size_t>(DOOM_PRESENT_W) * 2 * rows);
    }
    tftDeselect();

    if (firstFrame) {
        firstFrame = false;
        Serial.println("[DOOM FFC #002] first 480-wide game frame done");
    }
}
