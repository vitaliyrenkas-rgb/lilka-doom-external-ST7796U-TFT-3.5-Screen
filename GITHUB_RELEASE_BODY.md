# Doom for Lilka — ST7796U 3.5″ / v1.0.0

> GitHub Release body is intentionally text-only. The illustrated release notes and gallery live in the repository (`RELEASE_NOTES_EN.md`, `RELEASE_NOTES_UA.md`, `docs/GALLERY_*.md`). If you want a hero image in the GitHub Release itself, drag `docs/images/hero-title.jpg` into the Release editor so GitHub hosts it correctly.

## Doom Guy Leaves the Lab


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


### QUALITY — 480×300

- full 480-pixel width;
- `x=0, y=10` on the physical 480×320 panel;
- exact 3:2 expansion;
- polling DMA;
- 2880 B chunk;
- **25–26 FPS**;
- gameplay / save / load / sound — PASS.


The difference is visible in real use, so both modes stay. There is no fake “one best mode” claim here.

---

## Native Lilka startup UI

Display and sound selection use the native `lilka::Menu` directly on the external ST7796U.



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


---

## Exit From Hell

The final boss was almost comically symbolic.

Doom already had its native:

`Quit Game` → `Do you wanna exit to DOS? Y/N`

So we did not punch another hole through the wall. We simply put a proper handle on the existing door:

- **START = Y / exit to KeiraOS**;
- **SELECT = N / cancel**.


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


---

---

## Upstream / Credits — thank you

This release would not exist without the upstream work of:

- @And3rson — Lilka / Lilka v2: https://docs.lilka.dev/uk/latest/
- Lilka Doom POC: https://github.com/lilka-dev/doom_port
- @ozkl — DoomGeneric upstream: https://github.com/ozkl/doomgeneric

Huge respect and gratitude for the foundation we were able to build on.


## WAD / copyright

WAD files are not part of this release. Do not commit or redistribute commercial game data with this repository.

This is an unofficial port and is not affiliated with id Software, Bethesda, or Microsoft.

---

**Doom Guy discharged from Keira’s laboratory.**
**Condition: stable.**
**Demons: still a problem.**


---

# Українська версія

## Doom Guy виходить із лабораторії


Все почалося з дуже простої думки:

> **«Я не хочу тупити в маленький екран».**

А закінчилося повноцінним Doom на Lilka v2 із зовнішнім 3.5″ ST7796U, двома реально корисними режимами зображення, polling DMA, звуком, сейвами, автономною роботою й нормальним виходом назад у KeiraOS.

Це вже не proof of concept.

Це реліз.

---

## Що входить у реліз

### VANILLA 35 — 400×250

- centered `x=40, y=35`;
- exact 5:4 scaling із native 320×200;
- polling DMA;
- 4000 B chunk;
- **33–35 FPS** стабільно;
- gameplay / save / load / sound — PASS;
- battery/no-USB — PASS;
- boot без Serial Monitor — PASS.


### QUALITY — 480×300

- full 480-pixel width;
- `x=0, y=10` на фізичному 480×320 TFT;
- exact 3:2 scaling;
- polling DMA;
- 2880 B chunk;
- **25–26 FPS**;
- gameplay / save / load / sound — PASS.


Різниця між режимами реально відчувається, тому ми лишили обидва, замість того щоб оголосити один «правильним».

---

## Native Lilka startup UI

Display mode та sound device вибираються прямо на external ST7796U через рідний `lilka::Menu`.



Sound options:

- I2S DAC;
- П'єзо-динамік;
- Без звуку.

І ми свідомо зупинилися там, де UI вже виглядав органічно. Меню люди проскакують за секунду; не варто плодити регреси заради косметики.

---

## Polling DMA — той самий демоноріз

Найбільший технічний результат цієї епопеї — не просто FPS, а знайдений стабільний transport path.

Складні async/queue/ISR експерименти не дали того, що врешті дав простіший **polling DMA**:

- передбачуваність;
- стабільність;
- відсутність black-screen архітектури;
- фізично відчутний приріст швидкодії;
- простіший код, який реально можна підтримувати.

Іноді простіший інструмент ріже демона краще.

---

## Що ми свідомо відкинули

### 480×250 VANILLA WIDE

Воно працювало приблизно на **29 FPS**, transport був стабільний, але картинка була помітно розтягнута по X.

Вердикт:

**REJECTED / DO NOT DEVELOP FURTHER.**

Transport PASS не означає product PASS.

---

## Save / Load

Сейви працюють, завантаження працює, regression пройдений.


---

## Exit From Hell

Фінальний бос виявився дуже символічним.

У Doom уже був штатний:

`Quit Game` → `Do you wanna exit to DOS? Y/N`

Тому ми не проламували нову дірку у стіні. Просто повісили нормальну ручку на вже існуючі двері:

- **START = Y / вихід у KeiraOS**;
- **SELECT = N / cancel**.


Після START Doom використовує вже перевірений backend повернення в OTA0 KeiraOS і робить clean reboot.

Без RESET.

---

## System requirements — VERIFIED

- 3.5″ 480×320 ST7796U/ST7796S;
- 14-pin FFC;
- ESP32-S3 N16R8;
- найкращий перевірений досвід — Lilka v2 by Anderson;
- SD card;
- legal Doom WAD;
- трохи такої ж упоротості.

**Clock note:** FFC/ST7796U path фізично витримував до **125 MHz** на системному KeiraOS bench. Сам фінальний Doom release працює на **80 MHz bulk SPI** — це його validated configuration.

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
- повторний запуск Doom;
- без Serial Monitor;
- батарея / без USB.

Після цього ми перестали тестувати Doom і помітили, що просто граємо.

**Оце і був справжній release criterion.**

---

## Як ми ростили цього чортяку

Повна історія з фотографіями: **[docs/HOW_IT_WAS_UA.md](docs/HOW_IT_WAS_UA.md)**


---

## WAD / copyright

WAD не входить у реліз. Не комітьте й не розповсюджуйте commercial game data разом із цим репозиторієм.

Це неофіційний порт, не афілійований з id Software / Bethesda / Microsoft.

---

**Doom Guy discharged from Keira’s laboratory.**
**Condition: stable.**
**Demons: still a problem.**
