#include "Delay.h"     // 延时函数头文件
#include "stm32f10x.h" // STM32F10x 器件头文件
#include <stdio.h>      // 标准输入输出库

/* 发送指定长度的数据缓冲区 */
void My_USART_SendBytes(USART_TypeDef *USARTx, const uint8_t *data, uint16_t Size);
/* 阻塞接收 1 字节数据 */
uint8_t My_USART_ReceiveByte(USART_TypeDef *USARTx);

int main(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    USART_InitTypeDef USART_InitStructure = {0};
    uint8_t hello[] = "USART1 ready, type to echo.\r\n";

    /*
     * USART1 当前使用重映射功能：
     * TX -> PB6
     * RX -> PB7
     * 因此需要同时使能 AFIO、GPIOB 和 USART1 时钟。
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOB | RCC_APB2Periph_USART1, ENABLE);

    /* 将 USART1 从默认 PA9/PA10 重映射到 PB6/PB7 */
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* 配置 PB6 为 USART1_TX：复用推挽输出 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 配置 PB7 为 USART1_RX：上拉输入，空闲时保持高电平 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 先复位 USART1，再释放复位，保证外设处于确定状态 */
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, ENABLE);
    RCC_APB2PeriphResetCmd(RCC_APB2Periph_USART1, DISABLE);

    /* USART1 基本通信参数：115200, 8N1, 无硬件流控，支持收发 */
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStructure);
    USART_Cmd(USART1, ENABLE);

    /* 上电后先发送一条提示信息，便于确认串口已经正常启动 */
    My_USART_SendBytes(USART1, hello, sizeof(hello) - 1);

    while (1)
    {
        uint8_t rxData;

        /* 阻塞等待上位机发来 1 字节数据 */
        rxData = My_USART_ReceiveByte(USART1);

        /* 收到什么就回发什么，用于最小化验证接收链路是否正常 */
        My_USART_SendBytes(USART1, &rxData, 1);
    }
}

void My_USART_SendBytes(USART_TypeDef *USARTx, const uint8_t *data, uint16_t Size)
{
    uint16_t i;

    for (i = 0; i < Size; i++)
    {
        /* 等待发送数据寄存器空，再写入下一个字节 */
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
            ;

        USART_SendData(USARTx, data[i]);
    }

    /* 等待最后 1 字节实际发送完成，避免函数提前返回 */
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
        ;
}
