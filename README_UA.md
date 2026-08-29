<p align="center">
  <img src="docs/images/hero-title.jpg" alt="Doom на Lilka v2 із зовнішнім ST7796U 3.5 дюйма" width="900">
</p>

# Doom для Lilka — ST7796U 3.5″

> **Від «я не хочу тупити в маленький екран» до VANILLA 35.**

Це завершений multiboot-порт Doom для **Lilka v2 / ESP32-S3 N16R8** із виводом на зовнішній **3.5″ ST7796U 480×320** через FFC.

Ціль була проста: не технодемка, а нормальна річ, яку можна запустити з KeiraOS, сісти й грати — зі звуком, сейвами, двома режимами зображення і нормальним виходом назад у систему.

## Що вже VERIFIED / PASS

- **VANILLA 35 — 400×250, 33–35 FPS**;
- **QUALITY — 480×300, 25–26 FPS**;
- polling DMA transport;
- вибір режиму зображення на external TFT;
- native `lilka::Menu` для startup UI;
- `I2S DAC`, `П'єзо-динамік`, `Без звуку`;
- gameplay;
- save / load;
- Doom menu;
- робота без Serial Monitor;
- автономна робота від батареї без USB;
- запуск із KeiraOS;
- штатний Doom `Quit`;
- **START = YES**, **SELECT = NO**;
- clean reboot назад у KeiraOS;
- повторний запуск Doom.

## Режими зображення

| Режим | Геометрія | Scaling | Transport | Фізичний результат |
|---|---:|---|---|---:|
| **VANILLA 35** | 400×250, centered `x=40, y=35` | exact 5:4 | polling DMA, 4000 B chunk | **33–35 FPS** |
| **QUALITY** | 480×300, `x=0, y=10` | exact 3:2 | polling DMA, 2880 B chunk | **25–26 FPS** |

### VANILLA 35

320×200 Doom framebuffer розгортається в 400×250 точним 5:4 nearest-neighbor expander. Це режим для максимально плавної гри й наш основний шлях до «ванільних» ~35 кадрів.

<p align="center">
  <img src="docs/images/display-mode-vanilla.jpg" alt="VANILLA 35 selected" width="820">
</p>

### QUALITY

320×200 розгортається в 480×300 точним 3:2. Режим використовує всю ширину 480×320 дисплея, залишаючи по 10 px чорної смуги зверху й знизу.

<p align="center">
  <img src="docs/images/display-mode-quality.jpg" alt="QUALITY selected" width="820">
</p>

## System requirements — перевірена конфігурація

- **3.5″ 480×320 TFT з контролером ST7796U/ST7796S**;
- **14-pin FFC display interface**;
- **ESP32-S3 N16R8**;
- найкращий фізично перевірений досвід — **Lilka v2 by Anderson**;
- SD-картка;
- легально отриманий Doom WAD;
- трохи здорової технічної упоротості.

> **Про частоту:** FFC/ST7796U тракт у нашій системі фізично валідований до **125 MHz** у KeiraOS. Фінальний Doom release transport у цьому source tree навмисно працює на **80 MHz bulk SPI** — саме ця конфігурація пройшла Doom regression.

## Підключення external ST7796U

Фактичний mapping із фінального source tree:

| TFT signal | Lilka / ESP32-S3 |
|---|---|
| SCK | GPIO12 |
| MOSI | GPIO14 |
| CS | GPIO48 |
| DC | RX / GPIO44 |
| RST | GPIO47 |
| MISO | не використовується |
| BL | зовнішні 5 V; firmware не драйвить BL |

> Історична назва файлів `external_ili9488.*` лишена навмисно. Це вже ST7796U backend; перейменування не робилося перед релізом, щоб не створювати зайвий regression-ризик.

## Startup UI

Вибір display mode та звукового пристрою відбувається на великому ST7796U через рідний `lilka::Menu`.

<p align="center">
  <img src="docs/images/sound-device.jpg" alt="Вибір звукового пристрою" width="820">
</p>

Доступно:

- **I2S DAC**;
- **П'єзо-динамік**;
- **Без звуку**.

## Керування

| Lilka | Doom |
|---|---|
| D-pad | рух / поворот |
| A | FIRE |
| B | USE |
| C | automap / TAB |
| D | наступна зброя |
| SELECT | ESC / меню / cancel |
| START | ENTER / confirm |

У startup `lilka::Menu` активація працює штатною кнопкою **A**, а також **START**.

## Save / Load

Сейви працюють штатно й пережили фінальний regression.

<p align="center">
  <img src="docs/images/load-game.jpg" alt="LILKA SAVE slots" width="820">
</p>

## Exit From Hell

Ми не робили новий exit screen. Doom уже мав нормальні двері.

Штатний:

`Quit Game` → `Do you wanna exit to DOS? Y/N`

на Lilka працює так:

- **START → Y → вихід у KeiraOS**;
- **SELECT → N → скасувати й повернутися в Doom**.

<p align="center">
  <img src="docs/images/quit-confirmation.jpg" alt="Native Doom quit confirmation" width="820">
</p>

Backend повернення використовує перевірений KeiraOS OTA0 reboot path. Фізичний RESET не потрібен.

## Quick start

Це **multiboot binary**, його не треба прошивати поверх KeiraOS як основну firmware.

1. Відкрити проєкт у VS Code / PlatformIO.
2. Зібрати:

```bash
pio run -e v2
```

3. `move_firmware.py` копіює готовий firmware в корінь як `doom.bin`.
4. Покласти `doom.bin` і WAD в одну папку на SD, наприклад:

```text
/apps/doom/doom.bin
/apps/doom/doom.wad
```

5. Запустити `doom.bin` із KeiraOS multiboot / launcher.
6. Вибрати display mode.
7. Вибрати sound device.
8. Грати.

Doom шукає `doom*.wad` у тій самій SD-папці, звідки запущено `doom.bin`.

## Галерея

<p align="center">
  <img src="docs/images/gameplay-clean.jpg" alt="Doom gameplay on ST7796U" width="820">
</p>

Повна галерея: **[docs/GALLERY_UA.md](docs/GALLERY_UA.md)**
Історія розробки: **[docs/HOW_IT_WAS_UA.md](docs/HOW_IT_WAS_UA.md)**
Технічний канон релізу: **[docs/TECHNICAL_NOTES.md](docs/TECHNICAL_NOTES.md)**

## WAD / game data

WAD-файли **не входять у цей репозиторій і не повинні комітитись**. Використовуйте легально отримані game data. Див. [WAD_ASSETS.md](WAD_ASSETS.md).


## Подяки / Upstream

Цей реліз стоїть на роботі людей, без яких його не було б:

- **[@And3rson](https://github.com/and3rson)** — Lilka та Lilka ecosystem;
- **[Lilka v2 — документація](https://docs.lilka.dev/uk/latest/)**;
- **[lilka-dev/doom_port](https://github.com/lilka-dev/doom_port)** — оригінальна Doom POC / porting робота для Lilka;
- **[@ozkl](https://github.com/ozkl)** — upstream **[DoomGeneric](https://github.com/ozkl/doomgeneric)**.

Повний блок подяк: **[CREDITS.md](CREDITS.md)**.

## License / notice

Це неофіційний port package. Він не пов'язаний і не афілійований з id Software / Bethesda / Microsoft чи іншими правовласниками.

Див. `LICENSE`, `COPYING`, `NOTICE.md` та license files у `lib/doomgeneric`.

---

**At some point we stopped testing Doom and noticed we were simply playing it. That’s when we knew it was done.**
