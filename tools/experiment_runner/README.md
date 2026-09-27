# Automated Superloop Profile Collection

[Русская версия](README.ru.md)

`run_matrix.py` runs an experiment matrix without PuTTY. For every run, it changes the firmware configuration, builds the project, flashes the board through ST-LINK, reads UART, and stores the result.

## One-time setup

1. STM32CubeIDE with STM32Cube FW F7.
2. STM32CubeProgrammer.
3. Python 3.10 or later and the UART dependency:

   ```powershell
   py -m pip install -r tools\experiment_runner\requirements.txt
   ```

Locate the two executables. Typical Windows locations are:

```text
C:\ST\STM32CubeIDE_<version>\STM32CubeIDE\headless-build.bat
C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe
```

Close STM32CubeIDE if it is using the same workspace, and close PuTTY: the script needs exclusive access to the COM port.

## Full run

Connect the `NUCLEO-F767ZI`, find its ST-LINK Virtual COM Port number, then run this command from the repository root:

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\ST\STM32CubeIDE_<version>\STM32CubeIDE\headless-build.bat" `
  --programmer "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
```

`matrix.default.json` defines 84 runs: `U50`-`U100`, windows of 10/30/60/100/250/500/1000 seconds, and the `clean` and `checks` modes. The measurement windows alone take 390 minutes; allow additional time for 84 builds and flashes.

### Separate scalability run

`matrix.scalability.json` is deliberately separate from the default matrix. It runs exactly 12 measurements: task counts 2/3/4, scenarios `U65` and `U90`, 60-second windows, and `scale_clean`/`scale_checks` modes. It does not repeat or modify the 84-run campaign.

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix tools\experiment_runner\matrix.scalability.json
```

The measurement windows take 12 minutes in total; allow a few extra minutes for builds and flashes. Run it without `--output-dir` to create a new results directory and a separate `summary.csv`.

### Separate integrated-statistics run

`matrix.integrated_stats.json` is another independent campaign for the original full-statistics integrated Superloop profile. It runs `U50`, `U75`, `U90`, and `U100`; 2 and 3 synthetic tasks; and 30/60/100-second windows: 24 measurements in total.

```powershell
py tools\experiment_runner\run_matrix.py `
  --port COM5 `
  --headless-builder "C:\path\headless-build.bat" `
  --programmer "C:\path\STM32_Programmer_CLI.exe" `
  --matrix tools\experiment_runner\matrix.integrated_stats.json
```

The measurement windows take 25.3 minutes in total. In addition to `summary.csv`, this campaign writes `task_summary.csv` with the complete per-task execution, response-time, deadline, and skipped-release CSV rows. The 2-task configuration uses IMU and LiDAR, with their 3:4 relative workload split re-normalized to the selected total utilization.

For a safe check without touching the board:

```powershell
py tools\experiment_runner\run_matrix.py --port COM5 --headless-builder C:\path\headless-build.bat --programmer C:\path\STM32_Programmer_CLI.exe --dry-run
```

## Results

Every campaign creates its own `results/<UTC timestamp>/` directory:

- `summary.csv` - one table with requested and reported parameters for all runs;
- `task_summary.csv` - all `CSV_TASK` rows from integrated-statistics runs;
- `raw/*.log` - the complete UART log of each run;
- `build/*.log` and `build/*.flash.log` - build and flashing logs;
- `matrix.json` - the exact matrix used for the campaign.

The temporary STM32CubeIDE headless workspace is created outside the repository and is not placed in the results directory.

All result files are ignored by Git. A failed run is still written to `summary.csv` with `status=failed` and an error message. To continue the same campaign without repeating successful runs, use `--output-dir <directory> --resume`.

## Changing the matrix

Copy `matrix.default.json`, keep the required scenarios, windows, and modes, then pass it with `--matrix`. For `scale_clean` or `scale_checks`, also provide `task_counts` containing only `2`, `3`, and/or `4`. For example, a short test of one mode:

```json
{
  "scenarios": ["U65"],
  "windows_us": [10000000],
  "modes": ["clean"],
  "repeats": 1
}
```

For each run, the script temporarily rewrites [`implementation/Core/Inc/experiment_config.h`](../../implementation/Core/Inc/experiment_config.h). It restores the original file even after an error or `Ctrl+C`. After a forced PC shutdown, check that file with `git diff` before a manual build.

### Integrated measurement boundary

The integrated profile produces one measurement per reset, then waits for reset/reflash. The runner already reflashes each configuration. Task rows share a cutoff timestamp taken before UART output. `PENDING` identifies the released but unfinished tracked job (including one not started yet); `PENDING_OVERDUE` indicates a strictly exceeded deadline at cutoff. `PENDING_AGE_US` is its age since release and `PENDING_EXEC_US` is its measured execution so far. These describe the tracked job, not later releases awaiting skip accounting. `MISSES` and response statistics remain completion-only; report unfinished jobs separately. `TASK_EXEC` includes partial execution of unfinished jobs.

### Updated integrated matrices

The current comparison supersedes the old 24-run integrated campaign. `matrix.integrated_stats.json` and `matrix.integrated_edf.json` contain 42 runs each: 60 s, seven loads U50/U65/U75/U80/U90/U95/U100, 2/3 tasks, three repeats. Their `_100s.json` counterparts contain 18 runs each at U50/U90/U100. Total: 120 runs, 144 measurement minutes, all Release, EDF chunk 1000 us. Use a fresh output directory per matrix.

`extended_stats: false` retains completion/miss/skip counts, response sum/max, execution totals, scheduler/polling sums and counts, and unfinished-job fields. Histograms, stored samples, execution/cycle extrema and response minimum are disabled. Use `extended_stats: true` (manual build: `ENABLE_EXTENDED_STATS=1`) for separate diagnostic runs. EXTENDED_STATS is recorded and checked by the runner. Other profiles retain extended statistics.

### EDF chunk-size sweep

`matrix.integrated_edf_chunk2ms.json` and `matrix.integrated_edf_chunk4ms.json` use 2000/4000 us chunks. Each has 18 runs: U50/U90/U100, 2/3 tasks, 60 s, three repeats, Release, compact statistics. Total additional measurement time is 36 minutes, excluding builds/flashes. Pass either file with `--matrix` and the usual port/builder/programmer arguments; add `--dry-run` to check without hardware.

Use a fresh output directory per matrix (the default creates one); run_key does not include chunk size. Reuse the U50/U90/U100 subset of the main 1 ms EDF campaign only with matching sources/settings. Identify chunk size using summary.csv CHUNK_US and the saved matrix.json; join task rows to run rows within their campaign. Compare per-task response times, misses/skips, overhead and pending jobs. This sweep does not directly measure preemption delay.
