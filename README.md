# Experimental Scheduler Comparison on STM32F767

[Русская версия](README.ru.md)

STM32CubeIDE experimental platform for comparing SuperLoop and chunked EDF on an `STM32F767ZITx`. The study examines how workload utilization, task-set composition, and chunk size affect response time, deadline misses, skipped releases, and scheduler overhead.

Two scheduling approaches are implemented:

- **Superloop** - fixed, non-preemptive execution order.
- **Chunked EDF** - synthetic jobs execute in cooperative chunks, after each chunk, the ready job with the earliest absolute deadline is selected.

The current experiment series uses periodic synthetic tasks with controlled workloads. HC-SR04 and DHT11 support is implemented separately, synthetic results do not validate physical-sensor behavior. Firmware reports through UART, and the automated runner saves configurations, logs, and CSV data for reproducible comparisons.

## Requirements

- `NUCLEO-F767ZI` with its onboard ST-LINK Virtual COM Port.
- Data-capable USB cable.
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) with STM32Cube FW F7.
- A serial terminal such as [PuTTY](https://www.putty.org/) or Tera Term.

HC-SR04 and DHT11 are optional. The default manual configuration enables them, so set their `ENABLE_HCSR04` / `ENABLE_DHT11` switches to `0` when a sensor is not connected.

## Quick start

1. Clone the repository or extract its archive.
2. In STM32CubeIDE choose `File -> Import... -> General -> Existing Projects into Workspace`, select [`implementation/`](implementation/), and finish the import.
3. Connect the board to the ST-LINK USB connector and build with `Project -> Build All`.
4. Open the ST-LINK Virtual COM Port in a terminal before starting firmware: `115200`, `8N1`, no parity, no flow control.
5. Click `Run` or `Debug`. STM32CubeIDE flashes the board and starts the program.

If no COM port appears, reconnect the board and update the ST-LINK driver. The project uses `USART3` (`PD8` TX, `PD9` RX), see [`implementation/demonstration.ioc`](implementation/demonstration.ioc).

## Configure and run an experiment

The manual switches are grouped in [`app_config.h`](implementation/Core/Inc/app_config.h). Rebuild and flash after changing them. The current defaults are `U50`, Superloop, integrated profiling, a 60-second window, and both physical sensors enabled.

```c
#define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U50
#define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
```

Use the [configuration reference](docs/configuration.md) to select a workload, measurement window, scheduler, task count, or profiling mode. It also explains the CSV output and what every macro controls.

The [firmware source map](implementation/Core/README.md) shows where the configuration, sensors, scheduling, profiling, and CubeMX-managed code live.

The two scheduling paths differ in an important way:

- In Superloop, HC-SR04 and DHT11 use blocking baseline transactions.
- In Chunked EDF, each sensor exposes waiting as a non-runnable state, allowing other ready work to execute. HC-SR04 uses EXTI for ECHO edges, DHT11 has a scheduler-visible 30 ms start-low wait.

The [sensor guide](docs/sensors.md) documents both state machines, wiring-related pin assignments, and short hardware smoke tests.

The integrated profile measures one window per reset, prints its report through UART, then waits for reset or reflash. Unfinished jobs are reported separately. If the terminal was opened too late, reset the board.

Integrated mode defaults to compact research statistics. `ENABLE_EXTENDED_STATS=1` enables diagnostic histograms, execution samples, and auxiliary min/max values. Automated matrices use `extended_stats: true` for the same setting. Keep diagnostic and compact measurements separate.

## Automated experiment series

[`tools/experiment_runner/`](tools/experiment_runner/) can build, flash, capture UART, and aggregate CSV rows for an entire experiment matrix. It preserves the manual switches in `app_config.h`, writes logs, campaign configuration, and CSV tables under `results/<timestamp>/`, and keeps generated results outside Git. `summary.csv` holds run-level measurements, `task_summary.csv` holds per-task measurements.

The campaigns compare schedulers, 60/100 s windows, and EDF chunks of 1/2/4 ms. The automation guide below owns the full matrix inventory and run conditions.

Measurements include completed jobs, deadline misses, skipped releases, mean and maximum observed response time, execution time, and scheduler/polling overhead. The unfinished tracked job is reported with its age, execution so far, and overdue status at cutoff. Response statistics and `MISSES` cover completed jobs. Overhead metrics describe instrumented code regions, not total CPU utilization.

See the [automation guide](tools/experiment_runner/README.md) for setup, matrices, and commands.

## Documentation

| Document                                              | Contents                                                                                      |
| ----------------------------------------------------- | --------------------------------------------------------------------------------------------- |
| [Configuration reference](docs/configuration.md)      | Workloads, experiment modes, macros, synthetic tasks, and interpretation of profiling output. |
| [Sensor guide](docs/sensors.md)                       | HC-SR04 and DHT11 behavior in Superloop and Chunked EDF, pins, and smoke tests.               |
| [Automation guide](tools/experiment_runner/README.md) | Batch experiment collection on Windows.                                                       |
| [Project documents](docs/README.md)                   | Maintained Google Slides presentation and Google Colab analysis notebook.                     |

## Repository layout

| Path                                                   | Contents                                                                                                                                    |
| ------------------------------------------------------ | ------------------------------------------------------------------------------------------------------------------------------------------- |
| [`implementation/`](implementation/)                   | Self-contained STM32CubeIDE implementation, see its [firmware source map](implementation/Core/README.md) for the application-module layout. |
| [`docs/`](docs/)                                       | Concise technical documentation and links to maintained online materials.                                                                   |
| [`thesis/`](thesis/)                                   | LaTeX sources for the paper/thesis, figures, and the compiled PDF.                                                                          |
| [`tools/experiment_runner/`](tools/experiment_runner/) | Automated build, flashing, UART capture, and CSV aggregation.                                                                               |

`implementation/Debug/`, `implementation/Release/`, `results/`, Python caches, and local reference/presentation files are generated or personal material and are ignored by Git.

## Links

- [Project presentation](https://docs.google.com/presentation/d/1G00Xcs1oBs7-omI5czPnljay-vytueP_O6MEM0KGgIo/edit?usp=sharing)
- [Experiment plots and result processing](https://colab.research.google.com/drive/1dc12L2FlVo0GvSbgJcRNcKeT5a56jMEs?usp=sharing)
