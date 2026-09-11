# Real-Time Scheduler Demonstration on STM32F767

[Русская версия](README.ru.md)

STM32CubeIDE project for evaluating two cooperative real-time scheduling approaches on an `STM32F767ZITx`:

- **Superloop** - fixed, non-preemptive execution order.
- **Chunked EDF** - synthetic work is split into cooperative chunks and the earliest-deadline task is selected again after each chunk.

The firmware runs periodic synthetic and physical-sensor tasks, measures timing characteristics, and prints reports and CSV rows through UART. It is intended for reproducible experiments, not as a production scheduler.

## Requirements

- `NUCLEO-F767ZI` with its onboard ST-LINK Virtual COM Port.
- Data-capable USB cable.
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) with STM32Cube FW F7.
- A serial terminal such as [PuTTY](https://www.putty.org/) or Tera Term.

HC-SR04 and DHT11 are optional. The default manual configuration enables them, so set their `ENABLE_REAL_TAU*` switches to `0` when a sensor is not connected.

## Quick start

1. Clone the repository or extract its archive.
2. In STM32CubeIDE choose `File -> Import... -> General -> Existing Projects into Workspace`, select the repository root, and finish the import.
3. Connect the board to the ST-LINK USB connector and build with `Project -> Build All`.
4. Open the ST-LINK Virtual COM Port in a terminal before starting firmware: `115200`, `8N1`, no parity, no flow control.
5. Click `Run` or `Debug`. STM32CubeIDE flashes the board and starts the program.

If no COM port appears, reconnect the board and update the ST-LINK driver. The project uses `USART3` (`PD8` TX, `PD9` RX); see [`demonstration.ioc`](demonstration.ioc).

## Configure and run an experiment

The manual switches are grouped at the top of [`Core/Src/main.c`](Core/Src/main.c). Rebuild and flash after changing them. The current defaults are `U50`, Superloop, integrated profiling, a 60-second window, and both physical sensors enabled.

```c
#define WORKLOAD_SCENARIO WORKLOAD_SCENARIO_U50
#define SCHED_ALGO SCHED_ALGO_SUPERLOOP
#define EXPERIMENT_MODE EXPERIMENT_INTEGRATED
```

Use the [configuration reference](docs/configuration.md) to select a workload, measurement window, scheduler, task count, or profiling mode. It also explains the CSV output and what every macro controls.

The two scheduling paths differ in an important way:

- In Superloop, HC-SR04 and DHT11 use blocking baseline transactions.
- In Chunked EDF, each sensor exposes waiting as a non-runnable state, allowing other ready work to execute. HC-SR04 uses EXTI for ECHO edges; DHT11 has a scheduler-visible 30 ms start-low wait.

The [sensor guide](docs/sensors.md) documents both state machines, wiring-related pin assignments, and short hardware smoke tests.

UART output is emitted only after a profiling window. Save the reported CSV line for analysis; if the terminal was opened too late, restart the board because the result may already have been printed.

## Automated experiment series

[`tools/experiment_runner/`](tools/experiment_runner/) can build, flash, capture UART, and aggregate CSV rows for an entire experiment matrix. It preserves the manual switches in `main.c`, writes per-run logs plus `results/<timestamp>/summary.csv`, and keeps generated results outside Git.

See the [automation guide](tools/experiment_runner/README.md) for setup, matrices, and commands.

## Documentation

| Document | Contents |
| --- | --- |
| [Configuration reference](docs/configuration.md) | Workloads, experiment modes, macros, synthetic tasks, and interpretation of profiling output. |
| [Sensor guide](docs/sensors.md) | HC-SR04 and DHT11 behavior in Superloop and Chunked EDF, pins, and smoke tests. |
| [Automation guide](tools/experiment_runner/README.md) | Batch experiment collection on Windows. |
| [Project documents](docs/README.md) | Maintained Google Slides presentation and Google Colab analysis notebook. |

## Repository layout

| Path | Contents |
| --- | --- |
| [`Core/`](Core/) | Application code, configuration, interrupt handlers, and startup code. |
| [`Drivers/`](Drivers/) | STM32F7 HAL and CMSIS supplied by STM32Cube. |
| [`docs/`](docs/) | Concise technical documentation and links to maintained online materials. |
| [`tools/experiment_runner/`](tools/experiment_runner/) | Automated build, flashing, UART capture, and CSV aggregation. |
| [`demonstration.ioc`](demonstration.ioc) | CubeMX pin, clock, and peripheral configuration. |
| [`.project`](.project), [`.cproject`](.cproject), [`.mxproject`](.mxproject), [`.settings/`](.settings/) | STM32CubeIDE project metadata. |

`Debug/`, `Release/`, `results/`, Python caches, and local reference/presentation files are generated or personal material and are ignored by Git.

## Links

- [Project presentation](https://docs.google.com/presentation/d/1G00Xcs1oBs7-omI5czPnljay-vytueP_O6MEM0KGgIo/edit?usp=sharing)
- [Experiment plots and result processing](https://colab.research.google.com/drive/1dc12L2FlVo0GvSbgJcRNcKeT5a56jMEs?usp=sharing)
