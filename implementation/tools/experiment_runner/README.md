# Automated SuperLoop and chunked EDF experiments

[Русская версия](README.ru.md)

`run_matrix.py` configures, builds, flashes, captures UART, and saves CSV for each run. See the [root README](../../../README.md) for research scope and the [configuration reference](../../docs/configuration.md) for firmware parameters and metric definitions.

## Setup and execution

Install STM32CubeIDE with STM32Cube FW F7, STM32CubeProgrammer, and Python 3.10+. Install the dependency with `py -m pip install -r implementation/tools/experiment_runner/requirements.txt`. Close any terminal using the COM port and CubeIDE if it uses the same workspace. Run from the repository root, replacing the tool paths with your installation paths:

```powershell
py implementation\tools\experiment_runner\run_matrix.py `
  --port COM3 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix implementation\tools\experiment_runner\matrix.integrated_stats.json
```

Add `--dry-run` to validate the plan without builds or hardware access. Flashing connects under hardware reset (`mode=UR`, `reset=HWrst`) with up to three attempts for recognized ST-LINK connection errors. Other failures are not retried.

## Integrated matrices

| Matrix | Scheduler | Window, s | Chunk, ms | Runs |
| --- | --- | ---: | ---: | ---: |
| `matrix.integrated_stats.json` | SuperLoop | 60 | - | 42 |
| `matrix.integrated_edf.json` | EDF | 60 | 1 | 42 |
| `matrix.integrated_stats_100s.json` | SuperLoop | 100 | - | 18 |
| `matrix.integrated_edf_100s.json` | EDF | 100 | 1 | 18 |
| `matrix.integrated_edf_chunk2ms.json` | EDF | 60 | 2 | 18 |
| `matrix.integrated_edf_chunk4ms.json` | EDF | 60 | 4 | 18 |

The main 60 s series uses U50/U65/U75/U80/U90/U95/U100. The 100 s and 2/4 ms series use U50/U90/U100. All use 2/3 tasks, three repeats, Release, disabled sensors, and `extended_stats: false`. Total: 156 runs and 180 measurement minutes, excluding builds/flashes.

Run matrices sequentially in separate fresh output directories. Reuse the matching subset of the main 1 ms EDF campaign for the chunk sweep only with identical sources/settings. This sweep does not directly measure preemption delay.

For diagnostics, copy a matrix and set `extended_stats: true`. The runner records and validates `EXTENDED_STATS`. Do not pool diagnostic and compact runs, or old Debug runs with Release results.

## Other profiles

- `matrix.default.json`: 84 clean/checks runs, U50/U65/U80/U90/U95/U100, windows 10/30/60/100/250/500/1000 s; 390 measurement minutes.
- `matrix.scalability.json`: 12 scale_clean/scale_checks runs, U65/U90, 2/3/4 tasks, 60 s; 12 minutes.

Omitting `--matrix` selects `matrix.default.json`, not integrated. These separate profiles execute SuperLoop regardless of `scheduler_algorithm`; use integrated for EDF. Integrated supports 2/3 tasks. Customize a copy of the relevant matrix. Firmware-supported scenarios may differ from runner-supported scenarios (U110 is not yet accepted by the runner).

## Results and resume

Each campaign creates `results/<UTC timestamp>/`:

- `summary.csv`: run parameters, status, and measurements;
- `task_summary.csv`: integrated task rows;
- `raw/*.log`: UART output;
- `build/*.log`, `build/*.flash.log`, `build/*.flash.attemptN.log`: build and flashing attempts;
- `matrix.json` and `manifest.json`: configuration and source fingerprint.

Results are ignored by Git. run_key is unique within a campaign but omits scheduler/chunk: join tables using campaign directory plus run_key. Read chunk size from summary.csv CHUNK_US. See the [configuration reference](../../docs/configuration.md) for CSV semantics and unfinished-job accounting.

Resume with the same matrix and `--output-dir <directory> --resume`. Successful runs are skipped and failures retried; old failed rows remain. Analyze successful unique runs. Resume requires the same matrix and source fingerprint, including the runner; use a new campaign on mismatch rather than editing the manifest.

The runner temporarily writes experiment_config.h and restores its original contents on normal exit, errors, and Ctrl+C. Check the file after forcibly terminating the process. The temporary CubeIDE workspace is outside the repository.
