# Модуль 2, урок 29 — STM32 знайомство

Домашнє завдання: на основі коду з уроку (`HAL_GPIO_TogglePin` + `HAL_Delay(500)`) додати обробку кнопки.
Плата: STM32F411CEU6 (Black Pill). Світлодіод `LED` — `PC13` (синій, підпис C13),
кнопка `BTN` — `PA0` (кнопка **KEY** поруч зі світлодіодами, а не NRST чи BOOT0).

## Поведінка

- Кнопку відпущено — світлодіод блимає повільно, кожні 500 мс.
- Кнопку затиснуто — світлодіод блимає швидко, кожні 100 мс.

## Код

У репозиторії лише власний код і конфігурація CubeMX [first.ioc](first.ioc);
решту генерує STM32CubeIDE (див. [.gitignore](.gitignore)).
Весь доданий код — у секціях `USER CODE` файлу [Core/Src/main.c](Core/Src/main.c):

```c
/* USER CODE BEGIN PD */
#define BLINK_SLOW_MS 500 /* button released */
#define BLINK_FAST_MS 100 /* button held */

/* USER CODE BEGIN 1 */
uint32_t last_toggle = 0;

/* USER CODE BEGIN 3 */
uint8_t pressed = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET;
uint32_t period = pressed ? BLINK_FAST_MS : BLINK_SLOW_MS;

if (HAL_GetTick() - last_toggle >= period)
{
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  last_toggle = HAL_GetTick();
}
```

Кнопка має внутрішню підтяжку до живлення (`GPIO_PULLUP`), тому натиснута кнопка читається як `GPIO_PIN_RESET`.
`HAL_Delay(500)` з уроку замінено перевіркою `HAL_GetTick()`. Так цикл не блокується,
кнопка опитується постійно і зміну частоти видно одразу, а не після завершення затримки.

## Збирання і прошивання

1. У STM32CubeIDE: **File → New → STM32 Project from an Existing STM32CubeMX Configuration File (.ioc)**
   і вибрати `first.ioc` у цій папці.
2. Відкрити `first.ioc` і натиснути **Project → Generate Code**. Код у секціях `USER CODE` залишиться.
3. **Project → Build Project**, потім **Run**.

## Результати

Журнали з реальної плати — у [results](results):

- [build.txt](results/build.txt) — збірка без попереджень.
- [flash.txt](results/flash.txt) — прошивання через ST-LINK V2 і `Download verified successfully`.
- [led-timing-released.txt](results/led-timing-released.txt) — 300 зчитувань `uwTick`, `PA0` і `PC13`
  з працюючої плати при відпущеній кнопці: `PA0` = 1, середній інтервал перемикання LED — 500,4 мс.
- Затиснута кнопка KEY перевірена візуально на платі: світлодіод блимає помітно швидше.
