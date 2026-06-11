/**
 * @file    main.c
 * @brief   EXTI 外部中断实验 —— 两个按键分别控制 LED 亮/灭
 *
 * 硬件连接:
 *   PA5  -> 按键1（按下亮灯）
 *   PA6  -> 按键2（按下灭灯）
 *   PC13 -> 板载 LED（低电平点亮，开漏输出）
 *
 * 实验现象:
 *   按下 PA5 按键时，EXTI Line5 上升沿触发中断，点亮 PC13 LED；
 *   按下 PA6 按键时，EXTI Line6 上升沿触发中断，熄灭 PC13 LED。
 */

#include "Delay.h"
#include "stm32f10x.h"

void My_BoardLed_Init(void);
void App_Button_Init(void);
void EXTI9_5_IRQHandler(void);

int main(void)
{
    /* 配置 NVIC 中断分组：2 位抢占优先级 + 2 位子优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 初始化板载 LED（PC13，开漏输出，默认熄灭） */
    My_BoardLed_Init();

    /* 初始化按键 GPIO + EXTI + NVIC */
    App_Button_Init();

    /* 主循环为空，所有逻辑在中断中完成 */
    while (1)
    {
    }
}

/**
 * @brief   初始化按键 GPIO、EXTI 和 NVIC
 *
 * 流程:
 *   1. 配置 PA5/PA6 为上拉输入（默认高电平，按下接地）
 *   2. 开启 AFIO 时钟，将 PA5/PA6 映射到 EXTI Line5/Line6
 *   3. 配置 EXTI Line5/Line6 为上升沿触发中断模式
 *   4. 配置 NVIC，EXTI9_5 共享通道，抢占/子优先级均为 0
 */
void App_Button_Init(void)
{
    /* ---- 第一步：配置按键 GPIO ---- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PA5 - 上拉输入，未按下为高，按下为低 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA6 - 上拉输入 */
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ---- 第二步：AFIO 复用映射 ---- */
    /* 开启 AFIO 时钟，EXTI 引脚映射必须通过 AFIO 完成 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* 将 PA5 映射到 EXTI Line5，PA6 映射到 EXTI Line6 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource6);

    /* ---- 第三步：配置 EXTI ---- */
    EXTI_InitTypeDef EXTI_InitStruct = {0};

    /* EXTI Line5: 中断模式，上升沿触发（按键松开瞬间触发） */
    EXTI_InitStruct.EXTI_Line = EXTI_Line5;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStruct);

    /* EXTI Line6: 中断模式，上升沿触发 */
    EXTI_InitStruct.EXTI_Line = EXTI_Line6;
    EXTI_InitStruct.EXTI_LineCmd = ENABLE;
    EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
    EXTI_Init(&EXTI_InitStruct);

    /* ---- 第四步：配置 NVIC ---- */
    /* EXTI9_5 共享同一个中断通道（EXTI5 ~ EXTI9 合并） */
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);
}

/**
 * @brief   EXTI Line5 ~ Line9 中断服务函数
 *
 * EXTI5 ~ EXTI9 共享此中断入口，需在函数内判断具体是哪条线触发。
 *   - Line5（PA5 按键）：清除标志后点亮 LED（PC13 输出低电平）
 *   - Line6（PA6 按键）：清除标志后熄灭 LED（PC13 输出高电平）
 *
 * 注意：必须手动调用 EXTI_ClearFlag() 清除挂起位，否则会反复进入中断。
 */
void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetFlagStatus(EXTI_Line5) == SET)
    {
        EXTI_ClearFlag(EXTI_Line5);               // 清除 Line5 中断挂起标志
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET); // 点亮 LED（低电平有效）
    }
    else if (EXTI_GetFlagStatus(EXTI_Line6) == SET)
    {
        EXTI_ClearFlag(EXTI_Line6);               // 清除 Line6 中断挂起标志
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);   // 熄灭 LED
    }
}

/**
 * @brief   初始化板载 LED（PC13）
 *
 * PC13 配置为开漏输出模式（GPIO_Mode_Out_OD），默认输出高电平（熄灭）。
 * 注意：PC13 在大多数 Blue Pill 板上为低电平点亮。
 */
void My_BoardLed_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;  // 开漏输出，外部需上拉电阻
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);    // 初始状态：LED 熄灭
}
