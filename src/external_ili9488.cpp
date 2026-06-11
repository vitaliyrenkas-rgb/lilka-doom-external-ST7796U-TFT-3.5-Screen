#include "external_ili9488.h"

#include <SPI.h>

extern "C" {
#include "doomgeneric.h"
}

static SPIClass extSpi(FSPI);
static bool extReady = false;
static bool scaleMapsReady = false;

static uint16_t srcXMap[DOOM_PRESENT_W];
static uint16_t srcYMap[DOOM_PRESENT_H];

#if DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB565
static constexpr uint8_t TFT_COLMOD = 0x55;
static constexpr size_t TFT_BYTES_PER_PIXEL = 2;
#elif DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB666
static constexpr uint8_t TFT_COLMOD = 0x66;
static constexpr size_t TFT_BYTES_PER_PIXEL = 3;
#else
#error "Unknown DOOM_TFT_PIXFMT"
#endif

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

static void buildScaleMaps() {
    if (scaleMapsReady) {
        return;
    }

    for (uint16_t x = 0; x < DOOM_PRESENT_W; ++x) {
        srcXMap[x] = (static_cast<uint32_t>(x) * DOOMGENERIC_RESX) / DOOM_PRESENT_W;
    }

    for (uint16_t y = 0; y < DOOM_PRESENT_H; ++y) {
        srcYMap[y] = (static_cast<uint32_t>(y) * DOOMGENERIC_RESY) / DOOM_PRESENT_H;
    }

    scaleMapsReady = true;
}

static inline void encodePixel(uint8_t* out, uint8_t r, uint8_t g, uint8_t b) {
#if DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB565
    const uint16_t rgb565 = (static_cast<uint16_t>(r & 0xf8) << 8) |
                            (static_cast<uint16_t>(g & 0xfc) << 3) |
                            static_cast<uint16_t>(b >> 3);
    out[0] = static_cast<uint8_t>(rgb565 >> 8);
    out[1] = static_cast<uint8_t>(rgb565 & 0xff);
#else
    out[0] = r;
    out[1] = g;
    out[2] = b;
#endif
}

static inline void encodePixelFromPackedRgb888(uint8_t* out, const uint8_t* pixel) {
#if DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB565
    const uint8_t r = pixel[0];
    const uint8_t g = pixel[1];
    const uint8_t b = pixel[2];
    const uint16_t rgb565 = (static_cast<uint16_t>(r & 0xf8) << 8) |
                            (static_cast<uint16_t>(g & 0xfc) << 3) |
                            static_cast<uint16_t>(b >> 3);
    out[0] = static_cast<uint8_t>(rgb565 >> 8);
    out[1] = static_cast<uint8_t>(rgb565 & 0xff);
#else
    out[0] = pixel[0];
    out[1] = pixel[1];
    out[2] = pixel[2];
#endif
}

#define DOOM_TFT_NATIVE_DIRECT_RGB666 \
    (DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB666 && \
     DOOMGENERIC_FRAMEBUFFER_RGB888_PACKED && \
     DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL == 3 && \
     DOOM_PRESENT_W == DOOMGENERIC_RESX && \
     DOOM_PRESENT_H == DOOMGENERIC_RESY)

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

    extSpi.begin(EXT_TFT_SCK, -1, EXT_TFT_MOSI, EXT_TFT_CS);
    extSpi.beginTransaction(SPISettings(60000000, MSBFIRST, SPI_MODE0));

    writeCommand(0x01); // Software reset
    delay(120);
    writeCommand(0x11); // Sleep out
    delay(120);

    const uint8_t pixfmt[] = {TFT_COLMOD};
    writeCommandData(0x3A, pixfmt, sizeof(pixfmt)); // COLMOD / Interface Pixel Format

    // Landscape 480x320. This matched the KeiraOS external TFT milestone.
    const uint8_t madctl[] = {0xE8};
    writeCommandData(0x36, madctl, sizeof(madctl));

    writeCommand(0x20); // Display inversion off
    writeCommand(0x29); // Display on
    delay(50);

    buildScaleMaps();

    extReady = true;
    Serial.printf(
        "[DOOM TFT] profile=%d area=%dx%d+%d+%d colmod=0x%02x bpp=%u srcBpp=%u direct=%u\n",
        DOOM_TFT_PROFILE,
        DOOM_PRESENT_W,
        DOOM_PRESENT_H,
        DOOM_PRESENT_X,
        DOOM_PRESENT_Y,
        TFT_COLMOD,
        static_cast<unsigned>(TFT_BYTES_PER_PIXEL),
        static_cast<unsigned>(DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL),
        static_cast<unsigned>(DOOM_TFT_NATIVE_DIRECT_RGB666)
    );
    externalTftClear(0, 0, 0);
    return true;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady) {
        return;
    }

    static uint8_t row[EXT_TFT_W * TFT_BYTES_PER_PIXEL];
    for (int x = 0; x < EXT_TFT_W; ++x) {
        encodePixel(&row[x * TFT_BYTES_PER_PIXEL], r, g, b);
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
    if (w == 0 || h == 0) {
        return;
    }

    static uint8_t row[EXT_TFT_W * TFT_BYTES_PER_PIXEL];
    const size_t bytes = static_cast<size_t>(w) * TFT_BYTES_PER_PIXEL;
    for (uint16_t px = 0; px < w; ++px) {
        encodePixel(&row[px * TFT_BYTES_PER_PIXEL], r, g, b);
    }

    setAddressWindow(x, y, w, h);
    tftSelect();
    tftDataMode();
    for (uint16_t py = 0; py < h; ++py) {
        extSpi.writeBytes(row, bytes);
    }
    tftDeselect();
}

static void drawStaticBordersOnce() {
    static bool bordersDrawn = false;
    if (bordersDrawn) {
        return;
    }

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
    drawSolidRect(0, DOOM_PRESENT_Y, DOOM_PRESENT_X, DOOM_PRESENT_H, 0, 0, 0);
    drawSolidRect(
        DOOM_PRESENT_X + DOOM_PRESENT_W,
        DOOM_PRESENT_Y,
        EXT_TFT_W - DOOM_PRESENT_X - DOOM_PRESENT_W,
        DOOM_PRESENT_H,
        0,
        0,
        0
    );

    bordersDrawn = true;
}

void externalTftPresentDoomFrame(const uint32_t* framebuffer) {
    if (!extReady || framebuffer == nullptr) {
        return;
    }

    buildScaleMaps();
    drawStaticBordersOnce();

    const uint8_t* packed = reinterpret_cast<const uint8_t*>(framebuffer);
    const size_t srcStride = static_cast<size_t>(DOOMGENERIC_RESX) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;

    setAddressWindow(DOOM_PRESENT_X, DOOM_PRESENT_Y, DOOM_PRESENT_W, DOOM_PRESENT_H);

    tftSelect();
    tftDataMode();

#if DOOM_TFT_NATIVE_DIRECT_RGB666
    // FAST_320 + RGB666: DG_ScreenBuffer is already packed R,G,B bytes.
    // Stream each native Doom row directly to SPI; no row repack, no 32-bit reads.
    for (int y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint8_t* src = packed + (static_cast<size_t>(srcYMap[y]) * srcStride);
        extSpi.writeBytes(const_cast<uint8_t*>(src), DOOM_PRESENT_W * TFT_BYTES_PER_PIXEL);
    }
#else
    static uint8_t row[DOOM_PRESENT_W * TFT_BYTES_PER_PIXEL];
    for (int y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint8_t* src = packed + (static_cast<size_t>(srcYMap[y]) * srcStride);
#if (DOOM_TFT_PIXFMT == DOOM_TFT_PIXFMT_RGB666) && (DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL == 3)
        for (int x = 0; x < DOOM_PRESENT_W; ++x) {
            const uint8_t* sp = src + (static_cast<size_t>(srcXMap[x]) * 3u);
            uint8_t* dp = &row[x * 3u];

            dp[0] = sp[0];
            dp[1] = sp[1];
            dp[2] = sp[2];
        }
#else
        for (int x = 0; x < DOOM_PRESENT_W; ++x) {
            encodePixelFromPackedRgb888(
                &row[x * TFT_BYTES_PER_PIXEL],
                src + (static_cast<size_t>(srcXMap[x]) * DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL)
            );
        }
#endif
        extSpi.writeBytes(row, sizeof(row));
    }
#endif

    tftDeselect();
}
