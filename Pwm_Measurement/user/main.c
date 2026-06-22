/**
 * @file    main.c
 * @brief   PWM 信号测量：TIM3 输出 PWM，TIM1 输入捕获测量周期和占空比
 *
 * 原理：
 *   - TIM3 CH1 (PA6) 输出 PWM 信号，周期 1ms，占空比可调
 *   - TIM1 CH1 (PA8) 输入捕获，利用从模式自动复位计数器
 *   - CH1 上升沿捕获获得周期（CCR1），CH2 下降沿捕获获得高电平时间（CCR2）
 *   - 占空比 = CCR2 / CCR1 × 100%
 *
 * 硬件连接：PA6 (TIM3_CH1 PWM 输出) → PA8 (TIM1_CH1 输入捕获)
 */

#include "Delay.h"
#include "stm32f10x.h"
#include "usart.h"

/* 函数声明 */
void App_TIM3_OutPutPWM(void);    // TIM3 输出 PWM
void App_USART_Init(void);        // USART1 初始化
void App_TimeBaseInit_Init(void); // TIM1 时基初始化
void App_IC_Init(void);           // TIM1 输入捕获初始化

int main(void)
{
    // 使能 TIM3 CH1 输出 PWM 信号
    App_TIM3_OutPutPWM();

    // 使能 USART1 用于串口打印测量结果
    App_USART_Init();
    My_USART_Printf(USART1, "TIM1 IC PWM Measurement\r\n");

    // 初始化 TIM1 时基单元（输入捕获的计数基础）
    App_TimeBaseInit_Init();

    // 配置 TIM1 输入捕获通道（CH1 上升沿测周期，CH2 下降沿测高电平）
    App_IC_Init();

    // 设置 PWM 占空比：周期 1ms = 1000us，CCR=200 即 20% 占空比
    TIM_SetCompare1(TIM3, 200);

    while (1)
    {
        // 清除 TIM1 触发标志，等待下一次捕获完成（从模式复位后置位）
        TIM_ClearFlag(TIM1, TIM_FLAG_Trigger);

        while (TIM_GetFlagStatus(TIM1, TIM_FLAG_Trigger) == RESET)
            ;

        // 读取捕获值：CCR1 = 周期（上升沿到上升沿），CCR2 = 高电平时间（上升沿到下降沿）
        uint16_t ccr1 = TIM_GetCapture1(TIM1);
        uint16_t ccr2 = TIM_GetCapture2(TIM1);

        // 计算周期（ms）：CCR1 单位为 us（72MHz / 72 分频 = 1MHz 计数），转为 ms
        float period = ccr1 * 1.0e-6f * 1.0e3f;

        // 计算占空比（%）：高电平时间 / 周期 × 100
        float duty = ((float)ccr2 / ccr1) * 100.0f;

        // 打印测量结果
        My_USART_Printf(USART1, "Period: %0.3fms, Duty: %.2f%% \r\n", period, duty);

        Delay(500);
    }
}

/**
 * @brief  TIM3 输出 PWM 信号
 *
 * 配置 TIM3 CH1 (PA6) 输出 PWM1 模式：
 *   - 时钟：72MHz (APB1 × 2)
 *   - 预分频：72 → 计数频率 1MHz (1us)
 *   - 自动重装：999 → 周期 1000us = 1ms (1kHz)
 *   - 占空比由 TIM_SetCompare1() 动态调节
 */
void App_TIM3_OutPutPWM(void)
{
    // 使能 TIM3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    // 时基配置：72 分频，999 自动重装，向上计数
    TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.TIM_Prescaler = 71; // 72MHz / 72 = 1MHz
    TIM_InitStruct.TIM_Period = 999;   // 1MHz / 1000 = 1kHz
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_InitStruct);

    // 使能预装载，写入 ARR 后等待更新事件再生效
    TIM_ARRPreloadConfig(TIM3, ENABLE);

    // 启动定时器
    TIM_Cmd(TIM3, ENABLE);

    // 配置 PA6 为复用推挽输出（TIM3_CH1）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 输出比较配置：PWM1 模式，高电平有效
    TIM_OCInitTypeDef TIM_OC_InitStruct = {0};
    TIM_OC_InitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OC_InitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OC_InitStruct.TIM_Pulse = 0; // 初始占空比 0
    TIM_OC1Init(TIM3, &TIM_OC_InitStruct);

    // 使能 CH1 预装载，写入 CCR 后等待更新事件再生效
    TIM_CCPreloadControl(TIM3, ENABLE);
}

/**
 * @brief  USART1 初始化
 *
 * 配置 USART1 (PA9) 发送，115200-8-N-1，用于打印测量结果
 */
void App_USART_Init(void)
{
    // 配置 PA9 为复用推挽输出（USART1_TX）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 使能 USART1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // USART 参数配置
    USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_Mode = USART_Mode_Tx; // 仅发送
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART_InitStruct);

    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  TIM1 时基初始化
 *
 * 配置 TIM1 时基单元，用于输入捕获的计数：
 *   - 预分频：72 → 计数频率 1MHz (1us 精度)
 *   - 自动重装：65535 → 最大可测周期约 65ms
 *   - 从模式会自动复位计数器，所以 ARR 设最大即可
 */
void App_TimeBaseInit_Init(void)
{
    // 使能 TIM1 时钟（APB2 总线）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 时基配置：72 分频，ARR 最大值，向上计数
    TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.TIM_Prescaler = 71; // 72MHz / 72 = 1MHz
    TIM_InitStruct.TIM_Period = 65535; // 最大计数值
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM1, &TIM_InitStruct);

    // 使能预装载
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    // 启动定时器
    TIM_Cmd(TIM1, ENABLE);
}

/**
 * @brief  TIM1 输入捕获初始化
 *
 * 配置 TIM1 双通道输入捕获测量 PWM：
 *   - CH1 (PA8)：上升沿，直连 TI1 → 捕获完整周期到 CCR1
 *   - CH2：下降沿，交叉连接 TI1 → 捕获高电平时间到 CCR2
 *   - 从模式：TI1FP1 上升沿自动复位计数器，实现连续自动测量
 */
void App_IC_Init(void)
{
    // 配置 PA8 为浮空输入（TIM1_CH1 输入捕获）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD; // 下拉输入，确保空闲时为低
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // CH1 配置：直连 TI1，上升沿捕获，无滤波，无分频
    // → 上升沿到来时将计数值锁存到 CCR1（即一个完整周期的计数）
    TIM_ICInitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.TIM_Channel = TIM_Channel_1;
    TIM_InitStruct.TIM_ICFilter = 0;
    TIM_InitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
    TIM_InitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_InitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICInit(TIM1, &TIM_InitStruct);

    // CH2 配置：交叉连接 TI1（IndirectTI），下降沿捕获
    // → 下降沿到来时将计数值锁存到 CCR2（即高电平持续时间）
    TIM_InitStruct.TIM_Channel = TIM_Channel_2;
    TIM_InitStruct.TIM_ICFilter = 0;
    TIM_InitStruct.TIM_ICPolarity = TIM_ICPolarity_Falling;
    TIM_InitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_InitStruct.TIM_ICSelection = TIM_ICSelection_IndirectTI;
    TIM_ICInit(TIM1, &TIM_InitStruct);

    // 触发源选择：TI1 的滤波后信号作为触发输入
    TIM_SelectInputTrigger(TIM1, TIM_TS_TI1FP1);

    // 从模式：触发信号（TI1 上升沿）自动复位计数器
    // 这样每次上升沿 CCR1 锁存周期值后计数器归零重新开始
    TIM_SelectSlaveMode(TIM1, TIM_SlaveMode_Reset);
}
