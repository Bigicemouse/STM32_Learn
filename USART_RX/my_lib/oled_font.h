/**
  ******************************************************************************
  * @file    oled_font.h
  * @author  铁头山羊stm32工作组
  * @version V1.0.0
  * @date    2023年2月24日
  * @brief   oled字体
  ******************************************************************************
*/

#ifndef _OLED_FONT_H_
#define _OLED_FONT_H_

#include <stdint.h>

typedef struct
{
	const char *Name; // 字形名称
	uint32_t Encoding; // Unicode 编码
	
	uint16_t Swx0; // 逻辑宽度
	uint16_t Swy0; // 逻辑高度
	uint16_t Dwx0; // 设备宽度
	uint16_t Dwy0; // 设备高度
	
	uint16_t Swx1; // 逻辑宽度
	uint16_t Swy1; // 逻辑高度
	uint16_t Dwx1; // 设备宽度
	uint16_t Dwy1; // 设备高度
	
	uint16_t VVectorXoff; // 原点偏移 X
	uint16_t VVectorYoff; // 原点偏移 Y
	
	uint16_t BBw; // 位图宽度
	uint16_t BBh; // 位图高度
	int16_t BBxoff0x; // 位图偏移 X
	int16_t BByoff0y; // 位图偏移 Y
	
	const uint16_t nBytes; // 位图字节数
	const uint8_t *Bitmap; // 位图数据
}Glyph_TypeDef;

/* 字体对象 */
typedef struct
{
	const char * SpecVersion; // BDF 规范版本
	const char * FontName; // 字体名称
	uint16_t ContentVersion; // 字体版本号
	uint8_t MetricsSet; // 书写方向
	uint16_t FontSize; // 字号（磅）
	uint16_t Xres; // 横向分辨率(DPI)
	uint16_t Yres; // 纵向分辨率(DPI)
	uint16_t FBBx; // 最大边框宽度
	uint16_t FBBy; // 最大边框高度
	int16_t FBBXoff; // 最大边框横坐标
	int16_t FBBYoff; // 最大边框纵坐标
	uint16_t nChars; // 包含的字形数量
	const uint32_t *Map; // Unicode 到字形映射表
	const Glyph_TypeDef *Glyphs; // 字形数据
}Font_TypeDef;



#endif
