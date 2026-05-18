/**
  ******************************************************************************
  * @file    usart.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2024年9月1日
  * @brief   串口头文件
  ******************************************************************************
  */
	
#ifndef _USART_H_
#define _USART_H_

#include "stm32f10x.h"

/* 行结束符类型 */
#define LINE_SEPERATOR_CR   0x00 // 回车 \r
#define LINE_SEPERATOR_LF   0x01 // 换行 \n
#define LINE_SEPERATOR_CRLF 0x02 // 回车+换行 \r\n

/* 发送 1 个字节
 * 输入：USARTx，Data
 * 输出：无
 */
void My_USART_SendByte(USART_TypeDef *USARTx, const uint8_t Data);

/* 发送多个字节
 * 输入：USARTx，pData，Size
 * 输出：无
 */
void My_USART_SendBytes(USART_TypeDef *USARTx, const uint8_t *pData, uint16_t Size);

/* 发送 1 个字符
 * 输入：USARTx，C
 * 输出：无
 */
void My_USART_SendChar(USART_TypeDef *USARTx, const char C);

/* 发送字符串
 * 输入：USARTx，Str
 * 输出：无
 */
void My_USART_SendString(USART_TypeDef *USARTx, const char *Str);

/* 格式化发送字符串
 * 输入：USARTx，Format，...
 * 输出：无
 */
void My_USART_Printf(USART_TypeDef *USARTx, const char *Format, ...);

/* 接收 1 个字节
 * 输入：USARTx
 * 输出：接收到的 1 个字节
 */
uint8_t My_USART_ReceiveByte(USART_TypeDef *USARTx);

/* 接收多个字节
 * 输入：USARTx，pDataOut，Size，Timeout
 * 输出：实际接收到的字节数
 */
uint16_t My_USART_ReceiveBytes(USART_TypeDef *USARTx, uint8_t *pDataOut, uint16_t Size, int Timeout);

/* 按行接收字符串
 * 输入：USARTx，pStrOut，MaxLength，LineSeperator，Timeout
 * 输出：0 成功，-1 超时，-2 缓冲区不足
 */
int My_USART_ReceiveLine(USART_TypeDef *USARTx, char *pStrOut, uint16_t MaxLength, uint16_t LineSeperator, int Timeout);

#endif

