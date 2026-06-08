# STM32_Learn

这是一个基于 STM32F103C8T6 和 STM32 Standard Peripheral Library 的学习示例仓库。每个顶层目录都是一个相对独立的工程，用来练习 GPIO、按键、串口、OLED、SPI、中断等基础外设。

## 项目环境

- MCU: STM32F103C8T6
- 固件库: STM32F10x Standard Peripheral Library
- IDE: Keil uVision 或 VS Code + EIDE
- 编译器: ARM Compiler 5
- 常用宏: `STM32F10X_MD`, `USE_STDPERIPH_DRIVER`

## 目录说明

| 目录 | 内容 |
| --- | --- |
| `BlinkyLED/` | PC13 板载 LED 闪烁入门示例。 |
| `gpio_key/` | GPIO 输入按键控制 LED，包含基础消抖逻辑。 |
| `Button_code_encapsulation/` | 按键驱动封装示例，支持点击、双击和长按回调。 |
| `Serial_port_printf_String/` | USART1 初始化和 `printf` 重定向输出示例。 |
| `UART_Comm/` | USART1 收发回环示例，用于验证串口通信链路。 |
| `USART_RX/` | USART1 接收数据控制 LED，支持字符和原始字节命令。 |
| `Interrupt_learn/` | USART 接收中断控制 LED 闪烁速度示例。 |
| `OLED_I2C/` | I2C 驱动 OLED 显示，中英文混合显示示例。 |
| `SPI/` | SPI1 主机全双工收发示例，包含 SPI1 引脚重映射。 |
| `stm32f103_stdperiph_template/` | STM32F103 StdPeriph 工程模板。 |

## 常见工程结构

大多数示例目录都使用类似结构：

```text
project/
├── user/                 # main.c、中断文件、系统启动配置
├── my_lib/               # delay、USART、button、OLED、I2C、SPI 等自定义驱动
├── std_periph_driver/    # STM32 标准外设库源码和头文件
├── startup/              # 启动文件
├── .eide/                # EIDE 工程配置
├── *.uvprojx             # Keil 工程文件
└── *.code-workspace      # VS Code 工作区
```

## 使用方式

### 使用 Keil

进入对应项目目录，打开同名 `.uvprojx` 文件，例如：

```text
OLED_I2C/OLED_I2C.uvprojx
SPI/SPI.uvprojx
```

确认目标芯片、启动文件和宏定义后，直接编译并下载到开发板。

### 使用 VS Code + EIDE

打开项目对应的 `.code-workspace`，例如：

```powershell
code .\OLED_I2C\OLED_I2C.code-workspace
```

在 EIDE 中选择默认 target 构建。工程通常按 ARMCC5、Cortex-M3、`STM32F10X_MD` 配置。

## 硬件提示

- 板载 LED 常见连接为 `PC13`，低电平点亮。
- 串口示例多使用 USART1，部分工程将 USART1 从 `PA9/PA10` 重映射到 `PB6/PB7`。
- `SPI/` 示例使用 SPI1 重映射引脚：`SCK=PB3`、`MISO=PB4`、`MOSI=PB5`、`NSS=PA15`。
- `OLED_I2C/` 示例使用 I2C 驱动 OLED，并在刷新缓存后调用发送接口显示内容。

## 说明

本仓库用于 STM32 标准外设库学习和实验，重点是保留每个小工程的完整上下文，便于单独打开、编译、下载和对照学习。
