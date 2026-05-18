#include "delay.h"
#include "stm32f10x.h"

int main(void)
{
    // ---------------- 1. 硬件初始化阶段 ----------------

    // 开启 GPIOA/GPIOC 时钟 (必须最先执行)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    Delay_Init(); // 显式初始化 SysTick 时基，避免首次按键时才初始化

    GPIO_InitTypeDef GPIO_InitStruct = {0}; // 定义一次，全程复用

    // --- 配置 LED (PA0) ---
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;        // 选择引脚0
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP; // 输出推挽模式 (驱动LED)
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz; // 输出速度
    GPIO_Init(GPIOA, &GPIO_InitStruct);           // 写入 PA0 配置
    // 初始化 LED 状态：默认熄灭
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET); //    LED正极接PA0，输出高电平时点亮

    // 将gpioa1设置为输入上拉
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ---------------- 2. 主循环逻辑 ----------------
    while (1)
    {

        // 读取按键状态
        BitAction keyState = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1); // 读取PA1状态

        if (keyState == Bit_RESET) // 按键按下时，PA1被拉低
        {
            Delay(20); // 机械按键去抖，延时20ms后再次确认
            keyState = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1);

            if (keyState == Bit_RESET) // 点亮LED
            {
                GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
            }
            else // 熄灭LED
            {
                GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
            }
        }

        // 熄灭LED
        else
        {
            GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
        }
    }
}
