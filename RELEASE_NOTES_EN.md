# Doom for Lilka / ST7796U 3.5″ — v1.0.0
## Doom Guy Leaves the Lab

<p align="center">
  <img src="docs/images/hero-title.jpg" alt="The Ultimate Doom on Lilka" width="900">
</p>

It started with one very simple thought:

> **“I don’t want to squint at a tiny screen.”**

It ended with a complete Doom experience on Lilka v2 and an external 3.5″ ST7796U: two genuinely useful display modes, polling DMA, sound, saves, standalone battery operation, and a clean native exit back to KeiraOS.

This is no longer a proof of concept.

This is the release.

---

## What is in the release

### VANILLA 35 — 400×250

- centered at `x=40, y=35`;
- exact 5:4 expansion from native 320×200;
- polling DMA;
- 4000 B chunk;
- **33–35 FPS** sustained;
- gameplay / save / load / sound — PASS;
- battery/no-USB — PASS;
- boot without Serial Monitor — PASS.

<p align="center">
  <img src="docs/images/telemetry-33fps.jpg" alt="33 FPS telemetry" width="560">
</p>

### QUALITY — 480×300

- full 480-pixel width;
- `x=0, y=10` on the physical 480×320 panel;
- exact 3:2 expansion;
- polling DMA;
- 2880 B chunk;
- **25–26 FPS**;
- gameplay / save / load / sound — PASS.

<p align="center">
  <img src="docs/images/telemetry-25fps.jpg" alt="25 FPS telemetry" width="560">
</p>

The difference is visible in real use, so both modes stay. There is no fake “one best mode” claim here.

---

## Native Lilka startup UI

Display and sound selection use the native `lilka::Menu` directly on the external ST7796U.

<p align="center">
  <img src="docs/images/display-mode-vanilla.jpg" alt="Display mode selection" width="820">
</p>

<p align="center">
  <img src="docs/images/sound-device.jpg" alt="Sound device selection" width="820">
</p>

Sound options:

- I2S DAC;
- Piezo speaker;
- No sound.

We deliberately stopped polishing the startup menu once it looked native and worked reliably. Users spend seconds there; stability matters more than ornamental churn.

---

## Polling DMA — the demon-cutting tool

The biggest technical result of this project is not just the final FPS number. It is the transport path we found.

More complicated async/queue/ISR experiments did not deliver what the simpler **polling-DMA** path finally gave us:

- predictability;
- stability;
- no black-screen architecture;
- a clearly visible performance gain;
- simpler code that can actually be reasoned about and reused.

Sometimes the simpler tool cuts the demon better.

---

## What we deliberately rejected

### 480×250 VANILLA WIDE

It ran at roughly **29 FPS** and the transport was stable, but the image was visibly stretched on the X axis.

Verdict:

**REJECTED / DO NOT DEVELOP FURTHER.**

Transport PASS is not the same thing as product PASS.

---

## Save / Load

Saving works, loading works, and both survived final regression.

<p align="center">
  <img src="docs/images/load-game.jpg" alt="LILKA SAVE slots" width="820">
</p>

---

## Exit From Hell

The final boss was almost comically symbolic.

Doom already had its native:

`Quit Game` → `Do you wanna exit to DOS? Y/N`

So we did not punch another hole through the wall. We simply put a proper handle on the existing door:

- **START = Y / exit to KeiraOS**;
- **SELECT = N / cancel**.

<p align="center">
  <img src="docs/images/quit-confirmation.jpg" alt="Doom quit confirmation" width="820">
</p>

START follows the already-validated OTA0 KeiraOS reboot backend and returns cleanly to the OS.

No RESET button required.

---

## System requirements — VERIFIED

- 3.5″ 480×320 ST7796U/ST7796S;
- 14-pin FFC;
- ESP32-S3 N16R8;
- best verified experience: Lilka v2 by Anderson;
- SD card;
- legal Doom WAD;
- a small but healthy amount of technical stubbornness.

**Clock note:** the FFC/ST7796U path was physically validated up to **125 MHz** in the KeiraOS system bench. The final Doom release itself runs **80 MHz bulk SPI**, which is its validated configuration.

---

## Final physical regression

PASS:

- VANILLA 35;
- QUALITY;
- sound;
- save;
- load;
- menus;
- startup UI;
- Quit cancel;
- Quit confirm;
- KeiraOS reboot;
- relaunch Doom;
- no Serial Monitor;
- battery / no USB.

At some point we stopped testing Doom and noticed we were simply playing it.

**That was the real release criterion.**

---

## How We Raised This Demon

Full development story with photos: **[docs/HOW_IT_WAS_EN.md](docs/HOW_IT_WAS_EN.md)**

<p align="center">
  <img src="docs/images/gameplay-clean.jpg" alt="Finished Doom gameplay" width="860">
</p>

---


## Credits / Upstream

This release stands on work that came before it:

- **[@And3rson](https://github.com/and3rson)** — Lilka and the Lilka ecosystem;
- **[Lilka v2 documentation](https://docs.lilka.dev/uk/latest/)**;
- **[lilka-dev/doom_port](https://github.com/lilka-dev/doom_port)** — original Lilka Doom proof-of-concept / porting work;
- **[@ozkl](https://github.com/ozkl)** — upstream **[DoomGeneric](https://github.com/ozkl/doomgeneric)**.

Full attribution: **[CREDITS.md](CREDITS.md)**.

## WAD / copyright

WAD files are not part of this release. Do not commit or redistribute commercial game data with this repository.

This is an unofficial port and is not affiliated with id Software, Bethesda, or Microsoft.

---

**Doom Guy discharged from Keira’s laboratory.**
**Condition: stable.**
**Demons: still a problem.**
