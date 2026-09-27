# Автоматический сбор профилей Superloop

[English version](README.md)

`run_matrix.py` выполняет матрицу экспериментов без PuTTY: на каждом запуске он меняет конфигурацию прошивки, собирает её, прошивает плату через ST-LINK, читает UART и сохраняет результаты.

## Что устанавливается один раз

1. STM32CubeIDE с STM32Cube FW F7.
2. STM32CubeProgrammer.
3. Python 3.10 или новее и зависимость UART:

   ```powershell
   py -m pip install -r tools\experiment_runner\requirements.txt
   ```

Найдите два исполняемых файла. Типичные пути в Windows:

```text
C:\ST\STM32CubeIDE_<версия>\STM32CubeIDE\headless-build.bat
C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe
```

Перед запуском закройте STM32CubeIDE, если она открыла ту же workspace, и закройте PuTTY: COM-порт должен быть свободен для скрипта.

## Полный прогон

Подключите `NUCLEO-F767ZI`, узнайте номер ST-LINK Virtual COM Port и выполните из корня репозитория:

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\ST\STM32CubeIDE_<версия>\STM32CubeIDE\headless-build.bat" `
  --programmer "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
```

`matrix.default.json` задаёт 84 запуска: `U50`-`U100`, окна 10/30/60/100/250/500/1000 секунд, режимы `clean` и `checks`. Только суммарная длительность измеряемых окон составляет 390 минут; добавьте время на 84 сборки и прошивки.

### Отдельный scalability-прогон

`matrix.scalability.json` намеренно отделён от основной матрицы. Он содержит ровно 12 измерений: 2/3/4 задачи, сценарии `U65` и `U90`, окно 60 секунд, режимы `scale_clean` и `scale_checks`. Он не повторяет и не изменяет 84-запусковую кампанию.

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix tools\experiment_runner\matrix.scalability.json
```

Суммарная длительность измеряемых окон - 12 минут; добавьте несколько минут на сборки и прошивки. Запустите без `--output-dir`, чтобы получить новую папку результатов и отдельный `summary.csv`.

### Integrated: сравнение планировщиков

| Матрица | Планировщик | Окно | Запуски |
|---|---|---|---|
| `matrix.integrated_stats.json` | Super Loop | 60 s | 42 |
| `matrix.integrated_edf.json` | EDF | 60 s | 42 |
| `matrix.integrated_stats_100s.json` | Super Loop | 100 s | 18 |
| `matrix.integrated_edf_100s.json` | EDF | 100 s | 18 |

Основная серия: U50/U65/U75/U80/U90/U95/U100; проверочная: U50/U90/U100. Везде 2/3 задачи, три повтора, Release, чанк EDF 1000 us. Всего 120 запусков и 144 минуты измерений плюс сборки/прошивки. Передавайте каждую матрицу через `--matrix` в отдельный новый каталог результатов.

`extended_stats: false` сохраняет метрики статьи: завершения, misses/skips, среднее и максимум response time, суммарное execution time, расходы планировщика/polling и состояние незавершённых заданий. Гистограммы, образцы, min/max execution и циклов, minimum response time отключены. Для диагностики задайте `extended_stats: true` в отдельной матрице или `ENABLE_EXTENDED_STATS=1` при ручной сборке. CSV содержит проверяемое поле `EXTENDED_STATS`. Не смешивайте диагностические и компактные запуски либо старые Debug-результаты. Остальные профили сохраняют расширенную статистику.

Для безопасной проверки без платы:

```powershell
py tools\experiment_runner\run_matrix.py --port COM5 --headless-builder C:\path\headless-build.bat --programmer C:\path\STM32_Programmer_CLI.exe --dry-run
```

## Результаты

Каждая кампания создаёт отдельный каталог `results/<UTC-время>/`:

- `summary.csv` - единая таблица всех запусков, включая запрошенные и реально выведенные параметры;
- `task_summary.csv` - все `CSV_TASK` строки integrated-статистики;
- `raw/*.log` - полный UART-лог каждого запуска;
- `build/*.log` и `build/*.flash.log` - журналы сборки и прошивки;
- `matrix.json` - точная копия матрицы, использованной в кампании;
- Временная headless-workspace STM32CubeIDE создаётся вне репозитория и не попадает в папку результатов.

Все эти файлы игнорируются Git. При сбое строка со `status=failed` и текстом ошибки всё равно добавляется в `summary.csv`; успешные запуски можно не повторять, если продолжить ту же кампанию с `--output-dir <каталог> --resume`.

## Изменение матрицы

Скопируйте `matrix.default.json`, оставьте нужные сценарии, окна или режимы и передайте путь через `--matrix`. Для `scale_clean` или `scale_checks` также укажите `task_counts`, содержащий только `2`, `3` и/или `4`. Например, короткая проверка одного режима:

```json
{
  "scenarios": ["U65"],
  "windows_us": [10000000],
  "modes": ["clean"],
  "repeats": 1
}
```

Для каждого запуска скрипт временно переписывает [`implementation/Core/Inc/experiment_config.h`](../../implementation/Core/Inc/experiment_config.h), а в конце - даже при ошибке или `Ctrl+C` - восстанавливает исходное содержимое. После принудительного выключения ПК проверьте этот файл через `git diff` перед ручной сборкой.

### Граница integrated-эксперимента

Integrated-профиль выполняет одно измерительное окно на сброс. После вывода отчёта он ждёт сброса/перепрошивки; автоматический сборщик прошивает следующую конфигурацию как раньше. Состояние незавершённых заданий не сбрасывается для запуска следующего окна.

Все строки задач используют один момент окончания окна, снятый до UART-вывода. В `task_summary.csv` добавлены:

- `PENDING`: 0/1 — текущее задание уже выпущено, но не завершено (включая ещё не начатое).
- `PENDING_OVERDUE`: 0/1 — у этого задания на границе окна уже строго превышен дедлайн.
- `PENDING_AGE_US`: время от релиза этого задания до границы окна.
- `PENDING_EXEC_US`: уже измеренное время исполнения его чанков.

Эти поля относятся к текущему отслеживаемому заданию задачи, а не ко всем последующим релизам, для которых ещё не выполнен учёт пропусков. `MISSES` и response time по-прежнему относятся только к завершённым заданиям. Для описания результатов отдельно сообщайте незавершённые и просроченные незавершённые задания; не включайте их в среднее response time. `TASK_EXEC` включает уже выполненные чанки незавершённых заданий.

### Влияние размера чанка EDF

`matrix.integrated_edf_chunk2ms.json` и `matrix.integrated_edf_chunk4ms.json` задают чанки 2000 и 4000 us. Каждая матрица: U50/U90/U100, 2/3 задачи, окно 60 s, три повтора, Release, компактная статистика. Это 18 запусков на матрицу, всего 36 дополнительных минут измерений плюс сборки/прошивки.

Запускайте обычным `run_matrix.py` с `--matrix tools/experiment_runner/matrix.integrated_edf_chunk2ms.json` или `--matrix tools/experiment_runner/matrix.integrated_edf_chunk4ms.json` и своими параметрами COM-порта, сборщика и программатора. Для проверки без платы добавьте `--dry-run`.

Используйте отдельный новый каталог результатов для каждой матрицы (по умолчанию создаётся автоматически): run_key не включает размер чанка. Для базовых 1 ms берите только U50/U90/U100 из `matrix.integrated_edf.json`, при одинаковых исходниках и настройках. Сопоставляйте размер чанка через CHUNK_US в summary.csv и сохранённую matrix.json; task_summary.csv связывайте с summary.csv внутри своей кампании. Основные показатели: response time по задачам, misses/skips, расходы планировщика и незавершённые задания. Прямое измерение задержки вытеснения в эту серию не входит.
