#include "delay.h"
#include "stm32f10x.h"
#include "usart.h"

/* PC13 LED - 低电平点亮 */

/* 默认亮 1 秒、灭 1 秒；串口中断可修改闪烁速度 */
volatile uint32_t blinkInterval = 1000;

static void LED_Init(void);
static void USART1_Init(void);

int main(void)
{
    Delay_Init();
    LED_Init();
    USART1_Init();

    GPIO_SetBits(GPIOC, GPIO_Pin_13);

    My_USART_SendString(USART1, "LED Control Ready\r\n");
    My_USART_SendString(USART1, "0=Slow 1=Middle 2=Fast\r\n");

    // 主循环：根据 blinkInterval 控制 LED 闪烁
    while (1)
    {

        GPIO_ResetBits(GPIOC, GPIO_Pin_13); // LED 亮
        Delay(blinkInterval);
        GPIO_SetBits(GPIOC, GPIO_Pin_13); // LED 灭
        Delay(blinkInterval);
    }
}

/**
 * @brief  初始化 PC13 LED (推挽输出)
 */
static void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/**
 * @brief  初始化 USART1 (PA9=TX, PA10=RX, 115200-8-N-1)
 */
static void USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    USART_InitTypeDef USART_InitStruct;
    NVIC_InitTypeDef NVIC_InitStruct;

    // 开启时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    // 1  PA9 - TX 复用推挽
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 - RX 输入上拉
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 2  USART1 参数
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStruct);

    // 开启 USART1 接收中断
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

    //   3  配置 NVIC
    // 设置中断优先级分组：2位抢占优先级(0-3) + 2位子优先级(0-3)
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    NVIC_InitStruct.NVIC_IRQChannel = USART1_IRQn;         // USART1 中断名称
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0; // 抢占优先级：0（最高）
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;        // 子优先级：0（最高）
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;           // 使能中断

    NVIC_Init(&NVIC_InitStruct); // 应用 NVIC 配置

    USART_Cmd(USART1, ENABLE);
}

/**
 * @brief  USART1 中断服务函数
 * @note   虽然只开启了 RXNE 中断，但 ORE(溢出错误)也会触发此中断，
 *         因此需要检查 RXNE 标志位，确保是真正接收到数据才处理
 */
void USART1_IRQHandler(void)
{
    if (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)
    {
        uint8_t byte = USART_ReceiveData(USART1);

        if (byte == '0')
            blinkInterval = 1000; // 慢闪
        if (byte == '1')
            blinkInterval = 200; // 中速
        if (byte == '2')
            blinkInterval = 50; // 快闪
    }
}
