# FirstEmbeddedProject

Навчальні проєкти для ESP32-S3 (Arduino Framework, PlatformIO) і STM32 (STM32CubeIDE).

| Модуль | Проєкт | Опис і запуск |
| --- | --- | --- |
| module1 | Сутінковий вимикач: фоторезистор і реле | [Інструкція та схема](module1/README.md) |
| module2, урок 28 | Світлофор: апаратні таймери, переривання та автомат станів | [Інструкція та результати](module2/README.md) |
| module2, урок 29 | STM32F411: кнопка KEY змінює частоту блимання LED | [Інструкція та результати](module2/lesson29/README.md) |
| module2, урок 30 | STM32F411: blink через `setup()`/`loop()` в `app.c` | [Інструкція та результати](module2/lesson30/README.md) |

## Збирання

Із кореня репозиторію:

```sh
pio run -d module1 -e esp32-s3-devkitc-1
pio run -d module2 -e esp32-s3
sh module2/tests/run.sh
```

У VS Code відкрийте папку потрібного модуля через PlatformIO.
Підключення, команди прошивання та пояснення реалізації наведено в README кожного модуля.

## Перевірки та результати

[GitHub Actions](https://github.com/YuriiKyiv/FirstEmbeddedProject/actions/workflows/build.yml)
збирає обидва модулі, запускає тести світлофора та зберігає прошивки як артефакти.
Локальні журнали збірки й симуляції світлофора — у [module2/results](module2/results).
Симуляція не замінює перевірку на фізичній платі.
