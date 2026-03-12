/**
  ******************************************************************************
  * @file    usart.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2024年9月1日
  * @brief   串口通用驱动库头文件
  *          提供USART初始化、发送、接收等功能的接口
  ******************************************************************************
  */
	
#ifndef _USART_H_
#define _USART_H_

#include "stm32f10x.h"

/* ==================== 换行符定义 ==================== */
#define LINE_SEPERATOR_CR   0x00 /* 回车符 \r (Carriage Return) */
#define LINE_SEPERATOR_LF   0x01 /* 换行符 \n (Line Feed) */
#define LINE_SEPERATOR_CRLF 0x02 /* 回车换行 \r\n (CR+LF) */

/* ==================== USART发送函数声明 ==================== */

/**
 * @brief  通过USART发送单个字节
 * @param  USARTx - 指定要使用的USART外设（USART1/USART2/USART3）
 * @param  Data   - 待发送的字节数据
 * @return 无
 */
void My_USART_SendByte(USART_TypeDef *USARTx, const uint8_t Data);

/**
 * @brief  通过USART发送多个字节
 * @param  USARTx  - 指定要使用的USART外设
 * @param  pData   - 指向数据缓冲区的指针
 * @param  Size    - 待发送数据的字节数
 * @return 无
 */
void My_USART_SendBytes(USART_TypeDef *USARTx, const uint8_t *pData, uint16_t Size);

/**
 * @brief  通过USART发送单个字符
 * @param  USARTx - 指定要使用的USART外设
 * @param  C      - 待发送的字符
 * @return 无
 */
void My_USART_SendChar(USART_TypeDef *USARTx, const char C);

/**
 * @brief  通过USART发送字符串
 * @param  USARTx - 指定要使用的USART外设
 * @param  Str    - 指向字符串的指针（以 '\0' 结尾）
 * @return 无
 */
void My_USART_SendString(USART_TypeDef *USARTx, const char *Str);

/**
 * @brief  通过USART发送格式化字符串（类似printf）
 * @param  USARTx - 指定要使用的USART外设
 * @param  Format - 格式字符串（支持 %d, %s, %x, %f 等）
 * @param  ...    - 可变参数列表
 * @return 无
 * @note   使用示例: My_USART_Printf(USART1, "Temperature: %d\r\n", temp_value);
 */
void My_USART_Printf(USART_TypeDef *USARTx, const char *Format, ...);

/* ==================== USART接收函数声明 ==================== */

/**
 * @brief  通过USART接收单个字节（阻塞式）
 * @param  USARTx - 指定要使用的USART外设
 * @return 接收到的字节数据
 * @note   此函数会一直等待直到接收到数据
 */
uint8_t My_USART_ReceiveByte(USART_TypeDef *USARTx);

/**
 * @brief  通过USART接收多个字节
 * @param  USARTx    - 指定要使用的USART外设
 * @param  pDataOut  - 指向接收缓冲区的指针
 * @param  Size      - 期望接收的最大字节数
 * @param  Timeout   - 超时时间（毫秒），-1表示无超时
 * @return 实际接收到的字节数（0表示超时未收到）
 */
uint16_t My_USART_ReceiveBytes(USART_TypeDef *USARTx, uint8_t *pDataOut, uint16_t Size, int Timeout);

/**
 * @brief  通过USART接收一行数据（直到指定的换行符）
 * @param  USARTx        - 指定要使用的USART外设
 * @param  pStrOut       - 指向接收缓冲区的指针（用于存储字符串）
 * @param  MaxLength     - 接收缓冲区的最大长度
 * @param  LineSeperator - 换行符类型（CR/LF/CRLF）
 * @param  Timeout       - 超时时间（毫秒），-1表示无超时
 * @return 接收到的字符数（不包括换行符），-1表示超时或缓冲区满
 * @note   接收的字符串自动以 '\0' 结尾
 */
int My_USART_ReceiveLine(USART_TypeDef *USARTx, char *pStrOut, uint16_t MaxLength, uint16_t LineSeperator, int Timeout);

#endif

