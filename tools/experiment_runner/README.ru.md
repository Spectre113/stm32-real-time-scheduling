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

### Отдельный прогон integrated-статистики

`matrix.integrated_stats.json` - ещё одна независимая кампания для исходного integrated Superloop-профиля с полной статистикой. В ней используются `U50`, `U75`, `U90` и `U100`; 2 и 3 синтетические задачи; окна 30/60/100 секунд - всего 24 измерения.

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix tools\experiment_runner\matrix.integrated_stats.json
```

Суммарная длительность измеряемых окон - 25,3 минуты. Помимо `summary.csv`, эта кампания записывает `task_summary.csv` со всеми `CSV_TASK` строками: выполнением задач, response time, deadline и skipped releases. В конфигурации из двух задач используются IMU и LiDAR; их относительное распределение workload 3:4 нормализуется к выбранной суммарной загрузке.

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

Для каждого запуска скрипт временно переписывает [`Core/Inc/experiment_config.h`](../../Core/Inc/experiment_config.h), а в конце - даже при ошибке или `Ctrl+C` - восстанавливает исходное содержимое. После принудительного выключения ПК проверьте этот файл через `git diff` перед ручной сборкой.
