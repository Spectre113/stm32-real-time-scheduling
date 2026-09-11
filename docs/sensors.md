# HC-SR04 and DHT11 guide

[Русская версия](sensors.ru.md)

The physical tasks are optional. Set `ENABLE_REAL_TAU1` or `ENABLE_REAL_TAU2` to `0` before running without the corresponding hardware. HC-SR04 is `tau1` with a 100 ms period; DHT11 is `tau2` with a 2000 ms period.

| Device | Pin assignment |
| --- | --- |
| HC-SR04 TRIG | `PB2` |
| HC-SR04 ECHO | `PC0`, EXTI0 on rising and falling edges |
| DHT11 data | `PA5` |

In the current board configuration, UART uses `USART3`: `PD8` TX and `PD9` RX through the onboard ST-LINK Virtual COM Port.

## HC-SR04

Superloop keeps the original blocking measurement as a baseline. With `SCHED_ALGO_CHUNKED_EDF`, HC-SR04 is a staged transaction: its short trigger pulse runs synchronously, while ECHO rise and fall are captured by EXTI0. Waiting is active but not runnable, so another ready task can run. Execution time contains trigger and finalization CPU time; ECHO waiting contributes to response time.

| State | Meaning | Scheduler status |
| --- | --- | --- |
| `IDLE` | No active measurement. | Runnable when the release is due. |
| `TRIGGER` | Emit 2 us LOW then 10 us HIGH pulse. | Runs synchronously, then enters `WAIT_ECHO_RISE`. |
| `WAIT_ECHO_RISE` | Wait for ECHO rising edge on `PC0`. | Active, not runnable. EXTI0 records the edge; timeout makes finalization runnable. |
| `WAIT_ECHO_FALL` | Wait for ECHO falling edge. | Active, not runnable. EXTI0 records the edge; timeout makes finalization runnable. |
| `COMPLETE` | Both timestamps are ready and distance can be calculated. | Runnable once for finalization and whole-job statistics. |
| `ERROR` | An expected edge timed out. | Runnable once to publish the error and complete the job. |

The EXTI handler captures timestamps and changes state only. It does not print UART messages or calculate distance. If CubeMX regenerates the project, retain `PC0` as `GPIO_EXTI0`, rising/falling edge triggering with pulldown, and the generated `EXTI0_IRQHandler()` call to `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)`.

## DHT11

Superloop preserves the blocking `HAL_Delay(30)` baseline. With Chunked EDF, DHT11 drives `PA5` LOW and enters a scheduler-visible 30 ms `WAIT_START_LOW` state. It is active but not runnable while waiting. The response and all 40 data bits then run as one atomic timing-critical transaction, without artificial EDF yields. CPU execution excludes the passive wait; response time includes it.

| State | Meaning | Scheduler status |
| --- | --- | --- |
| `IDLE` | No active transaction. | Runnable when the release is due. |
| `START_LOW` | Configure `PA5` as output, drive LOW, store the wake-up time. | Short setup, then immediately enters `WAIT_START_LOW`. |
| `WAIT_START_LOW` | Required DHT11 start-low interval is elapsing. | Active, not runnable until `now_us >= wait_until_us`; no busy-wait or `HAL_Delay(30)`. |
| `READ_TRANSACTION` | Release `PA5` to input and read the response plus 40 bits. | Runnable and atomic; microsecond polling must finish without an EDF yield. |
| `DONE` | Checksum passed and values were updated. | Completes once and updates `tau2_runs` and statistics. |
| `ERROR` | Response timeout or checksum error. | Completes once and publishes the existing DHT11 error code. |

## Short hardware smoke tests

For a 10-second HC-SR04-only run:

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

For a DHT11-only run, use the same settings but set `ENABLE_REAL_TAU1` to `0` and `ENABLE_REAL_TAU2` to `1`. Restore `ENABLE_DEBUG_PRINT` to `0` for measurements because UART output changes timing.
