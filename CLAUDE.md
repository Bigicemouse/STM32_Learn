# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Overview

This is a collection of STM32F103C8 embedded development projects using the STM32 Standard Peripheral Library (StdPeriph). Each top-level folder is an independent project with a common structure. Target MCU: STM32F103C8 (Cortex-M3, Medium Density).

## Project Structure

Each project follows this layout:
- `user/` - Application entry points (`main.c`), interrupt handlers (`stm32f10x_it.c`), and STM32 system files
- `my_lib/` - Reusable board-level drivers (delay, USART, I2C, SPI, OLED, button)
- `std_periph_driver/` - STM32 Standard Peripheral Library (headers in `inc/`, sources in `src/`)
- `startup/` - Cortex-M3 startup assembly (`startup_stm32f10x_md.s`)
- `.eide/` - EIDE extension configuration for VS Code
- `*.uvprojx` - Keil uVision project files

## Build System

### Option 1: Keil uVision (Primary)
Open the `.uvprojx` file in Keil uVision. Build target is configured for:
- Compiler: ARMCC5
- CPU: Cortex-M3
- Device: STM32F10X_MD
- Memory: 64KB Flash (0x08000000), 20KB RAM (0x20000000)

### Option 2: VS Code + EIDE Extension
Open the `.code-workspace` file. Requires the EIDE extension (recommended extensions are listed in workspace file). Build via EIDE commands.

### Local EIDE/AC5 Toolchain Notes

- EIDE projects may use target `default` with toolchain `AC5`.
- The local ARMCC5 compiler path can be `C:\software\Keilv5\Core\ARM\ARMCC\bin\armcc.exe`.
- If `armcc` is not available on `PATH`, inspect `.eide/eide.yml`, `build/<target>/builder.params`, or `build/<target>/compile_commands.json`, then invoke the absolute compiler path.
- EIDE's `unify_builder.exe` may require `.NET 6.0.0`. If the machine only has a newer .NET runtime, the builder can fail before invoking ARMCC; use a focused ARMCC5 compile command for the edited source file when full EIDE CLI build is blocked.
- In `Button_code_encapsulation`, `user/main.c` and `user/stm32f10x_it.c` were verified with ARM Compiler 5.06 update 7 using EIDE's include paths and defines.

### Key Compiler Flags
- `STM32F10X_MD` - Medium density device
- `USE_STDPERIPH_DRIVER` - Enable Standard Peripheral Library
- `--no-multibyte-chars` - Required for Chinese comments

### Cleaning Build Artifacts
Each project has a cleanup script:
```cmd
.\ProjectName\清理.cmd
```
This removes build output (build/, Objects/, Listings/), Keil cache, and EIDE temporary files.

## Coding Conventions

- **Language**: C (not C++)
- **Style**: Microsoft base style, 4-space indentation, no tabs
- **Formatting**: See `.clang-format` in each project root
- **API Style**: Use StdPeriph API consistently (e.g., `GPIO_InitTypeDef`, `USART_InitTypeDef`, `RCC_APB2PeriphClockCmd`)
- **Don't mix**: HAL, LL, or CubeMX code styles into these projects
- **Include paths**: `std_periph_driver/inc`, `user`, `my_lib`, `my_lib/font`, `.cmsis/include`
- **注释风格**: 简单行内注释使用 `//`，函数/文件头部的文档注释使用 `/** */` 或 `/* */`。不要对简单注释使用 `/* */`

## Common Drivers (my_lib/)

Reusable drivers available across projects:
- `delay.c/h` - SysTick-based delay functions
- `usart.c/h` - USART initialization and communication
- `i2c.c/h` / `si2c.c/h` - I2C bus drivers (hardware and software)
- `spi.c/h` - SPI bus driver
- `oled.c/h` - OLED display driver
- `button.c/h` - Button input handling

## Verification

No automated test suite. Verify changes by:
1. Building the project (confirm no compile errors)
2. For hardware-facing changes: flash to STM32F103C8 and test on hardware
3. Check that device macros (`STM32F10X_MD`, `USE_STDPERIPH_DRIVER`) are active in build settings

## Commit Style

Use short imperative subjects: `Add Serial_port_printf_String folder`, `Replace embedded repo with full folder contents`

## Projects

- `BlinkyLED/` - Basic GPIO LED blinking example
- `gpio_key/` - Button input with GPIO
- `Serial_port_printf_String/` - USART printf output example
- `UART_Comm/` - UART communication
- `USART_RX/` - USART receive with LED control
- `OLED/` - OLED display interface
- `stm32f103_stdperiph_template/` - Base template for new projects
