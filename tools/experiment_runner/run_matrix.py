#!/usr/bin/env python3
"""Build, flash, capture and aggregate a matrix of STM32 scheduler experiments."""

from __future__ import annotations

import argparse
import csv
import json
import hashlib
import shutil
import os
import subprocess
import sys
import tempfile
import time
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

try:
    import serial
except ImportError as error:
    raise SystemExit(
        "Missing dependency: install it with 'py -m pip install -r "
        "tools/experiment_runner/requirements.txt'."
    ) from error


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
FIRMWARE_ROOT = REPOSITORY_ROOT / "implementation"
CONFIG_HEADER = FIRMWARE_ROOT / "Core" / "Inc" / "experiment_config.h"
PROJECT_NAME = "demonstration"
def firmware_path(build_configuration: str) -> Path:
    if build_configuration not in {"Debug", "Release"}:
        raise ValueError("build_configuration must be Debug or Release")
    return FIRMWARE_ROOT / build_configuration / "demonstration.elf"

SCENARIO_MACROS = {
    "U50": "WORKLOAD_SCENARIO_U50",
    "U65": "WORKLOAD_SCENARIO_U65",
    "U75": "WORKLOAD_SCENARIO_U75",
    "U80": "WORKLOAD_SCENARIO_U80",
    "U90": "WORKLOAD_SCENARIO_U90",
    "U95": "WORKLOAD_SCENARIO_U95",
    "U100": "WORKLOAD_SCENARIO_U100",
}
MODE_MACROS = {
    "integrated": "EXPERIMENT_INTEGRATED",
    "clean": "EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE",
    "checks": "EXPERIMENT_SUPERLOOP_CHECKS_PROFILE",
    "scale_clean": "EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN",
    "scale_checks": "EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS",
}
CSV_PREFIXES = {
    "integrated": "CSV_RUN,",
    "clean": "CSV_CLEAN_SUPERLOOP,",
    "checks": "CSV_SUPERLOOP_CHECKS,",
    "scale_clean": "CSV_SUPERLOOP_SCALE_CLEAN,",
    "scale_checks": "CSV_SUPERLOOP_SCALE_CHECKS,",
}
SCALABILITY_MODES = {"scale_clean", "scale_checks"}
INTEGRATED_MODES = {"integrated"}
TASK_COUNT_MODES = SCALABILITY_MODES | INTEGRATED_MODES

SUMMARY_FIELDS = [
    "run_key",
    "status",
    "host_started_utc",
    "host_finished_utc",
    "mode",
    "requested_scenario",
    "requested_window_us",
    "requested_tasks",
    "repeat",
    "csv_type",
    "SCENARIO",
    "U",
    "WINDOW_US",
    "DEVICE_WINDOW_TARGET_US",
    "SYNTH_U_X10000",
    "REAL_TASKS",
    "TAU1_RUNS",
    "TAU2_RUNS",
    "TAU3_RUNS",
    "TAU4_RUNS",
    "TASKS",
    "TASK_US",
    "TASK_PCT",
    "SUPERLOOP_US",
    "SUPERLOOP_PCT",
    "READINESS_CYCLES",
    "READINESS_US",
    "READINESS_PCT",
    "RELEASE_CYCLES",
    "RELEASE_US",
    "RELEASE_PCT",
    "CHECKS_CYCLES",
    "CHECKS_US",
    "CHECKS_PCT",
    "SYNTH_TASKS",
    "SCHED",
    "CHUNK_US",
    "SCHED_LOOPS",
    "SCHED_OVH",
    "POLL_LOOPS",
    "POLL_OVH",
    "TASK_EXEC",
    "BUSY",
    "IDLE",
    "error",
    "device_csv",
]

INTEGRATED_TASK_SUMMARY_FIELDS = [
    "run_key",
    "requested_scenario",
    "requested_window_us",
    "requested_tasks",
    "repeat",
    "csv_type",
    "SCHED",
    "SCENARIO",
    "U",
    "TASK",
    "RUNS",
    "C_US",
    "T_MS",
    "D_MS",
    "EXEC_AVG_US",
    "RESP_AVG_US",
    "RESP_MAX_US",
    "MISSES",
    "SKIPPED",
    "FAILURES",
    "MAX_LATENESS_US",
    "device_csv",
]


@dataclass(frozen=True)
class RunSpec:
    mode: str
    scenario: str
    window_us: int
    repeat: int
    task_count: int | None = None

    @property
    def run_key(self) -> str:
        if self.task_count is not None:
            return (
                f"{self.mode}_{self.scenario}_tasks{self.task_count}_"
                f"{self.window_us}_r{self.repeat:02d}"
            )
        return f"{self.mode}_{self.scenario}_{self.window_us}_r{self.repeat:02d}"


def utc_now() -> str:
    return datetime.now(timezone.utc).isoformat(timespec="seconds")


def atomic_write(path: Path, content: str) -> None:
    temporary_path = path.with_suffix(path.suffix + ".tmp")
    with temporary_path.open("w", encoding="utf-8", newline="\n") as temporary_file:
        temporary_file.write(content)
    os.replace(temporary_path, path)


def load_matrix(path: Path) -> dict[str, Any]:
    try:
        matrix = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise SystemExit(f"Cannot read matrix '{path}': {error}") from error

    for key in ("scenarios", "windows_us", "modes"):
        if not matrix.get(key):
            raise SystemExit(f"Matrix field '{key}' must not be empty.")

    for scenario in matrix["scenarios"]:
        if scenario not in SCENARIO_MACROS:
            raise SystemExit(f"Unsupported scenario '{scenario}'.")
    for mode in matrix["modes"]:
        if mode not in MODE_MACROS:
            raise SystemExit(f"Unsupported mode '{mode}'.")

    modes = set(matrix["modes"])
    scale_modes = modes & SCALABILITY_MODES
    task_count_modes = modes & TASK_COUNT_MODES
    if scale_modes:
        if set(matrix["scenarios"]) - {"U65", "U90"}:
            raise SystemExit("Scalability modes support U65 and U90 only.")

    if task_count_modes:
        task_counts = matrix.get("task_counts")
        if not task_counts:
            raise SystemExit("Task-count matrices must define non-empty 'task_counts'.")
        allowed_counts = {2, 3, 4} if scale_modes else {2, 3}
        if any(int(count) not in allowed_counts for count in task_counts):
            allowed_text = ", ".join(str(count) for count in sorted(allowed_counts))
            raise SystemExit(f"task_counts must contain only {allowed_text}.")

    if any(type(w) is not int or w <= 0 or w > 0xFFFFFFFF for w in matrix["windows_us"]):
        raise SystemExit("windows_us must contain positive 32-bit integers")
    firmware_path(matrix.get("build_configuration", "Debug"))
    if matrix.get("scheduler_algorithm", "SCHED_ALGO_SUPERLOOP") not in {"SCHED_ALGO_SUPERLOOP", "SCHED_ALGO_CHUNKED_EDF"}:
        raise SystemExit("Unsupported scheduler_algorithm")
    if int(matrix.get("edf_chunk_us", 1000)) <= 0:
        raise SystemExit("edf_chunk_us must be positive")
    if int(matrix.get("repeats", 1)) < 1:
        raise SystemExit("Matrix field 'repeats' must be at least 1.")
    return matrix


def create_specs(matrix: dict[str, Any]) -> list[RunSpec]:
    specs: list[RunSpec] = []
    for mode in matrix["modes"]:
        task_counts = matrix.get("task_counts", []) if mode in TASK_COUNT_MODES else [None]
        for scenario in matrix["scenarios"]:
            for task_count in task_counts:
                for window_us in matrix["windows_us"]:
                    for repeat in range(1, int(matrix.get("repeats", 1)) + 1):
                        specs.append(RunSpec(
                            mode, scenario, int(window_us), repeat, task_count,
                        ))
    return specs


def render_config(spec: RunSpec, matrix: dict[str, Any]) -> str:
    scheduler_macro = matrix.get("scheduler_algorithm", "SCHED_ALGO_SUPERLOOP")
    integrated_window_us = (
        spec.window_us if spec.mode in INTEGRATED_MODES
        else int(matrix.get("integrated_window_us", 10_000_000))
    )
    edf_chunk_us = int(matrix.get("edf_chunk_us", 1_000))
    scalability_window_us = (
        spec.window_us if spec.mode in SCALABILITY_MODES
        else int(matrix.get("scalability_window_us", 60_000_000))
    )
    scalability_task_count = (
        spec.task_count if spec.mode in SCALABILITY_MODES else 2
    )
    integrated_task_count = (
        spec.task_count if spec.mode in INTEGRATED_MODES
        else int(matrix.get("integrated_task_count", 3))
    )

    return f"""#ifndef EXPERIMENT_CONFIG_H
#define EXPERIMENT_CONFIG_H

/* Auto-generated by tools/experiment_runner/run_matrix.py. */
#define WORKLOAD_SCENARIO {SCENARIO_MACROS[spec.scenario]}
#define SCHED_ALGO {scheduler_macro}
#define EXPERIMENT_MODE {MODE_MACROS[spec.mode]}
#define INTEGRATED_SYNTH_TASK_COUNT {integrated_task_count}
#define PROFILE_WINDOW_US {integrated_window_us}ULL
#define MINIMAL_PROFILE_WINDOW_US {spec.window_us}ULL
#define SCALABILITY_PROFILE_WINDOW_US {scalability_window_us}ULL
#define SCALABILITY_TASK_COUNT {scalability_task_count}
#define EDF_CHUNK_US {edf_chunk_us}ULL
#define ENABLE_REAL_TAU1 0
#define ENABLE_REAL_TAU2 0
#define ENABLE_SYNTH_IMU 1
#define ENABLE_SYNTH_LIDAR 1
#define ENABLE_SYNTH_CONTROL 0
#define ENABLE_DEBUG_PRINT 0
#define ENABLE_POLLING_PROFILE 1
#define SCHEDULER_MODE SCHED_BUSY_POLLING

#endif /* EXPERIMENT_CONFIG_H */
"""


def run_command(command: list[str], *, cwd: Path, log_path: Path) -> None:
    completed = subprocess.run(
        command,
        cwd=cwd,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    log_path.write_text(
        "$ " + subprocess.list2cmdline(command) + "\n\n" + completed.stdout + completed.stderr,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        raise RuntimeError(f"Command failed ({completed.returncode}); see {log_path}.")


def build_firmware(
    headless_builder: Path,
    workspace: Path,
    build_configuration: str,
    import_project: bool,
    log_path: Path,
) -> Path:
    output_path = firmware_path(build_configuration)
    # A successful process exit alone must never authorize flashing stale output.
    output_path.unlink(missing_ok=True)
    command = [str(headless_builder), "-data", str(workspace)]
    if import_project:
        command.extend(["-import", str(FIRMWARE_ROOT)])
    command.extend(["-cleanBuild", f"{PROJECT_NAME}/{build_configuration}"])
    run_command(command, cwd=REPOSITORY_ROOT, log_path=log_path)

    if not output_path.is_file():
        raise RuntimeError(f"Build completed but firmware is missing: {output_path}")
    return output_path


def flash_firmware(programmer: Path, frequency_khz: int, log_path: Path, output_path: Path) -> None:
    command = [
        str(programmer),
        "-c",
        "port=SWD",
        f"freq={frequency_khz}",
        "-w",
        str(output_path),
        "-v",
        "-rst",
    ]
    run_command(command, cwd=REPOSITORY_ROOT, log_path=log_path)


def parse_device_csv(device_csv: str) -> dict[str, str]:
    fields = next(csv.reader([device_csv]))
    if not fields:
        raise ValueError("Empty device CSV line.")
    result = {"csv_type": fields[0]}
    for field in fields[1:]:
        key, separator, value = field.partition("=")
        if separator != "=":
            raise ValueError(f"Malformed device CSV field '{field}'.")
        result[key] = value
    return result


def validate_device_result(spec: RunSpec, device_result: dict[str, str], matrix: dict[str, Any]) -> None:
    if device_result.get("SCENARIO") != spec.scenario:
        raise ValueError(
            f"Device reported scenario {device_result.get('SCENARIO')!r}, "
            f"expected {spec.scenario!r}."
        )
    if int(device_result.get("WINDOW_US", "0")) < spec.window_us:
        raise ValueError(
            f"Device window {device_result.get('WINDOW_US')!r} is shorter than "
            f"the requested {spec.window_us}."
        )
    if spec.task_count is not None and device_result.get("TASKS") != str(spec.task_count):
        if spec.mode in SCALABILITY_MODES:
            raise ValueError(
                f"Device reported TASKS={device_result.get('TASKS')!r}, "
                f"expected {spec.task_count}."
            )
    if spec.mode in INTEGRATED_MODES:
        expected_scheduler = matrix.get("scheduler_algorithm", "SCHED_ALGO_SUPERLOOP").removeprefix("SCHED_ALGO_")
        expected_chunk = int(matrix.get("edf_chunk_us", 1000)) if expected_scheduler == "CHUNKED_EDF" else 0
        if device_result.get("SCHED") != expected_scheduler or device_result.get("CHUNK_US") != str(expected_chunk):
            raise ValueError("Device scheduler/chunk does not match requested configuration")
        if device_result.get("REQUESTED_WINDOW_US") != str(spec.window_us):
            raise ValueError("Device requested window does not match configuration")
        if device_result.get("REAL_TASKS") != "0":
            raise ValueError("Synthetic campaign must have physical sensors disabled")
    if spec.mode in INTEGRATED_MODES and device_result.get("SYNTH_TASKS") != str(spec.task_count):
        raise ValueError(
            f"Device reported SYNTH_TASKS={device_result.get('SYNTH_TASKS')!r}, "
            f"expected {spec.task_count}."
        )


def append_csv_row(path: Path, fieldnames: list[str], row: dict[str, str]) -> None:
    needs_header = not path.exists()
    with path.open("a", newline="", encoding="utf-8") as output_file:
        writer = csv.DictWriter(output_file, fieldnames=fieldnames, extrasaction="ignore")
        if needs_header:
            writer.writeheader()
        writer.writerow({field: row.get(field, "") for field in fieldnames})


def append_summary(summary_path: Path, row: dict[str, str]) -> None:
    append_csv_row(summary_path, SUMMARY_FIELDS, row)


def completed_run_keys(summary_path: Path) -> set[str]:
    if not summary_path.exists():
        return set()
    with summary_path.open(newline="", encoding="utf-8") as summary_file:
        return {
            row["run_key"]
            for row in csv.DictReader(summary_file)
            if row.get("status") == "ok"
        }


def validate_paths(arguments: argparse.Namespace) -> None:
    for path, label in (
        (arguments.headless_builder, "STM32CubeIDE headless builder"),
        (arguments.programmer, "STM32CubeProgrammer CLI"),
    ):
        if not path.is_file():
            raise SystemExit(f"{label} not found: {path}")
    if not CONFIG_HEADER.is_file():
        raise SystemExit(f"Configuration header not found: {CONFIG_HEADER}")


def source_fingerprint() -> str:
    digest = hashlib.sha256()
    paths = list((FIRMWARE_ROOT / "Core").rglob("*.c")) + list((FIRMWARE_ROOT / "Core").rglob("*.h"))
    paths += list((FIRMWARE_ROOT / "Drivers").rglob("*.c")) + list((FIRMWARE_ROOT / "Drivers").rglob("*.h"))
    paths += list(FIRMWARE_ROOT.glob("*.ld")) + list((FIRMWARE_ROOT / "Core").rglob("*.s"))
    paths += [FIRMWARE_ROOT / ".cproject", Path(__file__)]
    for path in sorted(paths):
        if path == CONFIG_HEADER:
            continue
        digest.update(str(path.relative_to(REPOSITORY_ROOT)).replace("\\", "/").encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def prepare_campaign(output_dir: Path, matrix: dict[str, Any], resume: bool) -> None:
    manifest = {"schema": 1, "matrix": matrix, "source_sha256": source_fingerprint()}
    manifest_path = output_dir / "manifest.json"
    if output_dir.exists() and any(output_dir.iterdir()):
        if not resume:
            raise ValueError("Output directory is not empty; use a new directory or --resume")
        if not manifest_path.exists() or json.loads(manifest_path.read_text(encoding="utf-8")) != manifest:
            raise ValueError("Cannot resume: matrix/source mismatch or missing manifest; use a new output directory")
        return
    output_dir.mkdir(parents=True, exist_ok=True)
    atomic_write(manifest_path, json.dumps(manifest, indent=2) + "\n")
    atomic_write(output_dir / "matrix.json", json.dumps(matrix, indent=2) + "\n")


def parse_arguments() -> argparse.Namespace:
    default_matrix = Path(__file__).with_name("matrix.default.json")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", required=True, help="ST-LINK virtual COM port, for example COM5")
    parser.add_argument("--headless-builder", required=True, type=Path,
                        help="Path to STM32CubeIDE headless-build.bat")
    parser.add_argument("--programmer", required=True, type=Path,
                        help="Path to STM32_Programmer_CLI.exe")
    parser.add_argument("--matrix", type=Path, default=default_matrix,
                        help="JSON file describing the experiment matrix")
    parser.add_argument("--output-dir", type=Path,
                        help="Directory for this campaign; default: results/<UTC timestamp>")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--resume", action="store_true",
                        help="Skip runs already recorded with status=ok in summary.csv")
    parser.add_argument("--dry-run", action="store_true",
                        help="Validate the matrix and print planned runs without touching hardware")
    return parser.parse_args()


def main() -> int:
    arguments = parse_arguments()
    matrix = load_matrix(arguments.matrix)
    specs = create_specs(matrix)
    total_window_seconds = sum(spec.window_us for spec in specs) / 1_000_000

    if arguments.output_dir:
        output_dir = arguments.output_dir.resolve()
    else:
        timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        output_dir = REPOSITORY_ROOT / "results" / timestamp
    raw_dir = output_dir / "raw"
    build_dir = output_dir / "build"
    # Eclipse refuses to import a project that contains its own workspace.
    # The default output directory lives under the repository, so the CubeIDE
    # workspace must be created elsewhere.
    workspace = None
    summary_path = output_dir / "summary.csv"
    task_summary_path = output_dir / "task_summary.csv"

    print(f"Planned runs: {len(specs)}; total measurement window: {total_window_seconds / 60:.1f} min")
    for spec in specs:
        print(f"  {spec.run_key}")
    if arguments.dry_run:
        return 0

    validate_paths(arguments)
    prepare_campaign(output_dir, matrix, arguments.resume)
    raw_dir.mkdir(exist_ok=True)
    build_dir.mkdir(exist_ok=True)
    workspace = Path(tempfile.mkdtemp(prefix="stm32-experiment-runner-"))

    completed = completed_run_keys(summary_path) if arguments.resume else set()
    original_config = CONFIG_HEADER.read_text(encoding="utf-8")
    import_project = True
    grace_seconds = float(matrix.get("uart_timeout_grace_s", 45))
    build_configuration = matrix.get("build_configuration", "Debug")
    frequency_khz = int(matrix.get("stlink_frequency_khz", 4000))

    failures = 0
    try:
        for index, spec in enumerate(specs, start=1):
            if spec.run_key in completed:
                print(f"[{index}/{len(specs)}] Skip completed {spec.run_key}")
                continue

            started_at = utc_now()
            row = {
                "run_key": spec.run_key,
                "status": "failed",
                "host_started_utc": started_at,
                "mode": spec.mode,
                "requested_scenario": spec.scenario,
                "requested_window_us": str(spec.window_us),
                "requested_tasks": "" if spec.task_count is None else str(spec.task_count),
                "repeat": str(spec.repeat),
            }
            print(f"[{index}/{len(specs)}] Running {spec.run_key}")
            try:
                atomic_write(CONFIG_HEADER, render_config(spec, matrix))
                output_path = build_firmware(
                    arguments.headless_builder,
                    workspace,
                    build_configuration,
                    import_project,
                    build_dir / f"{spec.run_key}.log",
                )
                import_project = False

                # Open the COM port before reset so the first UART byte is not lost.
                expected_prefix = CSV_PREFIXES[spec.mode]
                raw_log_path = raw_dir / f"{spec.run_key}.log"
                with serial.Serial(port=arguments.port, baudrate=arguments.baudrate, timeout=0.25) as serial_port:
                    serial_port.reset_input_buffer()
                    flash_firmware(
                        arguments.programmer,
                        frequency_khz,
                        build_dir / f"{spec.run_key}.flash.log",
                        output_path,
                    )
                    deadline = time.monotonic() + spec.window_us / 1_000_000 + grace_seconds
                    with raw_log_path.open("wb") as raw_log:
                        device_csv = ""
                        task_csv_lines: list[str] = []
                        while time.monotonic() < deadline:
                            data = serial_port.readline()
                            if not data:
                                continue
                            raw_log.write(data)
                            raw_log.flush()
                            line = data.decode("utf-8", errors="replace").strip()
                            task_prefix_index = line.find("CSV_TASK,")
                            if spec.mode in INTEGRATED_MODES and task_prefix_index >= 0:
                                task_csv_lines.append(line[task_prefix_index:])
                            prefix_index = line.find(expected_prefix)
                            if prefix_index >= 0:
                                device_csv = line[prefix_index:]
                            if device_csv and (
                                spec.mode not in INTEGRATED_MODES
                                or len(task_csv_lines) >= spec.task_count
                            ):
                                break

                if not device_csv:
                    raise TimeoutError(
                        f"UART timeout; expected '{expected_prefix}' after "
                        f"{spec.window_us / 1_000_000 + grace_seconds:.0f} s."
                    )
                if spec.mode in INTEGRATED_MODES and len(task_csv_lines) != spec.task_count:
                    raise TimeoutError(
                        f"UART captured {len(task_csv_lines)} CSV_TASK rows, expected "
                        f"{spec.task_count}."
                    )
                device_result = parse_device_csv(device_csv)
                validate_device_result(spec, device_result, matrix)
                row.update(device_result)
                row["DEVICE_WINDOW_TARGET_US"] = device_result.get("REQUESTED_WINDOW_US", "")
                row["device_csv"] = device_csv
                if spec.mode in INTEGRATED_MODES:
                    for task_csv in task_csv_lines:
                        task_result = parse_device_csv(task_csv)
                        if task_result.get("SCENARIO") != spec.scenario:
                            raise ValueError(
                                f"Task CSV reported scenario {task_result.get('SCENARIO')!r}, "
                                f"expected {spec.scenario!r}."
                            )
                        task_row = {
                            "run_key": spec.run_key,
                            "requested_scenario": spec.scenario,
                            "requested_window_us": str(spec.window_us),
                            "requested_tasks": str(spec.task_count),
                            "repeat": str(spec.repeat),
                            "device_csv": task_csv,
                        }
                        task_row.update(task_result)
                        append_csv_row(
                            task_summary_path,
                            INTEGRATED_TASK_SUMMARY_FIELDS,
                            task_row,
                        )
                row["status"] = "ok"
            except Exception as error:  # Record failures and continue with the matrix.
                failures += 1
                row["error"] = str(error)
                print(f"  FAILED: {error}", file=sys.stderr)
            finally:
                row["host_finished_utc"] = utc_now()
                append_summary(summary_path, row)
    finally:
        atomic_write(CONFIG_HEADER, original_config)
        shutil.rmtree(workspace, ignore_errors=True)

    print(f"Finished. Failures: {failures}. Results: {output_dir}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
