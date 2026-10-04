# Автоматические эксперименты SuperLoop и chunked EDF

[English version](README.md)

`run_matrix.py` выполняет цикл конфигурация → сборка → прошивка → UART → CSV для каждого запуска. Цель исследования описана в [основном README](../../../README.ru.md), параметры прошивки и смысл метрик - в [справочнике](../../docs/configuration.ru.md).

## Подготовка и запуск

Нужны STM32CubeIDE с STM32Cube FW F7, STM32CubeProgrammer и Python 3.10+. Установите зависимость: `py -m pip install -r implementation/tools/experiment_runner/requirements.txt`. Закройте терминал, занимающий COM-порт, и CubeIDE, если она использует ту же workspace. Выполняйте команду из корня репозитория, заменив пути на установленные у вас:

```powershell
py implementation\tools\experiment_runner\run_matrix.py `
  --port COM3 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix implementation\tools\experiment_runner\matrix.integrated_stats.json
```

Добавьте `--dry-run` для проверки плана без сборки и обращения к плате. Сборщик подключается под аппаратным сбросом (`mode=UR`, `reset=HWrst`) и делает до трёх попыток при распознаваемых ошибках подключения ST-LINK. Остальные ошибки не маскируются повторными попытками.

## Матрицы integrated

| Матрица | Планировщик | Окно, s | Чанк, ms | Запуски |
| --- | --- | ---: | ---: | ---: |
| `matrix.integrated_stats.json` | SuperLoop | 60 | - | 42 |
| `matrix.integrated_edf.json` | EDF | 60 | 1 | 42 |
| `matrix.integrated_stats_100s.json` | SuperLoop | 100 | - | 18 |
| `matrix.integrated_edf_100s.json` | EDF | 100 | 1 | 18 |
| `matrix.integrated_edf_chunk2ms.json` | EDF | 60 | 2 | 18 |
| `matrix.integrated_edf_chunk4ms.json` | EDF | 60 | 4 | 18 |

Основная серия 60 s использует U50/U65/U75/U80/U90/U95/U100. Серии 100 s и 2/4 ms используют U50/U90/U100. Везде 2/3 задачи, три повтора, Release, датчики выключены, `extended_stats: false`. Всего 156 запусков и 180 минут измерений плюс сборки/прошивки.

Запускайте матрицы последовательно, каждую в отдельном новом каталоге. Для сравнения чанков используйте соответствующее подмножество основной серии EDF 1 ms при одинаковых исходниках и настройках. Прямое измерение задержки вытеснения эта серия не выполняет.

Для диагностики скопируйте матрицу и задайте `extended_stats: true`. Значение попадает в `EXTENDED_STATS` и проверяется сборщиком. Не объединяйте диагностические измерения с компактными или старые Debug-результаты с Release.

## Другие профили

- `matrix.default.json`: 84 запуска `clean/checks`, U50/U65/U80/U90/U95/U100, окна 10/30/60/100/250/500/1000 s; 390 минут измерений.
- `matrix.scalability.json`: 12 запусков `scale_clean/scale_checks`, U65/U90, 2/3/4 задачи, 60 s; 12 минут.

Без `--matrix` скрипт выбирает `matrix.default.json`, а не integrated. Эти отдельные профили выполняют SuperLoop независимо от `scheduler_algorithm`; для EDF выбирайте `integrated`. Integrated поддерживает 2/3 задачи. Изменяйте копию подходящей матрицы; поддерживаемые сценарием прошивки значения могут отличаться от допускаемых сборщиком (например, U110 пока отсутствует в runner).

## Результаты и продолжение

Каждая кампания создаёт `results/<UTC-время>/`:

- `summary.csv` - параметры, статус и показатели запусков;
- `task_summary.csv` - строки задач integrated;
- `raw/*.log` - UART;
- `build/*.log`, `build/*.flash.log`, `build/*.flash.attemptN.log` - сборка и попытки прошивки;
- `matrix.json` и `manifest.json` - параметры и fingerprint исходников.

Файлы игнорируются Git. `run_key` уникален внутри кампании, но не содержит алгоритм или чанк: объединяйте таблицы по паре каталог кампании + `run_key`. Размер чанка берите из `summary.csv` (`CHUNK_US`). Смысл CSV и незавершённых заданий описан в [справочнике конфигурации](../../docs/configuration.ru.md).

Для продолжения передайте прежнюю матрицу и `--output-dir <каталог> --resume`. Успешные запуски пропускаются, неудачные повторяются; старые failed-строки сохраняются. Анализируйте успешные уникальные запуски. Resume требует совпадения матрицы и fingerprint исходников, включая скрипт; при несовпадении используйте новую кампанию, не подменяйте manifest.

Скрипт временно изменяет `experiment_config.h` и восстанавливает исходное содержимое при обычном выходе, ошибке и Ctrl+C. После принудительного завершения процесса проверьте файл перед ручной сборкой. Временная workspace CubeIDE создаётся вне репозитория.
