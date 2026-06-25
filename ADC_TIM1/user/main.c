#include "Delay.h"
#include "stm32f10x.h"
#include "usart.h"

/**
 * @brief  功能说明：使用ADC实现定时器TIM1触发的注入通道单次转换，
 *         并通过USART1将电压值发送到VOFA上位机显示波形。
 *
 * 硬件连接：
 *   - PA0  → ADC1通道0（模拟输入，接电位器或传感器）
 *   - PA9  → USART1_TX（串口发送，连接上位机）
 *
 * 工作原理：
 *   TIM1以1kHz频率产生TRGO更新事件 → 触发ADC注入组转换 →
 *   转换完成后读取JEOC标志和注入数据寄存器 → 换算为电压值 →
 *   通过串口发送给VOFA上位机显示波形。
 *
 * ADC时钟：
 *   PCLK2 = 72MHz，6分频后ADCCLK = 12MHz
 *   采样时间 = 13.5周期，转换时间 = 13.5 + 12.5 = 26周期 ≈ 2.17μs
 */

void App_USART_Init(void);
void App_TIM_Init(void);
void App_ADC_Init(void);

int main(void)
{
    /* 三步初始化：USART → TIM1 → ADC1 */
    App_USART_Init();
    App_TIM_Init();
    App_ADC_Init();

    /* 开机后先手动清一次JEOC标志，避免首次读到残留状态 */
    ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);

    while (1)
    {
        /* 等待TIM1触发的注入转换完成（JEOC置1） */
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_JEOC) == RESET)
            ;

        /* 清JEOC标志，为下次转换做准备 */
        ADC_ClearFlag(ADC1, ADC_FLAG_JEOC);

        /* 读取注入通道1的12位转换结果 */
        uint16_t res = ADC_GetInjectedConversionValue(ADC1, ADC_InjectedChannel_1);

        /* 将ADC原始值换算为电压：V = ADC值 × (参考电压 / 分辨率) */
        float volt = res * (3.3f / 4095);

        /* 通过串口发送电压值到VOFA上位机 */
        My_USART_Printf(USART1, "%.3f\n", volt);

        /* 延时500ms，控制串口输出频率 */
        Delay(500);
    }
}

/**
 * @brief  初始化TIM1，配置为1kHz的TRGO触发源
 *
 * 计算：72MHz / (71+1) / (999+1) = 72MHz / 72 / 1000 = 1kHz
 *   - 预分频器 PSC = 71 → 分频后计数频率 = 1MHz（1μs计数一次）
 *   - 自动重装载 ARR = 999 → 每1000次计数（1ms）产生一次更新事件
 *   - TRGO输出源设为更新事件，用于触发ADC注入转换
 */
void App_TIM_Init(void)
{
    /* 开启TIM1时钟（挂载在APB2总线上） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    /* 时基配置：向上计数，1kHz更新频率 */
    TIM_TimeBaseInitTypeDef TIM_InitStruct = {0};
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_InitStruct.TIM_Period = 999;   // ARR = 999
    TIM_InitStruct.TIM_Prescaler = 71; // PSC = 71
    TIM_InitStruct.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_InitStruct);

    /* 使能ARR预装载，更新事件在下一个计数周期生效 */
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* 设置TRGO触发源为更新事件（Update），每次溢出产生一个触发脉冲 */
    TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update);

    /* 启动定时器 */
    TIM_Cmd(TIM1, ENABLE);
}

/**
 * @brief  初始化USART1，PA9为TX，115200-8-N-1，仅发送模式
 */
void App_USART_Init(void)
{
    /* 配置PA9为复用推挽输出（USART1_TX） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 初始化USART1：115200波特率，8位数据，无校验，1位停止位，仅发送 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitTypeDef u_i = {0};
    u_i.USART_BaudRate = 115200;
    u_i.USART_Mode = USART_Mode_Tx;
    u_i.USART_Parity = USART_Parity_No;
    u_i.USART_StopBits = USART_StopBits_1;
    u_i.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &u_i);

    /* 使能USART1 */
    USART_Cmd(USART1, ENABLE);
}
/**
 * @brief  初始化ADC1，使用注入通道0（PA0），由TIM1 TRGO触发转换
 *
 * ADC时钟配置：
 *   PCLK2 = 72MHz，6分频 → ADCCLK = 12MHz（不超过14MHz限制）
 *
 * 注入通道说明：
 *   注入组相当于常规组的"高优先级中断"，可以被外部事件触发，
 *   适合定时采样场景。比常规转换多配置：注入序列长度、外部触发源、
 *   外部触发使能。
 *
 * 采样时间选择：
 *   输入源内阻已知，根据数据手册选择13.5周期采样时间（约1.125μs），
 *   满足采样精度要求。
 */
void App_ADC_Init(void)
{
    /* 配置PA0为模拟输入（ADC1通道0） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN; // 模拟输入模式，无需设置速度
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC时钟：72MHz / 6 = 12MHz */
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    /* ADC基础配置：独立模式，单通道，非连续，非扫描，右对齐 */
    ADC_InitTypeDef ADC_InitStruct = {0};
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;                 // 单次转换模式
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;              // 数据右对齐
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 常规组不使用外部触发
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;                  // 独立模式
    ADC_InitStruct.ADC_NbrOfChannel = 1;
    ADC_InitStruct.ADC_ScanConvMode = DISABLE; // 单通道无需扫描
    ADC_Init(ADC1, &ADC_InitStruct);

    /* 配置注入通道（比常规通道多3步配置） */
    ADC_InjectedSequencerLengthConfig(ADC1, 1);                                  // 注入序列长度=1
    ADC_ExternalTrigInjectedConvConfig(ADC1, ADC_ExternalTrigInjecConv_T1_TRGO); // 触发源=TIM1 TRGO

    ADC_InjectedChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_13Cycles5); // 通道0，序号1，13.5周期采样
    ADC_ExternalTrigInjectedConvCmd(ADC1, ENABLE);                               // 使能外部触发

    /* 开启ADC1 */
    ADC_Cmd(ADC1, ENABLE);
}
