/**
  ******************************************************************************
  * @file    si2c.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月3日
  * @brief   软i2c驱动源文件
  ******************************************************************************
  */
#include "si2c.h"

#define scl_w(v) GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
#define sda_w(v) GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, ((v)?Bit_SET:Bit_RESET))
#define scl_r ((GPIO_ReadInputDataBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin) == Bit_SET) ? 1 : 0)
#define sda_r ((GPIO_ReadInputDataBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin) == Bit_SET) ? 1 : 0)
void delay(uint32_t us) { for(uint32_t i = 0; i<8*us; i++); }

static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte);
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack);
static void SendStop(SI2C_TypeDef *SI2C);

/* 用法说明：
 * 1. 本模块使用 GPIO 模拟 I2C 时序，适合简单外设通信。
 * 2. 使用前先填写 SCL/SDA 对应端口和引脚，再调用 My_SI2C_Init()。
 * 3. Addr 参数使用 8 位地址格式，最低位为读写位。
 * 4. 返回值为 0 表示成功，负值表示总线交互失败。
 */

/* 初始化软件 I2C 引脚 */
__weak void My_SI2C_Init(SI2C_TypeDef *SI2C)
{
	if(SI2C->SCL_GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if(SI2C->SCL_GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
	
	GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, Bit_SET);
	GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, Bit_SET);
	
	if(SI2C->SDA_GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	}
	else if(SI2C->SDA_GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	}
	
	GPIO_InitTypeDef GPIO_InitStruct;
	
	GPIO_InitStruct.GPIO_Pin = SI2C->SCL_GPIO_Pin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(SI2C->SCL_GPIOx, &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin = SI2C->SDA_GPIO_Pin;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(SI2C->SDA_GPIOx, &GPIO_InitStruct);
}

/* 向从机发送多个字节
 * 常用于给寄存器型器件写配置或写显示数据。
 */
__weak int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	sda_w(0);
	delay(1);
	
	if(SendByte(SI2C, Addr & 0xfe) != 0)
	{
		SendStop(SI2C);
		return -1;
	}
	
	for(uint16_t i=0; i<Size; i++)
	{
		if(SendByte(SI2C, pData[i]) != 0)
		{
			SendStop(SI2C);
			return -2;
		}
	}
	
	SendStop(SI2C);
	
	return 0;
}

/* 从从机接收多个字节
 * 常用于读取寄存器或读取传感器数据。
 */
__weak int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
	sda_w(1);
	scl_w(1);
	
	sda_w(0);
	delay(1);
	
	if(SendByte(SI2C, Addr | 0x01) != 0)
	{
		SendStop(SI2C);
		return -1;
	}
	
	for(uint16_t i=0; i<Size; i++)
	{
		pBuffer[i] = ReceiveByte(SI2C, (i==Size-1) ? 1 : 0);
	}
	
	SendStop(SI2C);
	
	return 0;
}

/* 内部发送 1 个字节 */
static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte)
{
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0);
		sda_w((Byte & (0x01<<i)) ? 1 : 0);
		delay(2);
		
		scl_w(1);
		delay(2);
	}
	
	scl_w(0);
	sda_w(1);
	delay(2);
	
	scl_w(1);
	delay(2);
	
	return sda_r;
}

/* 内部发送停止信号 */
static void SendStop(SI2C_TypeDef *SI2C)
{
	scl_w(0);
	delay(1);
	sda_w(0);
	delay(1);
	scl_w(1);
	delay(1);
	sda_w(1);
	delay(1);
}


/* 内部接收 1 个字节 */
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack)
{
	uint8_t ret = 0;
	
	for(int8_t i=7; i>=0; i--)
	{
		scl_w(0);
		sda_w(1);
		delay(2);
		scl_w(1);
		delay(2);
		
		if(sda_r)
		{
			ret |= 0x01 << i;
		}
	}
	
	scl_w(0);
	
	if(Ack)
	{
		sda_w(0);
	}
	else
	{
		sda_w(1);
	}
	
	delay(2);
	
	return ret;
}
