#include "external_ili9488.h"

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_attr.h>
#include <string.h>

extern "C" {
#include "doomgeneric.h"
}

static constexpr spi_host_device_t EXT_TFT_SPI_HOST = SPI2_HOST;

// Keep every Doom bulk transfer inside one ESP-IDF DMA descriptor.
// ESP-IDF v4.4 defines SPI_MAX_DMA_LEN as 4092 bytes.
// Exact 5:4 vertical scaling maps four source rows -> five TFT rows:
// 5 * 400 * 2 = 4000 bytes per transfer.
static constexpr int DMA_SOURCE_ROWS_PER_CHUNK = 4;
static constexpr int DMA_OUTPUT_ROWS_PER_CHUNK = 5;
static constexpr size_t DMA_OUTPUT_ROW_BYTES =
    static_cast<size_t>(DOOM_PRESENT_W) * 2u;
static constexpr size_t DMA_CHUNK_BYTES =
    static_cast<size_t>(DMA_OUTPUT_ROWS_PER_CHUNK) *
    DMA_OUTPUT_ROW_BYTES;

static spi_device_handle_t extSpiDevice = nullptr;
static bool extReady = false;
static bool extTransportOk = true;

DMA_ATTR static uint8_t fillRow[EXT_TFT_W * 2];
DMA_ATTR static uint8_t doomChunk[DMA_CHUNK_BYTES];

static bool scaleMapsReady = false;
static uint16_t srcXMap[DOOM_PRESENT_W];
static size_t srcYOffsetMap[DOOM_PRESENT_H];

static void IRAM_ATTR extTftPreTransferCallback(spi_transaction_t* transaction) {
    const bool dataMode =
        reinterpret_cast<uintptr_t>(transaction->user) != 0;
    gpio_set_level(
        static_cast<gpio_num_t>(EXT_TFT_DC),
        dataMode ? 1 : 0
    );
}

static bool addSpiDevice(uint32_t hz) {
    spi_device_interface_config_t deviceConfig = {};
    deviceConfig.mode = 0;
    deviceConfig.clock_speed_hz = hz;
    deviceConfig.spics_io_num = EXT_TFT_CS;
    deviceConfig.queue_size = 1;
    deviceConfig.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_NO_DUMMY;
    deviceConfig.pre_cb = extTftPreTransferCallback;

    return spi_bus_add_device(
        EXT_TFT_SPI_HOST,
        &deviceConfig,
        &extSpiDevice
    ) == ESP_OK;
}

static bool beginSpiBus() {
    spi_bus_config_t busConfig = {};
    busConfig.mosi_io_num = EXT_TFT_MOSI;
    busConfig.miso_io_num = -1;
    busConfig.sclk_io_num = EXT_TFT_SCK;
    busConfig.quadwp_io_num = -1;
    busConfig.quadhd_io_num = -1;
    busConfig.data4_io_num = -1;
    busConfig.data5_io_num = -1;
    busConfig.data6_io_num = -1;
    busConfig.data7_io_num = -1;
    busConfig.max_transfer_sz = DMA_CHUNK_BYTES;

    if (
        spi_bus_initialize(
            EXT_TFT_SPI_HOST,
            &busConfig,
            SPI_DMA_CH_AUTO
        ) != ESP_OK
    ) {
        return false;
    }

    return addSpiDevice(EXT_TFT_INIT_SPI_HZ);
}

static bool switchSpiDeviceClock(uint32_t hz) {
    if (extSpiDevice != nullptr) {
        if (spi_bus_remove_device(extSpiDevice) != ESP_OK) {
            return false;
        }
        extSpiDevice = nullptr;
    }

    return addSpiDevice(hz);
}

static bool transmitPollingLocked(
    const void* bytes,
    size_t len,
    bool dataMode,
    bool keepCsActive
) {
    if (
        extSpiDevice == nullptr ||
        bytes == nullptr ||
        len == 0 ||
        len > DMA_CHUNK_BYTES
    ) {
        extTransportOk = false;
        return false;
    }

    spi_transaction_t transaction = {};
    transaction.user = reinterpret_cast<void*>(dataMode ? 1U : 0U);
    transaction.length = len * 8u;

    if (keepCsActive) {
        transaction.flags |= SPI_TRANS_CS_KEEP_ACTIVE;
    }

    if (len <= sizeof(transaction.tx_data)) {
        transaction.flags |= SPI_TRANS_USE_TXDATA;
        memcpy(transaction.tx_data, bytes, len);
    } else {
        transaction.tx_buffer = bytes;
    }

    if (
        spi_device_polling_transmit(
            extSpiDevice,
            &transaction
        ) != ESP_OK
    ) {
        extTransportOk = false;
        return false;
    }

    return true;
}

static void writeCommandData(
    uint8_t command,
    const uint8_t* data,
    size_t len
) {
    if (!extTransportOk || extSpiDevice == nullptr) {
        return;
    }

    if (
        spi_device_acquire_bus(
            extSpiDevice,
            portMAX_DELAY
        ) != ESP_OK
    ) {
        extTransportOk = false;
        return;
    }

    const bool commandOk =
        transmitPollingLocked(&command, 1, false, len > 0);

    bool dataOk = true;
    if (commandOk && len > 0) {
        dataOk = transmitPollingLocked(data, len, true, false);
    }

    spi_device_release_bus(extSpiDevice);
    extTransportOk = commandOk && dataOk;
}

static void writeCommand(uint8_t command) {
    writeCommandData(command, nullptr, 0);
}

static bool writeRepeatedRows(
    const uint8_t* row,
    size_t rowBytes,
    uint16_t rowCount
) {
    if (!extTransportOk || extSpiDevice == nullptr) {
        return false;
    }

    if (
        spi_device_acquire_bus(
            extSpiDevice,
            portMAX_DELAY
        ) != ESP_OK
    ) {
        extTransportOk = false;
        return false;
    }

    const uint8_t ramWriteCommand = 0x2C;
    bool ok = transmitPollingLocked(
        &ramWriteCommand,
        1,
        false,
        rowCount > 0
    );

    for (
        uint16_t rowIndex = 0;
        ok && rowIndex < rowCount;
        ++rowIndex
    ) {
        ok = transmitPollingLocked(
            row,
            rowBytes,
            true,
            rowIndex + 1 < rowCount
        );
    }

    spi_device_release_bus(extSpiDevice);
    extTransportOk = ok;
    return ok;
}

static void setAddressWindow(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h
) {
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

static void fillRect565(
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    uint8_t r,
    uint8_t g,
    uint8_t b
) {
    if (w == 0 || h == 0) {
        return;
    }

    const uint16_t color = packRgb565(r, g, b);
    const uint8_t hi = static_cast<uint8_t>(color >> 8);
    const uint8_t lo = static_cast<uint8_t>(color & 0xff);
    const size_t rowBytes = static_cast<size_t>(w) * 2u;

    for (uint16_t px = 0; px < w; ++px) {
        fillRow[px * 2 + 0] = hi;
        fillRow[px * 2 + 1] = lo;
    }

    setAddressWindow(x, y, w, h);
    writeRepeatedRows(fillRow, rowBytes, h);
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
    pinMode(EXT_TFT_DC, OUTPUT);
    pinMode(EXT_TFT_RST, OUTPUT);
    pinMode(EXT_TFT_BL, INPUT); // BL is externally powered; never drive it.

    digitalWrite(EXT_TFT_DC, HIGH);
    digitalWrite(EXT_TFT_RST, HIGH);

    delay(50);
    digitalWrite(EXT_TFT_RST, LOW);
    delay(80);
    digitalWrite(EXT_TFT_RST, HIGH);
    delay(150);

    extTransportOk = beginSpiBus();
    if (!extTransportOk) {
        return false;
    }

    initST7796U();
    if (!extTransportOk) {
        return false;
    }

    if (!switchSpiDeviceClock(EXT_TFT_BULK_SPI_HZ)) {
        extTransportOk = false;
        return false;
    }

    extReady = true;
    externalTftClear(0, 0, 0);
    return extTransportOk;
}

void externalTftClear(uint8_t r, uint8_t g, uint8_t b) {
    if (!extReady || !extTransportOk) {
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
        srcXMap[x] =
            (static_cast<uint32_t>(x) * DOOMGENERIC_RESX) /
            DOOM_PRESENT_W;
    }

    const size_t srcStride =
        static_cast<size_t>(DOOMGENERIC_RESX) *
        DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;

    for (uint16_t y = 0; y < DOOM_PRESENT_H; ++y) {
        const uint16_t sy =
            (static_cast<uint32_t>(y) * DOOMGENERIC_RESY) /
            DOOM_PRESENT_H;
        srcYOffsetMap[y] = static_cast<size_t>(sy) * srcStride;
    }

    scaleMapsReady = true;
}

#define DOOM_TFT_SCALE_320X200_TO_400X250_RGB565 \
    (DOOMGENERIC_FRAMEBUFFER_RGB565_WIRE_ORDER && \
     DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL == 2 && \
     DOOMGENERIC_RESX == 320 && \
     DOOMGENERIC_RESY == 200 && \
     DOOM_PRESENT_W == 400 && \
     DOOM_PRESENT_H == 250)

#if DOOM_TFT_SCALE_320X200_TO_400X250_RGB565
static inline void expandDoomRow320To400Rgb565(
    const uint16_t* src,
    uint16_t* dst
) {
    // Exact nearest-neighbor 5:4 mapping:
    // four source pixels -> five TFT pixels as A,A,B,C,D.
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
    if (
        !extReady ||
        !extTransportOk ||
        framebuffer == nullptr
    ) {
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

    const uint8_t* packed =
        reinterpret_cast<const uint8_t*>(framebuffer);

    setAddressWindow(
        DOOM_PRESENT_X,
        DOOM_PRESENT_Y,
        DOOM_PRESENT_W,
        DOOM_PRESENT_H
    );
    if (!extTransportOk) {
        return;
    }

#if DOOM_TFT_SCALE_320X200_TO_400X250_RGB565
    static constexpr size_t SOURCE_ROW_BYTES =
        static_cast<size_t>(DOOMGENERIC_RESX) *
        DOOMGENERIC_FRAMEBUFFER_BYTES_PER_PIXEL;

    if (
        spi_device_acquire_bus(
            extSpiDevice,
            portMAX_DELAY
        ) != ESP_OK
    ) {
        extTransportOk = false;
        return;
    }

    const uint8_t ramWriteCommand = 0x2C;
    bool ok = transmitPollingLocked(
        &ramWriteCommand,
        1,
        false,
        true
    );

    for (
        int srcY = 0;
        ok && srcY < DOOMGENERIC_RESY;
        srcY += DMA_SOURCE_ROWS_PER_CHUNK
    ) {
        const uint16_t* srcA =
            reinterpret_cast<const uint16_t*>(
                packed +
                static_cast<size_t>(srcY) *
                SOURCE_ROW_BYTES
            );
        const uint16_t* srcB =
            srcA + DOOMGENERIC_RESX;
        const uint16_t* srcC =
            srcB + DOOMGENERIC_RESX;
        const uint16_t* srcD =
            srcC + DOOMGENERIC_RESX;

        uint16_t* rowA =
            reinterpret_cast<uint16_t*>(doomChunk);
        uint16_t* rowADuplicate =
            rowA + DOOM_PRESENT_W;
        uint16_t* rowB =
            rowADuplicate + DOOM_PRESENT_W;
        uint16_t* rowC =
            rowB + DOOM_PRESENT_W;
        uint16_t* rowD =
            rowC + DOOM_PRESENT_W;

        expandDoomRow320To400Rgb565(srcA, rowA);
        memcpy(
            rowADuplicate,
            rowA,
            DMA_OUTPUT_ROW_BYTES
        );
        expandDoomRow320To400Rgb565(srcB, rowB);
        expandDoomRow320To400Rgb565(srcC, rowC);
        expandDoomRow320To400Rgb565(srcD, rowD);

        const bool isLastChunk =
            srcY + DMA_SOURCE_ROWS_PER_CHUNK >=
            DOOMGENERIC_RESY;

        ok = transmitPollingLocked(
            doomChunk,
            DMA_CHUNK_BYTES,
            true,
            !isLastChunk
        );
    }

    spi_device_release_bus(extSpiDevice);
    extTransportOk = ok;
#else
    extTransportOk = false;
#endif
}
