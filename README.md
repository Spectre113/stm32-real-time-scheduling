# Real-Time Scheduler Demonstration on STM32F767

[Русская версия](README.ru.md)

STM32CubeIDE project for studying real-time scheduling algorithms on an `STM32F767ZITx`: a non-preemptive Superloop and Chunked EDF that cooperatively splits synthetic work into chunks. The firmware runs synthetic periodic tasks, measures their execution, and writes results to the serial port.

By default, the clean Superloop profile runs scenario `U65` for 60 seconds. It then prints a report and a CSV row to UART.

## Requirements

- `NUCLEO-F767ZI` board with onboard ST-LINK and Virtual COM Port.
- A data-capable USB cable.
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) with STM32Cube FW F7. The project was created with STM32CubeMX 6.16.1.
- A UART terminal such as [PuTTY](https://www.putty.org/) or Tera Term.

On Windows, STM32CubeIDE normally installs the ST-LINK driver. If no virtual COM port appears in Device Manager, update the ST-LINK driver and reconnect the board.

## Quick start

1. Clone the repository or extract its archive.
2. In STM32CubeIDE, choose `File → Import… → General → Existing Projects into Workspace`, select the repository root, and finish the import.
3. Connect the board through the ST-LINK USB connector.
4. Build the project with `Project → Build All`.
5. Open the UART terminal before starting the firmware.
6. Click `Run` or `Debug` in STM32CubeIDE. The IDE programs the MCU through ST-LINK and starts it.

### PuTTY setup

1. In `Device Manager → Ports (COM & LPT)`, find the `ST-LINK Virtual COM Port` number, for example `COM5`.
2. In PuTTY select `Session → Connection type: Serial`, enter that COM port and `115200` baud.
3. In `Connection → Serial`, use:

   | Setting | Value |
   | --- | --- |
   | Speed | `115200` |
   | Data bits | `8` |
   | Stop bits | `1` |
   | Parity | `None` |
   | Flow control | `None` |

4. Click `Open`, then start the program on the board.

The project uses `USART3`: `PD8` is TX and `PD9` is RX. On `NUCLEO-F767ZI`, these pins connect to the onboard ST-LINK Virtual COM Port; see [`demonstration.ioc`](demonstration.ioc). When moving to another board, verify the physical connection and update the UART pins in the `.ioc` file if required. UART is used only for logging and is never called inside a profiling window.

## Current manual configuration

The default switches are at the top of [`Core/Src/main.c`](Core/Src/main.c):

```c
#define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U50
#define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
```

| Setting | Default |
| --- | --- |
| Scheduler | `SCHED_ALGO_SUPERLOOP` |
| Scenario | `U50` |
| Experiment mode | `EXPERIMENT_INTEGRATED` |
| Measurement window | `60,000,000 us` (60 seconds) |

The integrated mode prints `CSV_RUN,...` and `CSV_TASK,...` rows after its window. Other modes print the CSV row described in their mode entry below. Save the CSV output to a file or spreadsheet for processing. If PuTTY was opened after the firmware started, restart the program: it may have already emitted its only result.

## Automated experiment series

To run many scenarios and windows without manually changing macros or using PuTTY, use [`tools/experiment_runner/`](tools/experiment_runner/). The Python script builds and flashes each configuration, reads the ST-LINK Virtual COM Port, saves a separate UART log, and combines CSV rows in `results/<timestamp>/summary.csv`. During an automated run, temporary overrides disable physical sensors and debug UART output, and enable the standard synthetic tasks; your manual switches in `main.c` remain unchanged after the run.

The supplied matrix contains `U50`-`U100`, windows of 10/30/60/100/250/500/1000 seconds, and Clean/Checks modes: 84 runs in total. Separate matrices cover Superloop scalability (12 runs) and full-statistics integrated Superloop profiling (24 runs), without repeating the default campaign. See the [automation guide](tools/experiment_runner/README.md) for setup and commands. Results are ignored by Git; the temporary CubeIDE headless workspace is created outside the repository.

## Configuring an experiment

Edit the documented configuration block at the top of [`Core/Src/main.c`](Core/Src/main.c), then rebuild and reflash the board. [`Core/Inc/experiment_config.h`](Core/Inc/experiment_config.h) is reserved for temporary overrides from the automation runner and normally stays unchanged.

### Main parameters

| Macro | What it controls |
| --- | --- |
| `WORKLOAD_SCENARIO` | Workload scenario `U50`-`U110`. |
| `SCHED_ALGO` | Scheduler of the integrated experiment: Superloop or Chunked EDF. It is not used by modes 3-6. |
| `EXPERIMENT_MODE` | Run type and collected statistics. |
| `INTEGRATED_SYNTH_TASK_COUNT` | Synthetic task count in the integrated mode: `2` (IMU + LiDAR) or `3` (IMU + LiDAR + Camera). |
| `MINIMAL_PROFILE_WINDOW_US` | Window for modes 3 and 4; `60000000ULL` (60 seconds) by default. |
| `SCALABILITY_PROFILE_WINDOW_US` | Window for scalability modes 5 and 6; `60000000ULL` (60 seconds) by default. |
| `SCALABILITY_TASK_COUNT` | Active synthetic task count in modes 5 and 6: only `2`, `3`, or `4`. |
| `PROFILE_WINDOW_US` | Window for integrated profiling; `60000000ULL` (60 seconds) in the current manual configuration. |
| `EDF_CHUNK_US` | Synthetic-work chunk size in Chunked EDF. |
| `SCHEDULER_MODE` | Idle policy of the integrated scheduler: busy polling or WFI. Modes 3 and 4 always use busy polling. |
| `ENABLE_SYNTH_IMU`, `ENABLE_SYNTH_LIDAR`, `ENABLE_SYNTH_CONTROL` | Enable optional synthetic tasks in the integrated mode; Camera follows `INTEGRATED_SYNTH_TASK_COUNT`. |
| `ENABLE_REAL_TAU1`, `ENABLE_REAL_TAU2` | Enable the HC-SR04 and DHT11 tasks; set each to `0` when that sensor is not connected. |
| `ENABLE_DEBUG_PRINT` | Enable periodic debug UART output. Keep it at `0` during measurements. |
| `ENABLE_POLLING_PROFILE` | Enable full polling statistics for the integrated experiment. |

All time values are microseconds and use the `ULL` suffix. Rebuild and reflash after changing any macro. For comparable measurements, use the same scenario and window duration for each mode.

### Workload scenarios

`U` is the target total utilization of the synthetic tasks.

| Constant | Utilization |
| --- | --- |
| `WORKLOAD_SCENARIO_U50` | 50% |
| `WORKLOAD_SCENARIO_U65` | 65% |
| `WORKLOAD_SCENARIO_U75` | 75% |
| `WORKLOAD_SCENARIO_U80` | 80% |
| `WORKLOAD_SCENARIO_U90` | 90% |
| `WORKLOAD_SCENARIO_U95` | 95% |
| `WORKLOAD_SCENARIO_U100` | 100% |
| `WORKLOAD_SCENARIO_U110` | 110%, overloaded |

Only `U50`-`U100` are available in Superloop profile modes 3 and 4. `U110` is supported by the other modes.

Scalability modes 5 and 6 accept only `U65` and `U90`; they use 2, 3, or 4 tasks with periods of 10, 20, 50, and 100 ms respectively.

### Scheduling algorithms

| Constant | Purpose |
| --- | --- |
| `SCHED_ALGO_SUPERLOOP` | Fixed non-preemptive order of ready tasks. |
| `SCHED_ALGO_CHUNKED_EDF` | EDF with synthetic work split into `EDF_CHUNK_US` chunks; the scheduler chooses again after each chunk. HC-SR04 uses a separate staged EXTI path rather than fake synthetic chunks. This is cooperative scheduling, not hardware preemption or interrupt-driven context switching. |

### HC-SR04 with Chunked EDF

When `ENABLE_REAL_TAU1` is `1` and `SCHED_ALGO` is `SCHED_ALGO_CHUNKED_EDF`, HC-SR04 uses a staged transaction: the short trigger pulse runs synchronously, while ECHO rise/fall are captured by EXTI0 on PC0. The active job is not runnable while the sensor waits, so the scheduler can run other ready work. Its execution statistic contains only trigger/finalization CPU time; ECHO wait time contributes only to response time. Superloop retains the original blocking HC-SR04 baseline.

| HC-SR04 state | Meaning | Scheduler status |
| --- | --- | --- |
| `IDLE` | No active measurement. A due release may start a new job. | Runnable when the release is due. |
| `TRIGGER` | The scheduler emits the 2 us LOW then 10 us HIGH trigger pulse. | Runs synchronously, then moves immediately to `WAIT_ECHO_RISE`. |
| `WAIT_ECHO_RISE` | The sensor has been triggered and the code waits for the ECHO rising edge on PC0. | Active, not runnable. EXTI0 records the edge; a timeout makes it runnable for error finalization. |
| `WAIT_ECHO_FALL` | The ECHO rising edge was captured; the code waits for the falling edge. | Active, not runnable. EXTI0 records the edge; a timeout makes it runnable for error finalization. |
| `COMPLETE` | Both ECHO timestamps are available, so the distance can be calculated. | Runnable once for finalization and whole-job statistics. |
| `ERROR` | The expected ECHO edge did not arrive before its timeout. | Runnable once to publish the error and complete the job. |

The EXTI handler only captures edge timestamps and changes the sensor state. It does not print UART messages or perform distance calculations. The original release and absolute deadline remain associated with the job from `TRIGGER` through `COMPLETE` or `ERROR`.

### DHT11 with Chunked EDF

When `ENABLE_REAL_TAU2` is `1` and `SCHED_ALGO` is `SCHED_ALGO_CHUNKED_EDF`, DHT11 drives PA5 LOW, then enters a scheduler-visible 30 ms `WAIT_START_LOW` state. It is active but not runnable during that interval. After the wait, the response and 40-bit transfer run as one atomic timing-critical step; they are not split into artificial EDF chunks. CPU execution statistics exclude the passive 30 ms wait, while response time includes it. Superloop retains the original blocking `HAL_Delay(30)` baseline.

| DHT11 state | Meaning | Scheduler status |
| --- | --- | --- |
| `IDLE` | No active DHT11 transaction. A due release starts a new job. | Runnable when the release is due. |
| `START_LOW` | PA5 is configured as output and driven LOW. The 30 ms wake-up time is stored. | A short setup action; it immediately enters `WAIT_START_LOW`. |
| `WAIT_START_LOW` | The required DHT11 start-low interval is elapsing. | Active, not runnable until `now_us >= wait_until_us`. No busy-wait or `HAL_Delay(30)` is used here. |
| `READ_TRANSACTION` | PA5 is released to input, then the DHT11 response and all 40 data bits are read. | Runnable and atomic. The microsecond polling loops must complete without an EDF yield. |
| `DONE` | The checksum passed and temperature/humidity were updated. | The job completes once; `tau2_runs` and statistics are updated once. |
| `ERROR` | A response timeout or checksum error occurred. | The job completes once and publishes the existing DHT11 error code. |

For DHT11, response time is measured from the original release until `DONE` or `ERROR`, so it includes the 30 ms start-low interval. CPU execution time accumulates only the short setup and transaction code, not passive waiting.

For a 10-second HC-SR04-only smoke test, edit the block at the top of `Core/Src/main.c` to:

```c
#define SCHED_ALGO SCHED_ALGO_CHUNKED_EDF
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
#define PROFILE_WINDOW_US 10000000ULL

#define ENABLE_REAL_TAU1 1
#define ENABLE_REAL_TAU2 0
#define ENABLE_SYNTH_IMU 0
#define ENABLE_SYNTH_LIDAR 0
#define ENABLE_SYNTH_CONTROL 0
#define INTEGRATED_SYNTH_TASK_COUNT 2
#define ENABLE_DEBUG_PRINT 1
```

`PC0` is configured as GPIO EXTI0 on both edges and EXTI0 IRQ priority 5. If CubeMX regenerates the project, retain PC0 as `GPIO_EXTI0`, rising/falling edge trigger with pulldown, and keep the generated `EXTI0_IRQHandler()` calling `HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0)`.

For a DHT11-only 10-second smoke test, use:

```c
#define SCHED_ALGO SCHED_ALGO_CHUNKED_EDF
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
#define PROFILE_WINDOW_US 10000000ULL

#define ENABLE_REAL_TAU1 0
#define ENABLE_REAL_TAU2 1
#define ENABLE_SYNTH_IMU 0
#define ENABLE_SYNTH_LIDAR 0
#define ENABLE_SYNTH_CONTROL 0
#define INTEGRATED_SYNTH_TASK_COUNT 2
#define ENABLE_DEBUG_PRINT 1
```

### Experiment modes

| Constant | Purpose |
| --- | --- |
| `EXPERIMENT_INTEGRATED` (`0`) | Main experiment with task and scheduler statistics: execution, response time, deadline misses, skipped releases, and other metrics. |
| `EXPERIMENT_ISOLATED_TAU1` (`1`) | Isolated execution of the first task. |
| `EXPERIMENT_ISOLATED_TAU2` (`2`) | Isolated execution of the second task. |
| `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` (`3`) | Clean two-task busy-polling Superloop. `SUPERLOOP_PCT` is the part of the window outside synthetic task bodies. |
| `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` (`4`) | DWT-instrumented profile of readiness checks and release maintenance. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN` (`5`) | Clean busy-polling Superloop scalability profile with 2, 3, or 4 synthetic tasks. |
| `EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS` (`6`) | Diagnostic scalability profile of readiness checks and release maintenance. |

In modes 3-6, `SCHED_ALGO` is not used: they run their own synthetic Superloop profiles. `SCHED_ALGO` affects `EXPERIMENT_INTEGRATED` only.

Modes 5 and 6 also use their own fixed-order busy-polling Superloop and do not use `SCHED_ALGO`.

The modes answer different questions and should not be mixed during interpretation:

- `EXPERIMENT_INTEGRATED` is for schedulability and task statistics: response time, deadline misses, skipped releases, and related metrics.
- `EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE` measures the overall cost of the cleaned busy-polling Superloop. It has no physical sleep or idle state; `SUPERLOOP_PCT` includes polling/check logic and minimal experiment instrumentation.
- `EXPERIMENT_SUPERLOOP_CHECKS_PROFILE` breaks down readiness checks and release maintenance, but deliberately adds DWT instrumentation overhead.

Therefore, mode 4's `CHECKS_PCT` is not the clean total Superloop cost. Use mode 3's `SUPERLOOP_PCT` for that value. `CHECKS_PCT / SUPERLOOP_PCT` can be calculated outside the firmware as an approximate share of overall Superloop overhead explained by readiness checks and release maintenance.

## Tasks

The integrated mode enables three synthetic tasks by default:

| Task | Period |
| --- | --- |
| IMU | 10 ms |
| LiDAR | 50 ms |
| Camera | 200 ms |

Their execution time depends on `Uxx`. HC-SR04 and DHT11 are currently disabled (`ENABLE_REAL_TAU1` and `ENABLE_REAL_TAU2` are `0`), so no external sensors are required.

Superloop profiles use a different pair: `tau1` has a 10 ms period and `tau2` a 50 ms period. Their workload is controlled by `MINIMAL_TAU1_WORKLOAD_US` and `MINIMAL_TAU2_WORKLOAD_US`.

## Further work

The current project stage focuses on reproducible comparison of Superloop and Chunked EDF on synthetic periodic tasks, and on measuring scheduler overhead.

The next stage will develop and optimize Chunked EDF with real tasks and sensors. Planned work includes moving chunking from synthetic workloads to real I/O operations and studying platform limitations.

- Test Chunked EDF with real sensors.
- Identify blocking operations and other sources of increased response time and deadline misses.
- Profile Chunked EDF overhead under real load.
- Study the effect of `EDF_CHUNK_US` on schedulability and overhead.
- Optimize execution and task reselection between chunks.
- Compare deadline misses, skipped releases, response time, and CPU cost before and after optimization.

## Repository layout

| Path | Contents |
| --- | --- |
| [`Core/`](Core/) | Application code, including [`main.c`](Core/Src/main.c), interrupt handlers, and system files. |
| [`Drivers/`](Drivers/) | STM32F7 HAL and CMSIS. |
| [`docs/references/`](docs/references/) | Papers and other sources. |
| [`docs/presentation/`](docs/presentation/) | Presentation and system-model figures. |
| [`notebooks/`](notebooks/) | Colab notebook for result processing and plots. |
| [`tools/experiment_runner/`](tools/experiment_runner/) | Automated build, flashing, UART capture, and CSV aggregation. |
| [`demonstration.ioc`](demonstration.ioc) | CubeMX configuration: peripherals, pins, and clocks. |
| [`.project`](.project), [`.cproject`](.cproject), [`.mxproject`](.mxproject), [`.settings/`](.settings/) | STM32CubeIDE/Eclipse project files. |
| [`demonstration Debug.launch`](<demonstration Debug.launch>) | IDE run and debug configuration. |

`Debug/` and `Release/` are generated build outputs. `node_modules/` contains JavaScript dependencies. They are ignored by Git and must not be committed.

## Troubleshooting

| Symptom | Check |
| --- | --- |
| No COM port | Use a data-capable cable; check ST-LINK and its driver in Device Manager. |
| Garbled text | Set `115200`, `8N1`, `Parity=None`, and `Flow control=None`. |
| No output | Open the correct COM port before starting. In modes 3 and 4, wait 60 seconds, then restart the board if necessary. |
| Project does not build | Import the repository root and install STM32Cube FW F7 in STM32CubeIDE. |
| Board cannot be programmed | Close applications using ST-LINK or the COM port, reconnect the board, and retry `Debug`. |

## Additional materials

- [Project presentation](https://docs.google.com/presentation/d/1G00Xcs1oBs7-omI5czPnljay-vytueP_O6MEM0KGgIo/edit?usp=sharing)
- [Google Colab: plots and result processing](https://colab.research.google.com/drive/1k7JMMJAk3bt3nKziyVm84fFln8bctSOF?usp=sharing)
