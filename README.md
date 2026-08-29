<p align="center">
  <img src="docs/images/hero-title.jpg" alt="Doom running on Lilka v2 with external ST7796U 3.5-inch TFT" width="900">
</p>

# Doom for Lilka — ST7796U 3.5″

<p align="center"><strong>From “I don’t want to squint at a tiny screen” to VANILLA 35.</strong></p>

<p align="center">
  <a href="README_UA.md">Українська</a> ·
  <a href="README_EN.md">English</a> ·
  <a href="RELEASE_NOTES_UA.md">Release notes UA</a> ·
  <a href="RELEASE_NOTES_EN.md">Release notes EN</a> ·
  <a href="docs/GALLERY_UA.md">Галерея</a>
</p>

This repository contains a Lilka v2 / ESP32-S3 multiboot Doom build with output to an external **3.5″ 480×320 ST7796U TFT** over FFC.

| Mode | Output | Physical result |
|---|---:|---:|
| **VANILLA 35** | 400×250 | **33–35 FPS** |
| **QUALITY** | 480×300 | **25–26 FPS** |

Both modes use the final **polling-DMA** display transport and have passed physical regression on Lilka v2.

### Verified release behavior

- native Lilka startup menu on the external TFT;
- display-mode selection;
- sound selection: **I2S DAC / piezo / no sound**;
- gameplay, save and load;
- standalone operation without Serial Monitor;
- battery operation without USB;
- native Doom `Quit` confirmation;
- **START = Yes**, **SELECT = No**;
- clean return to KeiraOS and relaunch.

> **Important clock note:** the FFC/ST7796U hardware path has been validated at up to **125 MHz** at the KeiraOS system level. The final Doom release source itself uses **80 MHz bulk SPI**, which is the physically validated Doom configuration.

For the complete documentation, use [README_UA.md](README_UA.md) or [README_EN.md](README_EN.md).

> Doom WAD files are **not included**. Use legally obtained game data and keep it out of the repository. See [WAD_ASSETS.md](WAD_ASSETS.md).


## Upstream / Credits

Built on [Lilka v2](https://docs.lilka.dev/uk/latest/) by [@And3rson](https://github.com/and3rson), [lilka-dev/doom_port](https://github.com/lilka-dev/doom_port), and [DoomGeneric](https://github.com/ozkl/doomgeneric) by [@ozkl](https://github.com/ozkl). See [CREDITS.md](CREDITS.md).
