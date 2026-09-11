# Справочник по конфигурации

[English version](configuration.md)

Изменяйте документированный блок в начале [`Core/Src/main.c`](../Core/Src/main.c), затем соберите и прошейте проект. [`Core/Inc/experiment_config.h`](../Core/Inc/experiment_config.h) используется только для временных override автоматического runner и обычно не изменяется.

Все значения времени задаются в микросекундах и имеют суффикс `ULL`. Для сопоставимых измерений используйте одинаковые сценарий и окно измерения во всех сравниваемых запусках.

## Основные параметры

| Макрос | Назначение |
| --- | --- |
| `WORKLOAD_SCENARIO` | Сценарий синтетической нагрузки от `U50` до `U110`. |
| `SCHED_ALGO` | Алгоритм для integrated-режима: Superloop или Chunked EDF. Режимы 3-6 используют собственные Superloop. |
| `EXPERIMENT_MODE` | Поведение эксперимента и собираемая статистика. |
| `INTEGRATED_SYNTH_TASK_COUNT` | Две задачи (IMU + LiDAR) или три (добавляется Camera) в integrated-режиме. |
| `PROFILE_WINDOW_US` | Окно измерения integrated-профиля. |
| `MINIMAL_PROFILE_WINDOW_US` | Окно для режимов 3 и 4. |
| `SCALABILITY_PROFILE_WINDOW_US` | Окно для режимов 5 и 6. |
| `SCALABILITY_TASK_COUNT` | Две, три или четыре синтетические задачи в режимах 5 и 6. |
| `EDF_CHUNK_US` | Размер части синтетической работы для Chunked EDF. |
| `SCHEDULER_MODE` | Политика ожидания integrated-планировщика: busy polling или WFI. Режимы 3 и 4 всегда busy-polling. |
| `ENABLE_SYNTH_IMU`, `ENABLE_SYNTH_LIDAR`, `ENABLE_SYNTH_CONTROL` | Включение дополнительных синтетических integrated-задач. Camera определяется `INTEGRATED_SYNTH_TASK_COUNT`. |
| `ENABLE_REAL_TAU1`, `ENABLE_REAL_TAU2` | Включение HC-SR04 и DHT11. При отсутствии конкретного датчика задайте `0`. |
| `ENABLE_DEBUG_PRINT` | Периодический диагностический UART-вывод. Для измерений оставляйте `0`. |
| `ENABLE_POLLING_PROFILE` | Полная polling-статистика в integrated-режиме. |

## Сценарии нагрузки

`U` - целевая суммарная утилизация синтетических задач.

| Константа | Целевая утилизация |
| --- | --- |
| `WORKLOAD_SCENARIO_U50` | 50% |
| `WORKLOAD_SCENARIO_U65` | 65% |
| `WORKLOAD_SCENARIO_U75` | 75% |
| `WORKLOAD_SCENARIO_U80` | 80% |
| `WORKLOAD_SCENARIO_U90` | 90% |
| `WORKLOAD_SCENARIO_U95` | 95% |
| `WORKLOAD_SCENARIO_U100` | 100% |
| `WORKLOAD_SCENARIO_U110` | 110%, перегрузка |

Минимальные Superloop-режимы 3 и 4 принимают `U50`-`U100`. Scalability-режимы 5 и 6 принимают `U65` и `U90`, используют 2, 3 или 4 задачи с периодами 10, 20, 50 и 100 ms.

## Алгоритмы планирования

| Константа | Поведение |
| --- | --- |
| `SCHED_ALGO_SUPERLOOP` | Готовые задачи выполняются в фиксированном невытесняющем порядке. |
| `SCHED_ALGO_CHUNKED_EDF` | Синтетическая работа делится на части `EDF_CHUNK_US`, а после каждой части планировщик снова делает выбор. Это кооперативное планирование, а не аппаратное вытеснение прерыванием. |

В Chunked EDF HC-SR04 использует отдельный staged EXTI-путь, а не искусственные синтетические chunks. См. [руководство по датчикам](sensors.ru.md).

## Режимы экспериментов

| Константа | Назначение |
| --- | --- |
| `EXPERIMENT_INTEGRATED` (`0`) | Основной эксперимент со статистикой задач и планировщика: выполнение, response time, deadline misses, skipped releases и связанные метрики. |
| `EXPERIMENT_ISOLATED_TAU1` (`1`) | Изолированное выполнение первой задачи. |
| `EXPERIMENT_ISOLATED_TAU2` (`2`) | Изолированное выполнение второй задачи. |
| `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` (`3`) | Чистый двухзадачный busy-polling Superloop. `SUPERLOOP_PCT` - доля окна вне тел синтетических задач. |
| `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` (`4`) | DWT-профиль readiness-проверок и обслуживания releases. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN` (`5`) | Чистый scalability-профиль busy-polling Superloop с 2, 3 или 4 задачами. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS` (`6`) | Диагностический scalability-профиль 2/3/4 задач с инструментированием checks. |

Режимы 3-6 игнорируют `SCHED_ALGO`. Режимы 5 и 6 также используют собственные Superloop с фиксированным порядком и busy-polling.

Интерпретируйте профили так:

- Integrated-режим отвечает на вопросы schedulability: response time, deadline misses, skipped releases и статистика задач.
- Minimal Superloop измеряет стоимость очищенного busy-polling цикла. `SUPERLOOP_PCT` включает polling/check logic и минимальную обвязку эксперимента. Физического sleep или idle здесь нет.
- Checks-режим намеренно добавляет DWT-инструментирование, поэтому `CHECKS_PCT` не является чистой полной стоимостью Superloop. Для неё используйте `SUPERLOOP_PCT` из режима 3. Отношение `CHECKS_PCT / SUPERLOOP_PCT` - лишь приблизительная внешняя оценка доли readiness checks и обслуживания releases.

## Задачи и CSV-вывод

Синтетические integrated-задачи используют периоды: IMU 10 ms, LiDAR 50 ms, Camera 200 ms. Их время выполнения зависит от `Uxx`. Физические задачи используют HC-SR04 с периодом 100 ms и DHT11 с периодом 2000 ms. Минимальные Superloop-профили используют отдельную пару: `tau1` с периодом 10 ms и `tau2` с периодом 50 ms. Их нагрузка задаётся `MINIMAL_TAU1_WORKLOAD_US` и `MINIMAL_TAU2_WORKLOAD_US`.

Integrated-режим после окна выводит `CSV_RUN,...` и `CSV_TASK,...`. Другие режимы выводят свой CSV-формат. Откройте терминал до reset, затем сохраните строку для Google Colab или другого инструмента анализа.
