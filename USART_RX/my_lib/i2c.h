/**
  ******************************************************************************
  * @file    i2c.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2024年9月3日
  * @brief   i2c驱动头文件
  ******************************************************************************
  */

#include "stm32f10x.h"

/* 向从机发送多个字节
 * 输入：I2Cx，Addr，pData，Size
 * 输出：0 成功，负值失败
 */
int My_I2C_SendBytes(I2C_TypeDef *I2Cx, uint8_t Addr, const uint8_t *pData, uint16_t Size);

/* 从从机接收多个字节
 * 输入：I2Cx，Addr，pBuffer，Size
 * 输出：0 成功，负值失败
 */
int My_I2C_ReceiveBytes(I2C_TypeDef *I2Cx, uint8_t Addr, uint8_t *pBuffer, uint16_t Size);
