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

For each run, the script temporarily rewrites [`Core/Inc/experiment_config.h`](../../Core/Inc/experiment_config.h). It restores the original file even after an error or `Ctrl+C`. After a forced PC shutdown, check that file with `git diff` before a manual build.
