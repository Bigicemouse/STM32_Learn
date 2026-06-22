/**
 * @file    main.c
 * @brief   TIM3 定时器中断实现精确延迟，驱动 PC13 LED 闪烁
 * @note    MCU: STM32F103C8 (72MHz)
 *          TIM3 时基: PSC=71, ARR=999 -> 72MHz/(72*1000)=1kHz，即 1ms 中断一次
 */

#include "Delay.h"
#include "stm32f10x.h"

void My_GPIO_Init(void);
void App_Delay(uint16_t time);
void App_TIM_TimeBaseInit(void);
void TIM3_IRQHandler(void);

/* 全局毫秒计数器，由 TIM3 中断每 1ms 自增 */
volatile uint16_t current_time = 0;

int main(void)
{
    /* 中断优先级分组：2 位抢占 + 2 位子优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    /* 初始化 PC13 LED 引脚 */
    My_GPIO_Init();

    /* 初始化 TIM3，产生 1ms 定时中断 */
    App_TIM_TimeBaseInit();

    while (1)
    {
        /* LED 亮（PC13 低电平点亮） */
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
        App_Delay(100);

        /* LED 灭 */
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
        App_Delay(100);
    }
}

/**
 * @brief  基于 TIM3 中断的毫秒延迟函数
 * @param  time  延迟时间（毫秒）
 * @note   通过比较 current_time 与目标值实现阻塞等待
 */
void App_Delay(uint16_t time)
{
    uint16_t expire_time = current_time + time;

    /* 阻塞等待直到 current_time 达到或超过目标值 */
    while (current_time <= expire_time)
        ;
}

/**
 * @brief  TIM3 时基单元初始化，配置为 1ms 中断周期
 * @note   时钟源 72MHz (APB1 x2)
 *         PSC=71  -> 分频后 1MHz (1us 计数一次)
 *         ARR=999 -> 每 1000 次计数溢出，即 1ms
 *         中断频率 = 72MHz / (71+1) / (999+1) = 1kHz
 */
void App_TIM_TimeBaseInit(void)
{
    /* 使能 TIM3 时钟（挂载在 APB1 总线） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 配置时基参数 */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct;
    TIM_TimeBaseInitStruct.TIM_Prescaler = 71;                          /* 预分频 72 分频 */
    TIM_TimeBaseInitStruct.TIM_Period = 999;                             /* 自动重装载值，计数 0~999 */
    TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;         /* 向上计数模式 */

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseInitStruct);

    /* 使能定时器 */
    TIM_Cmd(TIM3, ENABLE);

    /* 使能更新中断（溢出时触发） */
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    /* 配置 NVIC，TIM3 中断优先级 */
    NVIC_InitTypeDef NVIC_InitStruct = {0};
    NVIC_InitStruct.NVIC_IRQChannel = TIM3_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;   /* 抢占优先级 0 */
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;          /* 子优先级 0 */
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStruct);
}

/**
 * @brief  TIM3 中断服务函数
 * @note   每次更新事件（溢出）将 current_time 自增 1，实现 1ms 累计
 */
void TIM3_IRQHandler(void)
{
    if (TIM_GetFlagStatus(TIM3, TIM_IT_Update) == SET)
    {
        TIM_ClearFlag(TIM3, TIM_IT_Update);   /* 清除更新标志，防止重复进入 */
        current_time++;
    }
}

/**
 * @brief  初始化 PC13 为开漏输出（板载 LED）
 * @note   STM32F103C8 最小系统板 LED 通常为低电平点亮
 */
void My_GPIO_Init(void)
{
    /* 使能 GPIOC 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;       /* 开漏输出 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOC, &GPIO_InitStruct);
}
