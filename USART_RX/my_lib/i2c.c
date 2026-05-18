/**
  ******************************************************************************
  * @file    i2c.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2024年9月3日
  * @brief   i2c驱动源文件
  ******************************************************************************
  */
#include "i2c.h"

/* 用法说明：
 * 1. 本模块基于 STM32 硬件 I2C 外设，不负责 I2C 引脚和外设初始化。
 * 2. 使用前请先完成 I2C 时钟、GPIO、速率和 ACK 等配置。
 * 3. Addr 参数使用 8 位地址格式，最低位为读写位。
 * 4. 返回值为 0 表示成功，负值表示寻址或数据阶段失败。
 */

/* 通过硬件 I2C 向从机发送多个字节
 * 适合写寄存器、写配置和连续写数据。
 */
__weak int My_I2C_SendBytes(I2C_TypeDef *I2Cx, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == SET);
	
	I2C_GenerateSTART(I2Cx, ENABLE);
	
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_SB) == RESET);
	
	I2C_ClearFlag(I2Cx, I2C_FLAG_AF);
	
	I2C_SendData(I2Cx, Addr & 0xfe);
	
	while(1)
	{
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR) == SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
		{
			I2C_GenerateSTOP(I2Cx, ENABLE);
			return -1;
		}
	}
	
	I2C_ReadRegister(I2Cx, I2C_Register_SR1);
	I2C_ReadRegister(I2Cx, I2C_Register_SR2);
	
	for(uint16_t i=0; i<Size; i++)
	{
		while(1)
		{
			if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
			{
				I2C_GenerateSTOP(I2Cx, ENABLE);
				return -2;
			}
			if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_TXE) == SET)
			{
				break;
			}
		}
		
		I2C_SendData(I2Cx, pData[i]);
	}
	
	while(1)
	{
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
		{
				I2C_GenerateSTOP(I2Cx, ENABLE);
				return -2;
		}
		
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF) == SET)
		{
			break;
		}
	}
	
	I2C_GenerateSTOP(I2Cx, ENABLE);
	return 0;
}

/* 通过硬件 I2C 从从机接收多个字节
 * 适合读寄存器、读状态和读传感器数据。
 */
__weak int My_I2C_ReceiveBytes(I2C_TypeDef *I2Cx, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
	if(Size == 0) return 0;
	
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == SET);
	
	I2C_GenerateSTART(I2Cx, ENABLE);
	
	while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_SB) == RESET);
	
	I2C_ClearFlag(I2Cx, I2C_FLAG_AF);
	
	I2C_SendData(I2Cx, Addr | 0x01);
	
	while(1)
	{
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR) == SET)
		{
			break;
		}
		if(I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
		{
			I2C_GenerateSTOP(I2Cx, ENABLE);
			return -1;
		}
	}
	
	if(Size == 1)
	{
		I2C_AcknowledgeConfig(I2Cx, DISABLE);
		
		__disable_irq();
		I2C_ReadRegister(I2Cx, I2C_Register_SR1);
		I2C_ReadRegister(I2Cx, I2C_Register_SR2);
		
		I2C_GenerateSTOP(I2Cx, ENABLE);
		__enable_irq();
		while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET);
		pBuffer[0] = I2C_ReceiveData(I2Cx);
	}
	else
	{
		I2C_AcknowledgeConfig(I2Cx, ENABLE);
		
		I2C_ReadRegister(I2Cx, I2C_Register_SR1);
		I2C_ReadRegister(I2Cx, I2C_Register_SR2);
		
		for(uint16_t i=0; i<Size-1; i++)
		{
			if(i==Size-2)
			{
				__disable_irq();
			}
			while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET);
			pBuffer[i] = I2C_ReceiveData(I2Cx);
		}
		
		I2C_AcknowledgeConfig(I2Cx, DISABLE);
		I2C_GenerateSTOP(I2Cx, ENABLE);
		
		__enable_irq();
		
		while(I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET);
		pBuffer[Size-1] = I2C_ReceiveData(I2Cx);
	}
	
	return 0;
}
