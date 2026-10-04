# STM32 implementation

[Русская версия](README.ru.md)

This directory is a self-contained STM32CubeIDE project for the `NUCLEO-F767ZI`.

See the [root README](../README.md) for import and startup instructions. Application sources are in `Core/`; HAL and CMSIS are in `Drivers/`.

For the firmware module map and CubeMX-safe editing rules, see
[Core/README.md](Core/README.md). Firmware parameters are documented in
[docs/configuration.md](docs/configuration.md), sensor behavior in
[docs/sensors.md](docs/sensors.md), and automated experiment collection in
[tools/experiment_runner/README.md](tools/experiment_runner/README.md).
