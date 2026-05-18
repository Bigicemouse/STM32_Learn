#include "stm32f10x.h"

#include "delay.h"
int main(void)
{
    // === 步骤 1. 开启时钟 ===
    // 信号流：系统时钟 -> APB2 总线 -> 激活 GPIOC 模块
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    // === 步骤 2. 定义配置结构体 ===
    // GPIO_InitTypeDef 是一个---结构体---，内部包含 Pin(哪一根), Speed(速度), Mode(模式)
    GPIO_InitTypeDef GPIO_InitStructure;

    // === 步骤 3. 填写清单 ===
    // 1. 选择第 13 号引脚 (C8T6 板载 LED 所在位置)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;

    // 2. 设置为推挽输出 (Push-Pull)
    // 推挽模式下引脚能输出稳定的 3.3V (高) 或 0V (低)
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;

    // 3. 设置输出速度 50MHz (这是引脚翻转的物理响应速度)
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;

    // === 步骤 4. 调用初始化函数 ===
    // GPIOC 是目标端口（寄存器首地址）
    // &GPIO_InitStructure 使用了取地址符，将配置清单的“指针”传递给函数
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // === 步骤 5. 程序主循环 ===
    while (1)
    {
        // GPIO_WriteBit(端口, 引脚, 电平状态)

        // 1. 点亮 LED (PC13 输出低电平，形成电势差)
        // Bit_RESET 代表逻辑 0，引脚对地导通
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
        Delay(100);
        // 2. 熄灭 LED (PC13 输出高电平)
        // Bit_SET 代表逻辑 1，引脚输出 3.3V，LED 两端均为 3.3V，无电流
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
        Delay(100);
    }
}
