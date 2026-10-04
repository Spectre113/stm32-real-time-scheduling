# Configuration reference

Periodic test tasks are `tau1`, `tau2`, `tau3` (periods 10, 50, 200 ms). Their budgets are `TAU1_WORKLOAD_US`, `TAU2_WORKLOAD_US`, `TAU3_WORKLOAD_US`; switches are `ENABLE_TAU1`, `ENABLE_TAU2`, `ENABLE_TAU3`; CSV labels are `TAU1`, `TAU2`, `TAU3`. Physical sensors are separate: `hcsr04_task` / `ENABLE_HCSR04` and `dht11_task` / `ENABLE_DHT11`. SuperLoop executes tasks in `tau1 -> tau2 -> tau3` order. EDF selects the earliest absolute deadline; ties retain the original experiments' `tau2 -> tau1 -> tau3` order.

New CSV files use `TAU_TASKS`, `SENSOR_TASKS`, `TAU_U_X10000`. Analysis of old archives requires explicit name conversion; firmware and runner contain no legacy aliases.

[Русская версия](configuration.ru.md)

Edit [`app_config.h`](../Core/Inc/app_config.h), then rebuild and flash. [`experiment_config.h`](../Core/Inc/experiment_config.h) is used only for temporary automation-runner overrides and normally remains unchanged.

The [firmware source map](../Core/README.md) explains the role of each source module and the CubeMX-safe editing boundary.

Window and chunk macros ending in `_US` use microseconds and the `ULL` suffix. For comparable measurements, use the same scenario and measurement window in every compared run.

## Main parameters

| Macro                                                            | Meaning                                                                                      |
| ---------------------------------------------------------------- | -------------------------------------------------------------------------------------------- |
| `WORKLOAD_SCENARIO`                                              | Synthetic workload scenario from `U50` to `U110`.                                            |
| `SCHED_ALGO`                                                     | Scheduler for integrated mode: Superloop or Chunked EDF. Modes 3-6 use their own Superloops. |
| `EXPERIMENT_MODE`                                                | Collected statistics and experiment behavior.                                                |
| `INTEGRATED_TASK_COUNT`                                    | Two tasks (tau1 + tau2) or three (adds tau3) in integrated mode.                           |
| `PROFILE_WINDOW_US`                                              | Integrated-profile measurement window.                                                       |
| `MINIMAL_PROFILE_WINDOW_US`                                      | Window for modes 3 and 4.                                                                    |
| `SCALABILITY_PROFILE_WINDOW_US`                                  | Window for modes 5 and 6.                                                                    |
| `SCALABILITY_TASK_COUNT`                                         | Two, three, or four synthetic tasks in modes 5 and 6.                                        |
| `EDF_CHUNK_US`                                                   | Synthetic-work chunk size for Chunked EDF.                                                   |
| `SCHEDULER_MODE`                                                 | Integrated scheduler idle policy: busy polling or WFI. Modes 3 and 4 always busy-poll.       |
| `ENABLE_TAU1`, `ENABLE_TAU2`, `ENABLE_TAU3` | Enable optional synthetic integrated tasks. tau3 follows `INTEGRATED_TASK_COUNT`.    |
| `ENABLE_HCSR04`, `ENABLE_DHT11`                           | Enable HC-SR04 and DHT11. Use `0` when the corresponding sensor is absent.                   |
| `ENABLE_DEBUG_PRINT`                                             | Periodic diagnostic UART output. Keep `0` for measurements.                                  |
| `ENABLE_POLLING_PROFILE`                                         | Full polling statistics in integrated mode.                                                  |

`ENABLE_EXTENDED_STATS` defaults to 0 in integrated mode and 1 in other modes. At 0, response sum/max, execution totals, completion/miss/skip counts, scheduler/polling sums and counts remain enabled. Histograms, stored execution samples, execution/cycle min/max and response minimum are disabled. At 1, these diagnostics and the detailed UART report are enabled. Automated matrices select this through `extended_stats`, keep the same setting across compared runs.

## Workload scenarios

`U` is the target total utilization of synthetic tasks.

| Constant                 | Target utilization |
| ------------------------ | ------------------ |
| `WORKLOAD_SCENARIO_U50`  | 50%                |
| `WORKLOAD_SCENARIO_U65`  | 65%                |
| `WORKLOAD_SCENARIO_U75`  | 75%                |
| `WORKLOAD_SCENARIO_U80`  | 80%                |
| `WORKLOAD_SCENARIO_U90`  | 90%                |
| `WORKLOAD_SCENARIO_U95`  | 95%                |
| `WORKLOAD_SCENARIO_U100` | 100%               |
| `WORKLOAD_SCENARIO_U110` | 110%, overloaded   |

Minimal Superloop modes 3 and 4 accept `U50`-`U100`. Scalability modes 5 and 6 accept `U65` and `U90` and use 2, 3, or 4 tasks with periods of 10, 20, 50, and 100 ms.

## Scheduling algorithms

| Constant                 | Behavior                                                                                                                                                              |
| ------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `SCHED_ALGO_SUPERLOOP`   | Ready tasks execute in fixed non-preemptive order.                                                                                                                    |
| `SCHED_ALGO_CHUNKED_EDF` | Synthetic work is split into `EDF_CHUNK_US` chunks and the scheduler chooses again after each chunk. This is cooperative scheduling, not interrupt-driven preemption. |

HC-SR04 uses its staged EXTI path in Chunked EDF rather than artificial synthetic chunks. See the [sensor guide](sensors.md).

## Experiment modes

| Constant                                        | Purpose                                                                                                                               |
| ----------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------- |
| `EXPERIMENT_INTEGRATED` (`0`)                   | Main experiment with task and scheduler statistics: execution, response time, deadline misses, skipped releases, and related metrics. |
| `EXPERIMENT_ISOLATED_HCSR04` (`1`)                | Isolated execution of the physical HC-SR04 sensor task.                                                                                                 |
| `EXPERIMENT_ISOLATED_DHT11` (`2`)                | Isolated execution of the physical DHT11 sensor task.                                                                                                |
| `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` (`3`)    | Clean two-task busy-polling Superloop profile. `SUPERLOOP_PCT` is the part of the window outside synthetic task bodies.               |
| `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` (`4`)     | DWT-instrumented readiness-check and release-maintenance profile.                                                                     |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN` (`5`)  | Clean 2/3/4-task busy-polling Superloop scalability profile.                                                                          |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS` (`6`) | Diagnostic 2/3/4-task scalability profile with check instrumentation.                                                                 |

Modes 3-6 ignore `SCHED_ALGO`. Modes 5 and 6 also use their own fixed-order busy-polling Superloops.

Interpret profiles as follows:

- Integrated mode measures observed timing behavior, without proving schedulability: response time, deadline misses, skipped releases, and task statistics.
- Minimal Superloop mode measures cleaned busy-polling cost. `SUPERLOOP_PCT` includes polling/check logic and minimal experiment instrumentation, there is no physical sleep or idle state.
- Checks mode deliberately adds DWT instrumentation, so `CHECKS_PCT` is not clean total Superloop cost. Use mode 3's `SUPERLOOP_PCT` for that value. `CHECKS_PCT / SUPERLOOP_PCT` is only an approximate offline estimate of the share explained by readiness checks and release maintenance.

## Tasks and CSV output

Integrated synthetic tasks use these periods: tau1 10 ms, tau2 50 ms, tau3 200 ms. Their execution time depends on `Uxx`. Physical tasks use HC-SR04 at 100 ms and DHT11 at 2000 ms. Minimal Superloop profiles use a separate pair: `tau1` at 10 ms and `tau2` at 50 ms, controlled by `MINIMAL_TAU1_WORKLOAD_US` and `MINIMAL_TAU2_WORKLOAD_US`.

Integrated mode prints `CSV_RUN,...` and `CSV_TASK,...` after the window. Other modes print their documented CSV row. Open the terminal before reset, then save the resulting line for the Colab notebook or another analysis tool.

## Integrated CSV and cutoff

One window is measured per reset, after the report the firmware waits for reset/reflash. The cutoff timestamp is taken before UART output and shared by task rows. WINDOW_US is the actual duration, REQUESTED_WINDOW_US is the configured target. The runner stores the latter as DEVICE_WINDOW_TARGET_US alongside its own requested_window_us.

RUNS counts completed jobs. RESP_AVG_US and RESP_MAX_US describe release-to-completion response times of completed jobs, MISSES counts their strictly exceeded deadlines. SKIPPED counts releases skipped by the completion-time release-advance rule. FAILURES sums MISSES and SKIPPED, MAX_LATENESS_US is maximum observed positive lateness among completed jobs. EXEC_AVG_US covers completed jobs, TASK_EXEC includes execution already spent on unfinished jobs.

PENDING (0/1) identifies the released, unfinished tracked job, including one not started. PENDING_OVERDUE marks a strictly exceeded deadline at cutoff, PENDING_AGE_US is its age and PENDING_EXEC_US its execution so far. These fields do not count subsequent releases awaiting skip accounting. Do not include pending jobs in mean response time or silently treat them as successful completions.

SCHED_OVH and POLL_OVH measure instrumented code regions and must not be added blindly: selection work can be included in both. BUSY/IDLE are derived accounting values, not measured physical CPU activity/sleep. Campaign files and commands are documented in the [runner guide](../tools/experiment_runner/README.md).
