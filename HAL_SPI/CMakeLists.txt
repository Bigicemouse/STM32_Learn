cmake_minimum_required(VERSION 3.20)

project(HAL_SPI C ASM)

set(STM32_DEVICE_SOURCES
    Core/Src/main.c
    Core/Src/stm32f1xx_it.c
    Core/Src/stm32f1xx_hal_msp.c
    Core/Src/syscalls.c
    Core/Src/system_stm32f1xx.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_spi.c
    Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s
)

add_executable(${PROJECT_NAME}.elf ${STM32_DEVICE_SOURCES})

target_compile_definitions(${PROJECT_NAME}.elf PRIVATE
    USE_HAL_DRIVER
    STM32F103xB
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE
    Core/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32F1xx/Include
    Drivers/CMSIS/Include
)

target_compile_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
    -Wno-unused-parameter
    $<$<COMPILE_LANGUAGE:C>:-std=gnu11>
)

target_link_options(${PROJECT_NAME}.elf PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -mfloat-abi=soft
    -T${CMAKE_CURRENT_SOURCE_DIR}/cmake/STM32F103C8Tx_FLASH.ld
    --specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
    -Wl,--print-memory-usage
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMENT "Generating ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)

add_custom_target(size
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Displaying firmware size"
)
