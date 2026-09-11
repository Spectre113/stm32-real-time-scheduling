# Configuration reference

[Русская версия](configuration.ru.md)

Edit the documented block at the top of [`Core/Src/main.c`](../Core/Src/main.c), then rebuild and flash. [`Core/Inc/experiment_config.h`](../Core/Inc/experiment_config.h) is used only for temporary automation-runner overrides and normally remains unchanged.

All time values are microseconds and use the `ULL` suffix. For comparable measurements, use the same scenario and measurement window in every compared run.

## Main parameters

| Macro | Meaning |
| --- | --- |
| `WORKLOAD_SCENARIO` | Synthetic workload scenario from `U50` to `U110`. |
| `SCHED_ALGO` | Scheduler for integrated mode: Superloop or Chunked EDF. Modes 3-6 use their own Superloops. |
| `EXPERIMENT_MODE` | Collected statistics and experiment behavior. |
| `INTEGRATED_SYNTH_TASK_COUNT` | Two tasks (IMU + LiDAR) or three (adds Camera) in integrated mode. |
| `PROFILE_WINDOW_US` | Integrated-profile measurement window. |
| `MINIMAL_PROFILE_WINDOW_US` | Window for modes 3 and 4. |
| `SCALABILITY_PROFILE_WINDOW_US` | Window for modes 5 and 6. |
| `SCALABILITY_TASK_COUNT` | Two, three, or four synthetic tasks in modes 5 and 6. |
| `EDF_CHUNK_US` | Synthetic-work chunk size for Chunked EDF. |
| `SCHEDULER_MODE` | Integrated scheduler idle policy: busy polling or WFI. Modes 3 and 4 always busy-poll. |
| `ENABLE_SYNTH_IMU`, `ENABLE_SYNTH_LIDAR`, `ENABLE_SYNTH_CONTROL` | Enable optional synthetic integrated tasks. Camera follows `INTEGRATED_SYNTH_TASK_COUNT`. |
| `ENABLE_REAL_TAU1`, `ENABLE_REAL_TAU2` | Enable HC-SR04 and DHT11. Use `0` when the corresponding sensor is absent. |
| `ENABLE_DEBUG_PRINT` | Periodic diagnostic UART output. Keep `0` for measurements. |
| `ENABLE_POLLING_PROFILE` | Full polling statistics in integrated mode. |

## Workload scenarios

`U` is the target total utilization of synthetic tasks.

| Constant | Target utilization |
| --- | --- |
| `WORKLOAD_SCENARIO_U50` | 50% |
| `WORKLOAD_SCENARIO_U65` | 65% |
| `WORKLOAD_SCENARIO_U75` | 75% |
| `WORKLOAD_SCENARIO_U80` | 80% |
| `WORKLOAD_SCENARIO_U90` | 90% |
| `WORKLOAD_SCENARIO_U95` | 95% |
| `WORKLOAD_SCENARIO_U100` | 100% |
| `WORKLOAD_SCENARIO_U110` | 110%, overloaded |

Minimal Superloop modes 3 and 4 accept `U50`-`U100`. Scalability modes 5 and 6 accept `U65` and `U90` and use 2, 3, or 4 tasks with periods of 10, 20, 50, and 100 ms.

## Scheduling algorithms

| Constant | Behavior |
| --- | --- |
| `SCHED_ALGO_SUPERLOOP` | Ready tasks execute in fixed non-preemptive order. |
| `SCHED_ALGO_CHUNKED_EDF` | Synthetic work is split into `EDF_CHUNK_US` chunks and the scheduler chooses again after each chunk. This is cooperative scheduling, not interrupt-driven preemption. |

HC-SR04 uses its staged EXTI path in Chunked EDF rather than artificial synthetic chunks. See the [sensor guide](sensors.md).

## Experiment modes

| Constant | Purpose |
| --- | --- |
| `EXPERIMENT_INTEGRATED` (`0`) | Main experiment with task and scheduler statistics: execution, response time, deadline misses, skipped releases, and related metrics. |
| `EXPERIMENT_ISOLATED_TAU1` (`1`) | Isolated execution of the first task. |
| `EXPERIMENT_ISOLATED_TAU2` (`2`) | Isolated execution of the second task. |
| `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` (`3`) | Clean two-task busy-polling Superloop profile. `SUPERLOOP_PCT` is the part of the window outside synthetic task bodies. |
| `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` (`4`) | DWT-instrumented readiness-check and release-maintenance profile. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN` (`5`) | Clean 2/3/4-task busy-polling Superloop scalability profile. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS` (`6`) | Diagnostic 2/3/4-task scalability profile with check instrumentation. |

Modes 3-6 ignore `SCHED_ALGO`. Modes 5 and 6 also use their own fixed-order busy-polling Superloops.

Interpret profiles as follows:

- Integrated mode answers schedulability questions: response time, deadline misses, skipped releases, and task statistics.
- Minimal Superloop mode measures cleaned busy-polling cost. `SUPERLOOP_PCT` includes polling/check logic and minimal experiment instrumentation; there is no physical sleep or idle state.
- Checks mode deliberately adds DWT instrumentation, so `CHECKS_PCT` is not clean total Superloop cost. Use mode 3's `SUPERLOOP_PCT` for that value. `CHECKS_PCT / SUPERLOOP_PCT` is only an approximate offline estimate of the share explained by readiness checks and release maintenance.

## Tasks and CSV output

Integrated synthetic tasks use these periods: IMU 10 ms, LiDAR 50 ms, Camera 200 ms. Their execution time depends on `Uxx`. Physical tasks use HC-SR04 at 100 ms and DHT11 at 2000 ms. Minimal Superloop profiles use a separate pair: `tau1` at 10 ms and `tau2` at 50 ms, controlled by `MINIMAL_TAU1_WORKLOAD_US` and `MINIMAL_TAU2_WORKLOAD_US`.

Integrated mode prints `CSV_RUN,...` and `CSV_TASK,...` after the window. Other modes print their documented CSV row. Open the terminal before reset, then save the resulting line for the Colab notebook or another analysis tool.
