# Структура исходного кода прошивки

[English version](README.md)

`Core/` содержит код конкретного приложения. STM32CubeIDE/CubeMX отвечает за
startup-код, инициализацию периферии, linker-интеграцию и конфигурацию HAL;
логика планирования и экспериментов разложена на небольшие модули.

## Конфигурация

Для ручного запуска изменяйте [`Inc/app_config.h`](Inc/app_config.h): алгоритм,
режим эксперимента, U-сценарий, окна, включённые задачи и размер EDF chunk.
[`Inc/experiment_config.h`](Inc/experiment_config.h) предназначен только для
временных override от automation runner. Периоды, таблицы нагрузки и
производные значения utilisation находятся в
[`Inc/workload_config.h`](Inc/workload_config.h).

## Модули

| Область | Файлы | Назначение |
| --- | --- | --- |
| Точка входа | `Src/main.c` | CubeMX user sections, запуск периферии, объекты задач, верхнеуровневый цикл планировщика и app-specific UART summary adapter. |
| Время и UART | `platform_time.*`, `app_uart.*` | Настройка DWT, микросекундная шкала времени, синтетическая работа и форматирование UART. |
| Датчики | `hcsr04.*`, `dht11.*` | Блокирующий доступ Superloop и неблокирующие state machine Chunked EDF. |
| Планирование | `edf_selector.*`, `edf_executor.*`, `scheduler_release.*`, `scheduler_types.h` | EDF-выбор/исполнение, обслуживание release и общие типы планировщика. |
| Измерения | `task_stats.*`, `cycle_metrics.*`, `profile_samples.*`, `task_reporting.*` | Статистика времени задач, cycle-агрегаты, выборки, histogram и CSV. |
| Режимы экспериментов | `isolated_profile.*`, `minimal_superloop_profile.*`, `superloop_checks_profile.*`, `superloop_scalability_profile.*` | Изолированные, чистые, диагностические и scalability-профили. |
| Вспомогательный код | `task_init.*`, `debug_status.*` | Инициализация периодических задач и live UART status. |

## Безопасность CubeMX

При повторной генерации изменяйте только области `USER CODE BEGIN` и `USER
CODE END` в файлах, которыми управляет CubeMX, например в `Src/main.c`.
Перечисленные самостоятельные модули CubeMX не генерирует и не должен менять.

После генерации соберите проект и убедитесь, что include-файлы в секции `USER
CODE` файла `main.c` сохранились.
