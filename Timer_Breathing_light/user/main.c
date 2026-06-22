/**
 * @file    main.c
 * @brief   TIM1 PWM 呼吸灯实验
 * @note    使用 TIM1 通道 1 (PA8) 和互补通道 1 (PB13) 驱动两个 LED，
 *          通过正弦函数改变占空比实现呼吸效果，两个 LED 亮度互补交替。
 *          PWM 频率 = (72MHz / (PSC+1))   / (ARR+1) = 72M / 72 / 1000 = 1kHz
 *         占空比 = CCR / ARR，CCR 由正弦函数动态计算，范围 0~999
 */

#include "delay.h"
#include "stm32f10x.h"
#include <math.h>

void App_GPIO_Init(void);
void App_TIM_Init(void);

int main(void)
{
    // 初始化 TIM1 通道 1 (PA8) 和互补通道 1 (PB13) 的 GPIO
    App_GPIO_Init();
    // 初始化 TIM1 时基单元和 PWM 输出比较
    App_TIM_Init();

    // 初始化完成后默认输出低电平（CCR1=0，占空比为 0）

    while (1)
    {
        // 用正弦函数生成 0~1 的呼吸占空比
        // sin 值域 [-1,1] → (sin+1)/2 映射到 [0,1]，周期 = 1s
        float t = GetTick() * 1.0e-3f;                      // 毫秒转秒
        float duty = 0.5f * (sin(0.5f * 3.14f * t) + 1.0f); // 占空比 0~1，周期4秒
        uint16_t ccr1 = (uint16_t)(duty * 999);             // 映射到 ARR 值

        TIM_SetCompare1(TIM1, ccr1); // 更新 CCR1，改变 PWM 占空比
    }
}

/**
 * @brief  初始化 TIM1 时基单元和 PWM 输出
 * @note   时钟源 72MHz，预分频 72 → 计数频率 1MHz，ARR=999 → PWM 频率 1kHz
 *         使用 PWM1 模式：CNT < CCR 时输出有效电平（高电平）
 */
void App_TIM_Init(void)
{
    // 1. 开启 TIM1 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    // 2. 开启 ARR 预加载，写入新值后在下一个更新事件生效
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    // 3. 配置时基单元：PSC=71, ARR=999, 向上计数
    TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.TIM_Prescaler = 71;                   // 72MHz / 72 = 1MHz 计数频率
    TIM_InitStruct.TIM_Period = 999;                     // 0~999，共 1000 个计数
    TIM_InitStruct.TIM_RepetitionCounter = 0;            // 不重复计数
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up; // 向上计数
    TIM_TimeBaseInit(TIM1, &TIM_InitStruct);

    // 4. 使能时基单元，开始计数
    TIM_Cmd(TIM1, ENABLE);

    // 5. 配置通道 1 输出比较为 PWM1 模式
    TIM_OCInitTypeDef TIM_OC1InitStruct = {0};
    TIM_OC1InitStruct.TIM_OCMode = TIM_OCMode_PWM1; // PWM1：CNT < CCR 输出有效

    TIM_OC1InitStruct.TIM_OCPolarity = TIM_OCPolarity_High;   // 有效电平为高
    TIM_OC1InitStruct.TIM_OCNPolarity = TIM_OCNPolarity_High; // 互补输出有效电平也为高
    TIM_OC1InitStruct.TIM_Pulse = 0;                          // 初始占空比 0

    TIM_OC1InitStruct.TIM_OutputState = TIM_OutputState_Enable;   // 使能主输出 (CH1)
    TIM_OC1InitStruct.TIM_OutputNState = TIM_OutputNState_Enable; // 使能互补输出 (CH1N)
    TIM_OC1Init(TIM1, &TIM_OC1InitStruct);

    // 6. 使能 MOE（主输出使能），高级定时器必须手动开启
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    // 7. 使能 CCR1 预加载，写入后在下一个更新事件生效，避免毛刺
    TIM_CCPreloadControl(TIM1, ENABLE);
}

/**
 * @brief  初始化 TIM1 通道 1 及互补通道的 GPIO
 * @note   CH1  → PA8  (TIM1_CH1)
 *         CH1N → PB13 (TIM1_CH1N)
 *         均配置为复用推挽输出
 */
void App_GPIO_Init(void)
{
    // PA8 — TIM1 通道 1 主输出
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_8;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PB13 — TIM1 通道 1 互补输出
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);
}
