# Lilka multiboot Doom — quick path

This project builds a multiboot binary named `doom.bin`. Do not upload it over KeiraOS as the main firmware.

1. Open this folder in VSCode / PlatformIO.
2. Run PlatformIO Build.
3. After build, `move_firmware.py` copies the firmware to the project root as `doom.bin`.
4. Copy `doom.bin` to the SD card, for example:

   `/apps/doom/doom.bin`

5. Put a legal WAD in the same folder, for example:

   `/apps/doom/doom.wad`

6. Boot normal KeiraOS and launch Doom through multiboot / launcher.

Expected serial markers after Doom starts:

`[DOOM BOOT]`
`[DOOM TFT]`
`[DOOM MULTIBOOT]`
`[DOOM WAD]`
`[DOOM CREATE]`
`[DOOM RUN]`

External TFT mapping used here:

SCK  -> GPIO12
MOSI -> GPIO14
CS   -> GPIO48
DC   -> RX
RST  -> TX
MISO -> not connected

External output geometry: Doom 320x200 -> ILI9488 480x300 with 10 px black bars top/bottom.
Rotation/MADCTL currently set to `0x88` after the first upside-down visual test.
