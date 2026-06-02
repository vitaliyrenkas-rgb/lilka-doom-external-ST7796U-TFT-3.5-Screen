# Doom для Lilka external TFT / multiboot

Це пакет Doom для запуску на Lilka через SD multiboot як `doom.bin`, з виводом кадру на зовнішній ILI9488 3.5'' TFT 480x320.

## Важливо

Цей проєкт **не треба прошивати поверх KeiraOS** як основну firmware. Він збирає окремий multiboot binary:

`doom.bin`

Після build поклади `doom.bin` на SD поруч із легально отриманим WAD-файлом. Дивись `MULTIBOOT_HOWTO.md`.

## SD layout

Приклад:

`/apps/doom/doom.bin`
`/apps/doom/doom.wad`

Doom шукає `doom*.wad` у тій самій папці, звідки multiboot запустив `doom.bin`.

## External TFT

Поточний backend малює Doom native 320x200 у 480x300 на ILI9488, із чорними смугами 10 px зверху/знизу.

Піни:

SCK -> GPIO12
MOSI -> GPIO14
CS -> GPIO48
DC -> RX
RST -> TX
MISO -> not connected

MADCTL/rotation виставлено на `0x88` після першого тесту, де зображення було перевернуте.

## Assets

WAD-файли не входять у цей пакет і не мають комітитись у репозиторій. Дивись `WAD_ASSETS.md`.

## License / notice

Дивись `LICENSE`, `COPYING`, `NOTICE.md` і license файли у `lib/doomgeneric`.
