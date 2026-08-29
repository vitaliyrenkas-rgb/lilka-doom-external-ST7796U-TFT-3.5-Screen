# Doom для Lilka / ST7796U 3.5″ — v1.0.0
## Doom Guy виходить із лабораторії

<p align="center">
  <img src="docs/images/hero-title.jpg" alt="The Ultimate Doom on Lilka" width="900">
</p>

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

<p align="center">
  <img src="docs/images/telemetry-33fps.jpg" alt="33 FPS telemetry" width="560">
</p>

### QUALITY — 480×300

- full 480-pixel width;
- `x=0, y=10` на фізичному 480×320 TFT;
- exact 3:2 scaling;
- polling DMA;
- 2880 B chunk;
- **25–26 FPS**;
- gameplay / save / load / sound — PASS.

<p align="center">
  <img src="docs/images/telemetry-25fps.jpg" alt="25 FPS telemetry" width="560">
</p>

Різниця між режимами реально відчувається, тому ми лишили обидва, замість того щоб оголосити один «правильним».

---

## Native Lilka startup UI

Display mode та sound device вибираються прямо на external ST7796U через рідний `lilka::Menu`.

<p align="center">
  <img src="docs/images/display-mode-vanilla.jpg" alt="Display mode selection" width="820">
</p>

<p align="center">
  <img src="docs/images/sound-device.jpg" alt="Sound device selection" width="820">
</p>

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

<p align="center">
  <img src="docs/images/load-game.jpg" alt="LILKA SAVE slots" width="820">
</p>

---

## Exit From Hell

Фінальний бос виявився дуже символічним.

У Doom уже був штатний:

`Quit Game` → `Do you wanna exit to DOS? Y/N`

Тому ми не проламували нову дірку у стіні. Просто повісили нормальну ручку на вже існуючі двері:

- **START = Y / вихід у KeiraOS**;
- **SELECT = N / cancel**.

<p align="center">
  <img src="docs/images/quit-confirmation.jpg" alt="Doom quit confirmation" width="820">
</p>

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

<p align="center">
  <img src="docs/images/gameplay-clean.jpg" alt="Finished Doom gameplay" width="860">
</p>

---


## Подяки / Upstream

Цей реліз стоїть на роботі людей, без яких його не було б:

- **[@And3rson](https://github.com/and3rson)** — Lilka та Lilka ecosystem;
- **[Lilka v2 — документація](https://docs.lilka.dev/uk/latest/)**;
- **[lilka-dev/doom_port](https://github.com/lilka-dev/doom_port)** — оригінальна Doom POC / porting робота для Lilka;
- **[@ozkl](https://github.com/ozkl)** — upstream **[DoomGeneric](https://github.com/ozkl/doomgeneric)**.

Повний блок подяк: **[CREDITS.md](CREDITS.md)**.

## WAD / copyright

WAD не входить у реліз. Не комітьте й не розповсюджуйте commercial game data разом із цим репозиторієм.

Це неофіційний порт, не афілійований з id Software / Bethesda / Microsoft.

---

**Doom Guy discharged from Keira’s laboratory.**
**Condition: stable.**
**Demons: still a problem.**
