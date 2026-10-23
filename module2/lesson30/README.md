# Модуль 2, урок 30 — STM32 і VSCode

Домашнє завдання: винести бізнес-логіку (blink) з `main.c` у функції `setup()` і `loop()`
за аналогією з Arduino Framework.
Плата: STM32F411CEU6 (Black Pill), LED на `PC13`. Проєкт згенеровано у STM32CubeMX / STM32CubeIDE.

## Структура

У репозиторії лише власний код і конфігурація CubeMX.
Драйвери HAL/CMSIS, USB-стек, startup, лінкер-скрипти та файли проєкту IDE генеруються з
[first.ioc](first.ioc) і в git не потрапляють (див. [.gitignore](.gitignore)).

- [first.ioc](first.ioc) — конфігурація STM32CubeMX: тактування, піни `LED` (PC13) і `BTN` (PA0), USB CDC.
- [Core/Inc/app.h](Core/Inc/app.h) — оголошення `setup()` і `loop()`.
  Include guard `#ifndef __APP_H` / `#define __APP_H` не дає підключити заголовок повторно
  і захищає від циклічного імпорту.
- [Core/Src/app.c](Core/Src/app.c) — логіка: `setup()` вимикає LED, `loop()` перемикає його кожні 500 мс.
- [Core/Src/main.c](Core/Src/main.c) — лише ініціалізація HAL, тактування і периферії;
  у блоці `USER CODE 2` викликається `setup()`, у нескінченному циклі (`USER CODE 3`) — `loop()`.
  Код стоїть у секціях `USER CODE`, тож CubeMX не зітре його під час регенерації.

LED на `PC13` вмикається низьким рівнем, тому `GPIO_PIN_SET` його вимикає.

## Збирання і прошивання

1. У STM32CubeIDE: **File → New → STM32 Project from an Existing STM32CubeMX Configuration File (.ioc)**
   і вибрати `first.ioc` у цій папці.
2. Відкрити `first.ioc` і натиснути **Project → Generate Code**. CubeMX допише згенеровані файли,
   а код у секціях `USER CODE` файлу `main.c` залишить без змін.
3. **Project → Build Project**, потім **Run**.

Із терміналу тим самим тулчейном, що встановлює CubeIDE (GNU Tools for STM32 14.3),
після хоча б однієї збірки в IDE (вона генерує `Debug/makefile`):

```sh
make -C Debug all
STM32_Programmer_CLI -c port=SWD mode=UR -w Debug/first.elf -v -rst
```

## Результати

Журнали з реальної плати — у [results](results):

- [build.txt](results/build.txt) — збірка без попереджень, розмір прошивки, символи `setup`/`loop` у `first.elf`.
- [flash.txt](results/flash.txt) — прошивання через ST-LINK V2 і `Download verified successfully`.
- [led-pin-samples.txt](results/led-pin-samples.txt) — регістр `GPIOC->ODR`, зчитаний з працюючої плати:
  біт 13 (`0x2000`) змінюється між `0` і `1`, тобто LED блимає.
