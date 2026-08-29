# Release technical canon — Doom / Lilka / ST7796U

This file intentionally separates **verified release facts** from historical experiments.

## Hardware

- MCU: ESP32-S3 N16R8.
- Best verified device: Lilka v2 by Anderson.
- External display: 3.5″ 480×320 ST7796U/ST7796S.
- Interface: 14-pin FFC display path.
- FFC/ST7796U system path has been physically validated up to 125 MHz in KeiraOS testing.
- **Final Doom release bulk SPI: 80 MHz.**
- Init SPI: 15 MHz.
- SPI host: `SPI2_HOST`.
- SPI mode: MODE0.
- DMA: `SPI_DMA_CH_AUTO`.
- Transfer API: `spi_device_polling_transmit()`.

## Wiring in final Doom source tree

| Signal | GPIO |
|---|---:|
| SCK | 12 |
| MOSI | 14 |
| CS | 48 |
| DC | 44 / RX |
| RST | 47 |
| MISO | unused |
| BL | externally powered 5 V; firmware leaves GPIO21 as input |

## Native framebuffer

- Doom framebuffer: 320×200.
- Packed wire-order RGB565 path.

## VANILLA 35

- Output: 400×250.
- Position: `x=40, y=35`.
- Exact 5:4 nearest-neighbor expansion.
- Horizontal mapping: 4 source pixels → 5 output pixels.
- Vertical chunk mapping: 4 source rows → 5 output rows.
- DMA chunk: 4000 bytes.
- 50 DMA transfers/frame.
- Physical result: 33–35 FPS.

## QUALITY

- Output: 480×300.
- Position: `x=0, y=10`.
- Exact 3:2 nearest-neighbor expansion.
- Horizontal mapping: 2 source pixels → 3 output pixels.
- Vertical chunk mapping: 2 source rows → 3 output rows.
- DMA chunk: 2880 bytes.
- Physical result: 25–26 FPS.

## Startup UI

- Native `lilka::Menu`.
- Rendered into `lilka::Canvas(400, 240)`.
- Presented to external TFT through `externalTftPresentUiCanvas()`.

## Sound options

- I2S DAC.
- Piezo speaker.
- No sound.

## Input

- D-pad → Doom arrows.
- A → FIRE.
- B → USE.
- C → TAB / automap.
- D → next weapon.
- SELECT → ESC.
- START → ENTER.
- Lilka v2 SELECT / GPIO0 is explicitly restored at runtime with `INPUT_PULLUP`.

## Native quit mapping

Only inside the stock Doom quit-confirmation context:

- `KEY_ENTER` / START → `key_menu_confirm` (`y`).
- `KEY_ESCAPE` / SELECT → `key_menu_abort` (`n`).

Global gameplay mappings remain unchanged.

## KeiraOS return backend

- Find `ESP_PARTITION_SUBTYPE_APP_OTA_0`.
- `esp_ota_set_boot_partition(...)`.
- `esp_restart()`.

## Historical file name

`src/external_ili9488.*` is a historical name. It is intentionally preserved in the release source to avoid a non-functional refactor immediately before release. The current backend is ST7796U.

## Rejected experiment

480×250 VANILLA WIDE:

- ~29 FPS;
- transport stable;
- visually stretched on X;
- **REJECTED / DO NOT DEVELOP FURTHER**.

## Release philosophy

- Stability > cosmetics.
- One-variable tests.
- No PASS without physical Lilka.
- Do not resurrect async/ISR/black-screen transport paths without a fundamental reason.
