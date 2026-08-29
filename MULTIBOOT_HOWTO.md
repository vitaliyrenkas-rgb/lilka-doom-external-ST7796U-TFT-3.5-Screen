# Lilka multiboot Doom — ST7796U release quick path

This project builds a multiboot binary named `doom.bin`.

**Do not flash it over KeiraOS as the main firmware.**

## Build

Open the project in VS Code / PlatformIO and run:

```bash
pio run -e v2
```

After a successful build, `move_firmware.py` copies the firmware to the project root as:

```text
doom.bin
```

## SD layout

Put `doom.bin` and a legally obtained Doom WAD in the same directory, for example:

```text
/apps/doom/doom.bin
/apps/doom/doom.wad
```

The launcher path may be different; the requirement is that `doom.bin` and `doom*.wad` are in the same folder.

Doom uses `lilka::multiboot.getFirmwarePath()` and searches for `doom*.wad` next to the launched binary.

## Launch

1. Boot normal KeiraOS.
2. Launch `doom.bin` through the multiboot / file launcher.
3. Choose display mode:
   - `VANILLA 35   400x250`
   - `QUALITY      480x300`
4. Choose sound device:
   - `I2S DAC`
   - `П'ЄЗО-ДИНАМІК`
   - `БЕЗ ЗВУКУ`
5. Play.

## Exit back to KeiraOS

Use the stock Doom menu:

```text
Quit Game → Do you wanna exit to DOS? Y/N
```

On Lilka:

- `START` = Yes / exit to KeiraOS;
- `SELECT` = No / cancel.

## External ST7796U mapping

| TFT | GPIO |
|---|---:|
| SCK | 12 |
| MOSI | 14 |
| CS | 48 |
| DC | 44 / RX |
| RST | 47 |
| MISO | unused |
| BL | external 5 V |

Final Doom bulk SPI is 80 MHz. The wider FFC/ST7796U system path has separately been validated up to 125 MHz in KeiraOS testing.

## WAD policy

WAD files are not included in this repository/package. See `WAD_ASSETS.md`.
