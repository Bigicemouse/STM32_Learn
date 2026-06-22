/**
 * @file    main.c
 * @brief   超声波测距模块主程序
 * @note    直接读取Echo引脚高电平持续时间，结合声速换算为距离，
 *          通过USART1串口输出结果
 */

#include "delay.h"
#include "stm32f10x.h"

#include "usart.h"

void App_USART_Init(void);
void App_HC_SR04_GPIO_Init(void);

typedef enum
{
    HC_SR04_OK = 0,
    HC_SR04_ECHO_STUCK_HIGH,
    HC_SR04_ECHO_NO_RISING,
    HC_SR04_ECHO_NO_FALLING
} HC_SR04_Result;

static void HC_SR04_Trig(void);
static HC_SR04_Result HC_SR04_MeasureByGpio(uint32_t *pulse_width_us);
static void PrintDistance(uint32_t pulse_width_us);
static void PrintMeasureError(HC_SR04_Result result);
static void PrintBootInfo(void);

/**
 * @brief  主函数 — 超声波测距流程入口
 * @note   测距原理: 发送10us触发脉冲 → 传感器发出超声波 →
 *         读取Echo高电平持续时间 → 换算为距离并串口输出
 */
int main(void)
{
    App_USART_Init();
    App_HC_SR04_GPIO_Init();
    PrintBootInfo();

    while (1)
    {
        uint32_t pulse_width = 0;
        HC_SR04_Result result = HC_SR04_MeasureByGpio(&pulse_width);

        if (result == HC_SR04_OK)
            PrintDistance(pulse_width);
        else
            PrintMeasureError(result);

        Delay(2000);
    }
}

static void HC_SR04_Trig(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
    DelayUs(2);
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
    DelayUs(10);
    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
}

static HC_SR04_Result HC_SR04_MeasureByGpio(uint32_t *pulse_width_us)
{
    uint64_t deadline = GetUs() + 30000;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == Bit_SET)
    {
        if (GetUs() >= deadline)
            return HC_SR04_ECHO_STUCK_HIGH;
    }

    HC_SR04_Trig();

    deadline = GetUs() + 30000;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == Bit_RESET)
    {
        if (GetUs() >= deadline)
            return HC_SR04_ECHO_NO_RISING;
    }

    uint64_t rising_time = GetUs();
    deadline = rising_time + 30000;
    while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == Bit_SET)
    {
        if (GetUs() >= deadline)
            return HC_SR04_ECHO_NO_FALLING;
    }

    *pulse_width_us = (uint32_t)(GetUs() - rising_time);
    return HC_SR04_OK;
}

static void PrintDistance(uint32_t pulse_width_us)
{
    // 声速340m/s，除以2得到单程距离，结果换算成厘米(cm)
    float distance_cm = pulse_width_us * 0.017f;
    My_USART_Printf(USART1, "%.3f cm\n", distance_cm);
}

static void PrintMeasureError(HC_SR04_Result result)
{
    if (result == HC_SR04_ECHO_STUCK_HIGH)
        My_USART_Printf(USART1, "timeout: PA8 is stuck HIGH before Trig, check Echo short to 5V or wrong pin\n");
    else if (result == HC_SR04_ECHO_NO_RISING)
        My_USART_Printf(USART1, "timeout: PA8 has no rising edge, check Trig->PA0 and Echo->PA8 wiring\n");
    else if (result == HC_SR04_ECHO_NO_FALLING)
        My_USART_Printf(USART1, "timeout: PA8 rising ok but no falling edge, check Echo line or target range\n");
    else
        My_USART_Printf(USART1, "timeout: unknown HC-SR04 measure error\n");
}

static void PrintBootInfo(void)
{
    BitAction pa8_state = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) ? Bit_SET : Bit_RESET;

    My_USART_Printf(USART1, "HC-SR04 GPIO diag 2026-06-19 PA0=Trig PA8=Echo\n");
    My_USART_Printf(USART1, "boot: PA8=%s\n", (pa8_state == Bit_SET) ? "HIGH" : "LOW");
}

/**
 * @brief  串口外设初始化
 * @note   PA9被复用为USART1_TX引脚，波特率115200，仅配置TX用于串口打印输出
 */
void App_USART_Init()
{
    // 使能GPIOA外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 仅配置TX引脚，无需RX
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出，供USART使用
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 使能USART1外设时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // USART基本参数配置
    USART_InitTypeDef USART_InitStruct = {0};
    USART_InitStruct.USART_BaudRate = 115200;                // 波特率: 115200
    USART_InitStruct.USART_Mode = USART_Mode_Tx;             // 工作模式: 仅使能发送TX
    USART_InitStruct.USART_Parity = USART_Parity_No;         // 校验位: 无校验位
    USART_InitStruct.USART_StopBits = USART_StopBits_1;      // 停止位: 1个停止位
    USART_InitStruct.USART_WordLength = USART_WordLength_8b; // 数据位长度: 8位

    USART_Init(USART1, &USART_InitStruct);
    USART_Cmd(USART1, ENABLE);
}
/**
 * @brief  HC-SR04 GPIO初始化
 * @note   PA0输出Trig，PA8输入Echo；测量逻辑与Arduino pulseIn版本一致
 */
void App_HC_SR04_GPIO_Init()
{
    // 使能GPIOA时钟(PA0为Trig，PA8为Echo)
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStrut = {0};
    GPIO_InitStrut.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出，驱动Trig信号
    GPIO_InitStrut.GPIO_Pin = GPIO_Pin_0;        // PA0接HC-SR04的Trig
    GPIO_InitStrut.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &GPIO_InitStrut);

    GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);

    GPIO_InitStrut.GPIO_Mode = GPIO_Mode_IPD; // 下拉输入，防止Echo悬空误触发
    GPIO_InitStrut.GPIO_Pin = GPIO_Pin_8;     // PA8接HC-SR04的Echo
    GPIO_Init(GPIOA, &GPIO_InitStrut);
}
