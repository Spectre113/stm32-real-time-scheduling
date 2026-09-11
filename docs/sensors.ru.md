# Руководство по HC-SR04 и DHT11

[English version](sensors.md)

Физические задачи необязательны. Перед запуском без конкретного устройства задайте соответствующий `ENABLE_REAL_TAU1` или `ENABLE_REAL_TAU2` равным `0`. HC-SR04 - это `tau1` с периодом 100 ms, DHT11 - `tau2` с периодом 2000 ms.

| Устройство | Назначение пинов |
| --- | --- |
| HC-SR04 TRIG | `PB2` |
| HC-SR04 ECHO | `PC0`, EXTI0 на фронтах RISE и FALL |
| DHT11 data | `PA5` |

В текущей конфигурации платы UART использует `USART3`: `PD8` - TX, `PD9` - RX через встроенный ST-LINK Virtual COM Port.

## HC-SR04

Superloop сохраняет исходное блокирующее измерение как baseline. При `SCHED_ALGO_CHUNKED_EDF` HC-SR04 работает как staged-транзакция: короткий trigger pulse выполняется синхронно, а фронты ECHO RISE и FALL захватываются EXTI0. Ожидание остаётся active, но not runnable, поэтому может выполняться другая готовая задача. Execution time включает CPU-время trigger и finalization, а ожидание ECHO входит в response time.

| Состояние | Что означает | Статус для планировщика |
| --- | --- | --- |
| `IDLE` | Активного измерения нет. | Runnable, когда release наступил. |
| `TRIGGER` | Формируется pulse: 2 us LOW, затем 10 us HIGH. | Выполняется синхронно, затем переходит в `WAIT_ECHO_RISE`. |
| `WAIT_ECHO_RISE` | Ожидание фронта ECHO RISE на `PC0`. | Active, not runnable. EXTI0 сохраняет фронт; timeout делает finalization runnable. |
| `WAIT_ECHO_FALL` | Ожидание фронта ECHO FALL. | Active, not runnable. EXTI0 сохраняет фронт; timeout делает finalization runnable. |
| `COMPLETE` | Оба timestamp получены, можно вычислить расстояние. | Один раз runnable для finalization и статистики job. |
| `ERROR` | Ожидаемый фронт не пришёл до timeout. | Один раз runnable для публикации ошибки и завершения job. |

EXTI-обработчик только захватывает timestamps и меняет состояние. Он не выводит UART и не вычисляет расстояние. Если CubeMX регенерирует проект, сохраните `PC0` как `GPIO_EXTI0`, триггеры RISE/FALL с pulldown и вызов `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)` в сгенерированном `EXTI0_IRQHandler()`.

## DHT11

Superloop сохраняет блокирующий baseline с `HAL_Delay(30)`. В Chunked EDF DHT11 переводит `PA5` в LOW и входит в видимое планировщику состояние `WAIT_START_LOW` на 30 ms. Пока идёт ожидание, задача active, но not runnable. Затем response и все 40 бит выполняются одной atomic timing-critical транзакцией без искусственных EDF yield. CPU execution не включает пассивное ожидание, а response time включает.

| Состояние | Что означает | Статус для планировщика |
| --- | --- | --- |
| `IDLE` | Активной транзакции нет. | Runnable, когда release наступил. |
| `START_LOW` | `PA5` настраивается как output, переводится в LOW и сохраняется wake-up time. | Короткий setup, затем сразу `WAIT_START_LOW`. |
| `WAIT_START_LOW` | Идёт обязательный DHT11 start-low интервал. | Active, not runnable до `now_us >= wait_until_us`; без busy-wait и `HAL_Delay(30)`. |
| `READ_TRANSACTION` | `PA5` переводится в input, читаются response и 40 бит. | Runnable и atomic; микросекундный polling должен завершиться без EDF yield. |
| `DONE` | Checksum прошёл, значения обновлены. | Один раз завершает job и обновляет `tau2_runs` и статистику. |
| `ERROR` | Timeout ответа или ошибка checksum. | Один раз завершает job и публикует прежний код ошибки DHT11. |

## Короткие аппаратные smoke-тесты

Для 10-секундного запуска только HC-SR04:

```c
#define SCHED_ALGO SCHED_ALGO_CHUNKED_EDF
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
#define PROFILE_WINDOW_US 10000000ULL
#define ENABLE_REAL_TAU1 1
#define ENABLE_REAL_TAU2 0
#define ENABLE_SYNTH_IMU 0
#define ENABLE_SYNTH_LIDAR 0
#define ENABLE_SYNTH_CONTROL 0
#define ENABLE_DEBUG_PRINT 1
```

Для запуска только DHT11 оставьте те же настройки, но установите `ENABLE_REAL_TAU1` в `0`, а `ENABLE_REAL_TAU2` в `1`. Для измерений верните `ENABLE_DEBUG_PRINT` в `0`, так как UART-вывод меняет временные характеристики.
