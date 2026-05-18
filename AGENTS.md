# Repository Guidelines

## Project Structure & Module Organization

This repository contains multiple STM32F103 StdPeriph example projects, each in its own top-level folder: `BlinkyLED/`, `gpio_key/`, `OLED/`, `Serial_port_printf_String/`, `UART_Comm/`, `USART_RX/`, and `stm32f103_stdperiph_template/`. Treat each folder as an independent embedded project.

Common layout:
- `user/`: application entry points, interrupt handlers, and STM32 system files.
- `my_lib/`: reusable board-level drivers such as delay, USART, button, I2C, SPI, and OLED helpers.
- `std_periph_driver/inc` and `std_periph_driver/src`: STM32 Standard Peripheral Library headers and sources.
- `startup/`: Cortex-M3 startup assembly; STM32F103C8 projects normally use `startup_stm32f10x_md.s`.
- `docs/` or PNG files: project notes, wiring diagrams, and reference assets when present.

## Build, Test, and Development Commands

Open a project through its workspace or Keil project file, for example:

```powershell
code .\Serial_port_printf_String\Serial_port_printf_String.code-workspace
```

Build from VS Code with the EIDE extension or from Keil uVision using the matching `.uvprojx`, for example `gpio_key/gpio_key.uvprojx`. Projects are configured for ARMCC5, Cortex-M3, `STM32F10X_MD`, and `USE_STDPERIPH_DRIVER`.

Use each project cleanup script before packaging or comparing generated output:

```powershell
.\Serial_port_printf_String\清理.cmd
```

## Coding Style & Naming Conventions

Use C with STM32 StdPeriph APIs; do not mix HAL, LL, or CubeMX styles into these projects. Keep initialization code in the existing struct/function style (`GPIO_InitTypeDef`, `USART_InitTypeDef`, `RCC_APB2PeriphClockCmd`, etc.).

Follow the local `.clang-format`: Microsoft base style, 4-space indentation, no tabs, no include sorting, and no fixed column limit. Keep public helper APIs in `my_lib/*.h` stable unless the change explicitly requires an interface update.

## Testing Guidelines

There is no central automated test suite. Verify changes by building the affected project and, for hardware-facing changes, flashing/debugging on STM32F103C8 hardware when possible. For compile checks, confirm device macros and include paths are active before debugging source-level errors.

## Commit & Pull Request Guidelines

Existing history uses short imperative commit subjects such as `Add Serial_port_printf_String folder` and `Replace embedded repo with full folder contents`. Continue that style: describe the concrete change in one line.

Pull requests should name the affected project folder, summarize the hardware or build impact, list verification performed, and include wiring screenshots or serial output when behavior changes.

## Agent-Specific Instructions

Prefer minimal, project-local edits. Inspect the actual file, build log, or generated artifact before changing code. Do not refactor shared STM32 drivers, rename folders, or remove generated files unless the task explicitly asks for it.
