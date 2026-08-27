# Демонстрация планировщиков реального времени на STM32F767

Проект STM32CubeIDE для исследования алгоритмов планирования реального времени на микроконтроллере `STM32F767ZITx`: невытесняющего Superloop, невытесняющего EDF и Chunked EDF с кооперативным разбиением синтетической работы на части. Он запускает синтетические периодические задачи, измеряет их выполнение и выводит результаты в последовательный порт.

По умолчанию включён чистый профиль Superloop для сценария `U65`: две синтетические задачи работают 60 секунд, после чего в UART выводятся отчёт и CSV-строка.

## Что потребуется

- Плата `NUCLEO-F767ZI` со встроенными ST-LINK и Virtual COM Port.
- USB-кабель с передачей данных.
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) с STM32Cube FW F7. Проект создан в STM32CubeMX 6.16.1.
- Терминал для UART: [PuTTY](https://www.putty.org/), Tera Term или аналог.

На Windows драйвер ST-LINK обычно устанавливается вместе со STM32CubeIDE. Если виртуальный COM-порт не появился в «Диспетчере устройств», обновите драйвер ST-LINK и переподключите плату.

## Быстрый запуск

1. Клонируйте репозиторий или распакуйте его архив.
2. В STM32CubeIDE выберите `File → Import… → General → Existing Projects into Workspace`, укажите корень репозитория и завершите импорт.
3. Подключите плату через разъём ST-LINK USB.
4. Соберите проект: `Project → Build All`.
5. Откройте UART-терминал до запуска программы.
6. Нажмите `Run` или `Debug` в STM32CubeIDE. IDE прошьёт контроллер через ST-LINK и запустит программу.

### Настройка PuTTY

1. В «Диспетчере устройств → Порты (COM и LPT)» найдите номер `ST-LINK Virtual COM Port`, например `COM5`.
2. В PuTTY выберите `Session → Connection type: Serial`, задайте этот COM-порт и скорость `115200`.
3. В `Connection → Serial` установите параметры:

   | Параметр | Значение |
   | --- | --- |
   | Speed | `115200` |
   | Data bits | `8` |
   | Stop bits | `1` |
   | Parity | `None` |
   | Flow control | `None` |

4. Нажмите `Open`, затем запустите программу на плате.

Используется `USART3`: `PD8` — TX, `PD9` — RX. На `NUCLEO-F767ZI` эти линии подключены к встроенному ST-LINK Virtual COM Port согласно [`demonstration.ioc`](demonstration.ioc). При переносе на другую плату проверьте физическое соединение этих выводов со ST-LINK и при необходимости измените UART-пины в `.ioc`. UART служит только для журнала: внутри измеряемого окна профилей он не вызывается.

## Результат по умолчанию

Текущие переключатели находятся в [`Core/Src/main.c`](Core/Src/main.c):

```c
#define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U65
#define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#define EXPERIMENT_MODE EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE
```

| Параметр | Значение |
| --- | --- |
| Планировщик | `SCHED_ALGO_SUPERLOOP` |
| Сценарий | `U65` |
| Режим | `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` |
| Окно измерения | `60 000 000 us` (60 секунд) |

После окна терминал выведет `=== CLEAN SUPERLOOP PROFILE ===` и строку `CSV_CLEAN_SUPERLOOP,...`. Сохраните CSV-строку в файл или таблицу для обработки. Если PuTTY был открыт после запуска, перезапустите программу: профиль уже мог отправить свой единственный результат.

## Автоматический сбор серии экспериментов

Для прогона множества сценариев и окон без ручного переключения макросов и PuTTY используйте [`tools/experiment_runner/`](tools/experiment_runner/). Python-скрипт сам собирает и прошивает каждую конфигурацию, читает ST-LINK virtual COM port, сохраняет отдельный UART-лог и объединяет полученные CSV-строки в `results/<время>/summary.csv`.

Готовая матрица включает `U50`–`U100`, окна 10/30/60/100/250/500/1000 секунд и режимы Clean/Checks — всего 84 запуска. Подробная установка и команда запуска находятся в [инструкции автоматизации](tools/experiment_runner/README.md). Результаты игнорируются Git; временная headless-workspace CubeIDE создаётся вне репозитория.

## Настройка эксперимента

Измените константы в начале [`Core/Src/main.c`](Core/Src/main.c), сохраните файл и пересоберите проект.

### Основные параметры

| Макрос | Что можно изменить |
| --- | --- |
| `WORKLOAD_SCENARIO` | Сценарий нагрузки `U50`–`U110`. |
| `SCHED_ALGO` | Алгоритм интегрированного эксперимента: Superloop, EDF или Chunked EDF. В режимах 3 и 4 не используется. |
| `EXPERIMENT_MODE` | Тип запуска и собираемой статистики. |
| `MINIMAL_PROFILE_WINDOW_US` | Длительность окна режимов 3 и 4. По умолчанию `60000000ULL` — 60 секунд. |
| `PROFILE_WINDOW_US` | Длительность окна интегрированного профиля. По умолчанию `10000000ULL` — 10 секунд. |
| `EDF_CHUNK_US` | Размер части синтетической работы в Chunked EDF. |
| `SCHEDULER_MODE` | Режим ожидания интегрированного планировщика: busy-polling или WFI. Профили 3 и 4 всегда используют busy-polling. |
| `ENABLE_SYNTH_IMU`, `ENABLE_SYNTH_LIDAR`, `ENABLE_SYNTH_CAMERA` | Включение синтетических задач интегрированного режима. |
| `ENABLE_REAL_TAU1`, `ENABLE_REAL_TAU2` | Включение реальных HC-SR04 и DHT11; для запуска без датчиков оставьте `0`. |

Все значения времени задаются в микросекундах и имеют суффикс `ULL`. После изменения любого макроса требуется новая сборка и прошивка платы. Для сопоставимых измерений запускайте сравниваемые режимы с одинаковыми сценарием и длительностью окна.

### Сценарии нагрузки

`U` — целевая суммарная загрузка синтетическими задачами.

| Константа | Загрузка |
| --- | --- |
| `WORKLOAD_SCENARIO_U50` | 50 % |
| `WORKLOAD_SCENARIO_U65` | 65 % |
| `WORKLOAD_SCENARIO_U80` | 80 % |
| `WORKLOAD_SCENARIO_U90` | 90 % |
| `WORKLOAD_SCENARIO_U95` | 95 % |
| `WORKLOAD_SCENARIO_U100` | 100 % |
| `WORKLOAD_SCENARIO_U110` | 110 %, перегруженный сценарий |

Для профилей Superloop (режимы 3 и 4) доступны только `U50`–`U100`; `U110` поддерживается другими режимами.

### Алгоритмы планирования

| Константа | Назначение |
| --- | --- |
| `SCHED_ALGO_SUPERLOOP` | Фиксированный невытесняющий порядок готовых задач. |
| `SCHED_ALGO_EDF` | Невытесняющий EDF: выбирается готовая задача с ближайшим абсолютным дедлайном и выполняется до завершения. |
| `SCHED_ALGO_CHUNKED_EDF` | EDF с кооперативным разбиением синтетической работы на части `EDF_CHUNK_US`; после каждого chunk планировщик повторно выбирает задачу. Это не аппаратное вытеснение и не переключение контекста прерыванием. |

### Режимы экспериментов

| Константа | Что делает |
| --- | --- |
| `EXPERIMENT_INTEGRATED` (`0`) | Основной эксперимент со статистикой задач и планировщика: выполнение, response time, deadline misses, skipped releases и другие метрики. |
| `EXPERIMENT_ISOLATED_TAU1` (`1`) | Изолированное выполнение первой задачи. |
| `EXPERIMENT_ISOLATED_TAU2` (`2`) | Изолированное выполнение второй задачи. |
| `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` (`3`) | Очищенный двухзадачный busy-polling Superloop. `SUPERLOOP_PCT` — доля окна вне тел синтетических задач. |
| `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` (`4`) | Инструментированный DWT-профиль readiness-проверок и обслуживания releases. |

В режимах 3 и 4 значение `SCHED_ALGO` не используется: они запускают собственный двухзадачный busy-polling Superloop. `SCHED_ALGO` влияет на `EXPERIMENT_INTEGRATED`.

Режимы решают разные задачи и не должны смешиваться при интерпретации результатов:

- `EXPERIMENT_INTEGRATED` предназначен для анализа schedulability и статистики задач: response time, deadline misses, skipped releases и других метрик.
- `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` измеряет общую стоимость очищенного busy-polling Superloop. Физического sleep/idle в нём нет; `SUPERLOOP_PCT` включает polling/check logic и минимальную инструментальную обвязку эксперимента.
- `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` детализирует время readiness-проверок и обслуживания releases, но намеренно добавляет DWT instrumentation overhead.

Поэтому `CHECKS_PCT` режима 4 нельзя считать чистой общей стоимостью Superloop. Для общей стоимости используйте `SUPERLOOP_PCT` из режима 3. Отношение `CHECKS_PCT / SUPERLOOP_PCT` можно вычислить вне STM32-кода как приблизительную оценку доли общего Superloop overhead, объясняемую readiness-проверками и обслуживанием releases.

## Задачи

В интегрированном режиме по умолчанию включены три синтетические задачи:

| Задача | Период |
| --- | --- |
| IMU | 10 ms |
| LiDAR | 50 ms |
| Camera | 200 ms |

Их время выполнения зависит от `Uxx`. Датчики HC-SR04 и DHT11 сейчас выключены (`ENABLE_REAL_TAU1` и `ENABLE_REAL_TAU2` равны `0`), поэтому внешние датчики не нужны.

Профили Superloop используют другой набор: `tau1` с периодом 10 ms и `tau2` с периодом 50 ms. Их нагрузки задаются макросами `MINIMAL_TAU1_WORKLOAD_US` и `MINIMAL_TAU2_WORKLOAD_US`.

## Структура репозитория

| Путь | Содержимое |
| --- | --- |
| [`Core/`](Core/) | Код приложения, включая [`main.c`](Core/Src/main.c), обработчики прерываний и системные файлы. |
| [`Drivers/`](Drivers/) | HAL и CMSIS для STM32F7. |
| [`docs/references/`](docs/references/) | Научные статьи и использованные источники. |
| [`docs/presentation/`](docs/presentation/) | Презентация и рисунки системной модели. |
| [`notebooks/`](notebooks/) | Colab notebook с обработкой результатов и графиками. |
| [`demonstration.ioc`](demonstration.ioc) | Конфигурация CubeMX: периферия, выводы и тактирование. |
| [`.project`](.project), [`.cproject`](.cproject), [`.mxproject`](.mxproject), [`.settings/`](.settings/) | Файлы STM32CubeIDE/Eclipse для импорта проекта. |
| [`demonstration Debug.launch`](<demonstration Debug.launch>) | Конфигурация запуска и отладки IDE. |

`Debug/` и `Release/` — генерируемые результаты сборки; `node_modules/` — зависимости JavaScript. Они игнорируются Git и не должны добавляться в репозиторий.

## Типичные проблемы

| Симптом | Что проверить |
| --- | --- |
| Нет COM-порта | Кабель должен передавать данные; проверьте ST-LINK и драйвер в «Диспетчере устройств». |
| Нечитаемый текст | Установите `115200`, `8N1`, `Parity=None`, `Flow control=None`. |
| Нет вывода | Откройте правильный COM-порт до запуска; в режимах 3 и 4 дождитесь 60 секунд; затем перезапустите плату. |
| Проект не собирается | Импортируйте корень репозитория и установите STM32Cube FW F7 в STM32CubeIDE. |
| Не прошивается плата | Закройте программы, занявшие ST-LINK/COM-порт, переподключите плату и повторите `Debug`. |

## Дополнительные материалы

- [Презентация проекта](https://docs.google.com/presentation/d/1G00Xcs1oBs7-omI5czPnljay-vytueP_O6MEM0KGgIo/edit?usp=sharing)
- [Google Colab: графики и обработка результатов](https://colab.research.google.com/drive/1k7JMMJAk3bt3nKziyVm84fFln8bctSOF?usp=sharing)
