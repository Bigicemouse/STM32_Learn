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
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU; // 默认高电平
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // ---------------- 2. 主循环逻辑 ----------------
    while (1)
    {
        // 按键状态检测（带消抖）
        // 说明：PA1 配置为上拉输入，未按下时为高电平(1)，按下时接地为低电平(0)

        if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_RESET) // 检测到按键按下（低电平）
        {
            Delay(20); // 延时20ms消除机械抖动（典型按键抖动时间为5-20ms）

            // 再次确认按键状态，排除干扰信号
            if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_RESET)
            {
                // 切换 LED 状态（按一下开，再按一下关）
                // 读取当前 LED 状态，取反后写回
                BitAction currentLed = GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_0);
                GPIO_WriteBit(GPIOA, GPIO_Pin_0, (currentLed == Bit_RESET) ? Bit_SET : Bit_RESET);

                // 等待按键释放，避免长按连续触发
                while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == Bit_RESET)
                {
                    // 占位，直到松开按键
                }
            }
        }
    }
}
