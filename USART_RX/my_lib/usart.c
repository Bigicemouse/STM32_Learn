/**
 ******************************************************************************
 * @file    usart.c
 * @author  铁头山羊
 * @version V 1.0.0
 * @date    2024年9月1日
 * @brief   串口源文件
 ******************************************************************************
 */

#include "usart.h"
#include "delay.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* 用法说明：
 * 1. 本模块只负责轮询方式的串口收发，不负责 USART 外设初始化。
 * 2. 使用前请先完成 GPIO、波特率、工作模式和 USART 使能。
 * 3. 发送接口适合调试输出、命令发送和简单协议通信。
 * 4. 带 Timeout 的接收接口依赖 delay 模块计时。
 */

/* 发送 1 个字节
 * 适合发送命令字、状态字节或原始数据。
 */
void My_USART_SendByte(USART_TypeDef *USARTx, const uint8_t Data)
{
    My_USART_SendBytes(USARTx, &Data, 1);
}

/* 连续发送多个字节
 * 适合发送缓冲区、协议帧或数组数据。
 */
__weak void My_USART_SendBytes(USART_TypeDef *USARTx, const uint8_t *pData, uint16_t Size)
{
    if (Size == 0)
        return;

    for (uint16_t i = 0; i < Size; i++)
    {
        while (USART_GetFlagStatus(USARTx, USART_FLAG_TXE) == RESET)
            ;

        USART_SendData(USARTx, pData[i]);
    }

    while (USART_GetFlagStatus(USARTx, USART_FLAG_TC) == RESET)
        ;
}

/* 发送单个字符
 * 适合发送 ASCII 字符，如 '\r'、'\n'、'A'。
 */
void My_USART_SendChar(USART_TypeDef *USARTx, const char C)
{
    My_USART_SendBytes(USARTx, (const uint8_t *)&C, 1);
}

/* 发送字符串
 * 只发送字符串内容，不发送末尾 '\0'。
 */
void My_USART_SendString(USART_TypeDef *USARTx, const char *Str)
{
    My_USART_SendBytes(USARTx, (const uint8_t *)Str, strlen(Str));
}

/* 格式化发送
 * 用法类似 printf，适合调试日志输出。
 */
void My_USART_Printf(USART_TypeDef *USARTx, const char *Format, ...)
{
    char format_buffer[128];
    va_list argptr;

    __va_start(argptr, Format);

    vsprintf(format_buffer, Format, argptr);

    __va_end(argptr);

    My_USART_SendString(USARTx, format_buffer);
}

/* 阻塞接收 1 个字节
 * 如果一直没有数据，函数会持续等待。
 */
uint8_t My_USART_ReceiveByte(USART_TypeDef *USARTx)
{
    while (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == RESET)
        ;

    return USART_ReceiveData(USARTx);
}

/* 按指定长度接收多个字节
 * Timeout 小于 0 时表示一直等待直到收满。
 */
__weak uint16_t My_USART_ReceiveBytes(USART_TypeDef *USARTx, uint8_t *pDataOut, uint16_t Size, int Timeout)
{
    uint32_t expireTime;

    Delay_Init();

    if (Timeout >= 0)
    {
        expireTime = GetTick() + Timeout;
    }

    uint16_t i = 0;

    do
    {
        if (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == SET)
        {
            pDataOut[i++] = USART_ReceiveData(USARTx);

            if (i == Size)
                break;
        }
    } while (Timeout < 0 || GetTick() < expireTime);

    return i;
}

/*
按行接收字符串
 * 适合读取串口命令、文本行或带结束符的数据。
 */
int My_USART_ReceiveLine(USART_TypeDef *USARTx, char *pStrOut, uint16_t MaxLength, uint16_t LineSeperator, int Timeout)
{
    if (MaxLength < 2 || ((LineSeperator == LINE_SEPERATOR_CRLF) && (MaxLength < 1)))
    {
        return -2;
    }

    int ret = -1;
    uint32_t expireTime;

    Delay_Init();

    if (Timeout >= 0)
    {
        expireTime = GetTick() + Timeout;
    }

    uint16_t i = 0;

    do
    {
        if (USART_GetFlagStatus(USARTx, USART_FLAG_RXNE) == SET)
        {
            char c = (char)USART_ReceiveData(USARTx);
            pStrOut[i++] = c;

            if (LineSeperator == LINE_SEPERATOR_CR && c == '\r')
            {
                ret = 0;
                break;
            }
            else if (LineSeperator == LINE_SEPERATOR_LF && c == '\n')
            {
                ret = 0;
                break;
            }
            else if (i >= 2 && pStrOut[i - 2] == '\r' && c == '\n')
            {
                ret = 0;
                break;
            }

            if (i == MaxLength)
            {
                ret = -2;
                break;
            }
        }
    } while (Timeout < 0 || GetTick() < expireTime);

    if (i == MaxLength)
    {
        pStrOut[i - 1] = '\0';
    }
    else
    {
        pStrOut[i] = '\0';
    }

    return ret;
}
