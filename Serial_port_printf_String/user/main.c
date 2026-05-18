#include "Delay.h"
#include "stm32f10x.h"
#include <stdio.h>

void my_USART_SendBytes(USART_TypeDef *USARTx, uint8_t *pData, uint8_t size);
/* 初始化 USART1（GPIO + AFIO + 外设） */
void my_USART1_Init(void);

/* 主函数入口
 * 当前示例完成串口初始化后，发送一组测试字节，然后停留在空循环中
 */
int main(void)
{
    /* 初始化 USART1（GPIO + AFIO + 外设） */
    my_USART1_Init();

    while (1)
    {
        printf("Hello, USART1!\n");
        Delay(1000);
        printf("This is a test message.\n");
        Delay(1000);
    }
}

/* 重定向标准库 fputc()
 * printf 最终会逐字符调用本函数，因此这里只需要保证单字节发送正确即可
 * 当前实现固定输出到 USART1，适合本工程的单串口调试场景
 */
int fputc(int ch, FILE *f)
{
    /* 等待发送数据寄存器空，确保当前字符可以写入 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        ;

    /* 将 printf 传入的字符写入 USART1 数据寄存器 */
    USART_SendData(USART1, (uint8_t)ch);

    return ch; // 返回发送的字符，符合 fputc 的返回规范
}

/* 阻塞方式发送字节数组
 * 执行流程：
 * 1. 每发送一个字节之前，先等待 TXE 置位
 * 2. TXE 置位表示发送数据寄存器为空，可以继续写入下一个字节
 * 3. 所有字节写入完成后，再等待 TC 置位
 * 4. TC 置位表示最后一个字节已从移位寄存器完整发送到总线
 * 注意：
 * 1. 本函数是忙等待实现，发送期间 CPU 会一直停留在轮询循环中
 * 2. 若底层串口硬件异常或时钟配置错误，理论上可能一直卡在等待标志位的循环里
 */
void my_USART_SendBytes(USART_TypeDef *USARTx, uint8_t *pData, uint8_t size)
{
    for (uint8_t i = 0; i < size; i++)
    {
        /* 等待 TXE=1
         * TXE 表示发送数据寄存器为空，说明当前可以安全写入一个新字节
         */
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
            ;

        /* 将当前字节写入 USART 数据寄存器
         * 写入后硬件会自动开始发送，无需软件再手动触发
         */
        USART_SendData(USARTx, pData[i]);
    }

    /* 等待 TC=1
     * 这里只等待 TXE 还不够，因为 TXE 只表示数据寄存器空，不代表最后一位已经发出
     * 等到 TC 置位后再返回，才能确保整帧数据已经真正离开发送引脚
     */
    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
        ;
}

void my_USART1_Init(void)
{
    /* 1. 使能 AFIO 时钟并配置 USART1 重映射（使用 PB6/PB7） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* 2. 使能 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 3. 配置 TX 引脚 PB6（复用推挽输出） */
    GPIO_InitTypeDef gpio_initStruct = {0};
    gpio_initStruct.GPIO_Pin = GPIO_Pin_6;
    gpio_initStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initStruct.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_initStruct);

    /* 4. 配置 RX 引脚 PB7（上拉输入） */
    gpio_initStruct.GPIO_Pin = GPIO_Pin_7;
    gpio_initStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio_initStruct);

    /* 5. 使能 USART1 外设时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    /* 6. 配置 USART1 参数（115200, 8N1） */
    USART_InitTypeDef usart_initStruct = {0};
    usart_initStruct.USART_BaudRate = 115200;
    usart_initStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    usart_initStruct.USART_WordLength = USART_WordLength_8b;
    usart_initStruct.USART_StopBits = USART_StopBits_1;
    usart_initStruct.USART_Parity = USART_Parity_No;
    USART_Init(USART1, &usart_initStruct);

    /* 7. 使能 USART1 */
    USART_Cmd(USART1, ENABLE);
}
