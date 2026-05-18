/**
  ******************************************************************************
  * @file    si2c.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月3日
  * @brief   软i2c驱动头文件
  ******************************************************************************
  */

#include "stm32f10x.h"

/* 软件 I2C 引脚定义 */
typedef struct
{
	GPIO_TypeDef *SCL_GPIOx; // SCL 端口
	uint16_t SCL_GPIO_Pin;   // SCL 引脚
	
	GPIO_TypeDef *SDA_GPIOx; // SDA 端口
	uint16_t SDA_GPIO_Pin;   // SDA 引脚
	
} SI2C_TypeDef;

/* 初始化软件 I2C
 * 输入：SI2C
 * 输出：无
 */
void My_SI2C_Init(SI2C_TypeDef *SI2C);

/* 向从机发送多个字节
 * 输入：SI2C，Addr，pData，Size
 * 输出：0 成功，负值失败
 */
int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size);

/* 从从机接收多个字节
 * 输入：SI2C，Addr，pBuffer，Size
 * 输出：0 成功，负值失败
 */
int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size);
