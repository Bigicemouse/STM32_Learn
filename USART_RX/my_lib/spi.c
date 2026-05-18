/**
  ******************************************************************************
  * @file    spi.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2024年9月12日
  * @brief   spi驱动源文件
  ******************************************************************************
  */
	
#include "spi.h"

/* 用法说明：
 * 1. 本模块只封装一次完整的 SPI 主机收发流程，不负责 SPI 外设初始化。
 * 2. 使用前请先完成 SPI 引脚、时钟、主机模式、时序参数和 NSS 管理配置。
 * 3. pDataTx 和 pDataRx 都应提供有效缓冲区，长度至少为 Size。
 */

/* SPI 主机模式下收发数据
 * 发送多少字节，就同步接收多少字节。
 */
void My_SPI_MasterTransmitReceive(SPI_TypeDef *SPIx, const uint8_t *pDataTx, uint8_t *pDataRx, uint16_t Size)
{
	if(Size == 0) return;
	
	SPI_Cmd(SPIx, ENABLE);
	
	SPI_I2S_SendData(SPIx, pDataTx[0]);
	
	for(uint16_t i=0; i<Size-1; i++)
	{
		while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET);
		
		SPI_I2S_SendData(SPIx, pDataTx[i+1]);
		
		while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
		
		pDataRx[i] = SPI_I2S_ReceiveData(SPIx);
	}
	
	while(SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET);
	
	pDataRx[Size-1] = SPI_I2S_ReceiveData(SPIx);
	
	SPI_Cmd(SPIx, DISABLE);
}
