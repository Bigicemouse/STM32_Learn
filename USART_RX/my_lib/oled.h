/**
  ******************************************************************************
  * @file    oled.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月4日
  * @brief   OLED显示器驱动程序
  ******************************************************************************
  */
#ifndef _OLED_H_
#define _OLED_H_

#include "stm32f10x.h"
#include "oled_font.h"

#define OLED_SLAVE_ADDR 0x78

extern const Font_TypeDef default_font;

/* 颜色定义 */
#define OLED_COLOR_TRANSPARENT 0x00 // 透明
#define OLED_COLOR_WHITE       0x01 // 白色
#define OLED_COLOR_BLACK       0x02 // 黑色

/* 画笔颜色 */
#define PEN_COLOR_TRANSPARENT OLED_COLOR_TRANSPARENT // 透明画笔
#define PEN_COLOR_WHITE       OLED_COLOR_WHITE // 白色画笔
#define PEN_COLOR_BLACK       OLED_COLOR_BLACK // 黑色画笔

/* 画刷颜色 */
#define BRUSH_TRANSPARENT OLED_COLOR_TRANSPARENT // 透明画刷
#define BRUSH_WHITE       OLED_COLOR_WHITE       // 白色画刷
#define BRUSH_BLACK       OLED_COLOR_BLACK       // 黑色画刷

/* OLED 初始化参数 */
typedef struct
{
	int (*i2c_write_cb)(uint8_t addr, const uint8_t *pdata, uint16_t size); // I2C 写回调
}OLED_InitTypeDef;

/* OLED 运行对象 */
typedef struct 
{
	int (*i2c_write_cb)(uint8_t addr, const uint8_t *pdata, uint16_t size);
	
	uint8_t *pBuffer;
	const Font_TypeDef *Font;
	uint8_t PenColor;
	uint8_t PenWidth;
	uint8_t Brush;
	int16_t CursorX;
	int16_t CursorY;
	uint16_t RefreshProgress;
	
	int16_t ClipRegionX;
	int16_t ClipRegionY;
	uint16_t ClipRegionWidth;
	uint16_t ClipRegionHeight;
	
	int16_t TextRegionX;
	int16_t TextRegionY;
	uint16_t TextRegionWidth;
	uint16_t TextRegionHeight;
	
}OLED_TypeDef;

/* 初始化 OLED
 * 输入：OLED，OLED_InitStruct
 * 输出：0 成功，负值失败
 */
int OLED_Init(OLED_TypeDef *OLED, OLED_InitTypeDef *OLED_InitStruct);

/* 清空缓冲区
 * 输入：OLED
 * 输出：无
 */
void OLED_Clear(OLED_TypeDef *OLED);

/* 获取屏幕宽度
 * 输入：OLED
 * 输出：屏幕宽度
 */
uint16_t OLED_GetScreenWidth(OLED_TypeDef *OLED);

/* 获取屏幕高度
 * 输入：OLED
 * 输出：屏幕高度
 */
uint16_t OLED_GetScreenHeight(OLED_TypeDef *OLED);

/* 整屏发送缓冲区
 * 输入：OLED
 * 输出：0 成功，负值失败
 */
int OLED_SendBuffer(OLED_TypeDef *OLED);

/* 开始分段发送缓冲区
 * 输入：OLED
 * 输出：0 成功，负值失败
 */
int OLED_StartSendBuffer(OLED_TypeDef *OLED);

/* 继续分段发送缓冲区
 * 输入：OLED，pMoreOut
 * 输出：0 成功，负值失败；pMoreOut 表示是否还有后续数据
 */
int OLED_EndSendBuffer(OLED_TypeDef *OLED, uint8_t *pMoreOut);


/* 设置光标位置
 * 输入：OLED，X，Y
 * 输出：无
 */
void OLED_SetCursor(OLED_TypeDef *OLED, int16_t X, int16_t Y);

/* 设置光标 X
 * 输入：OLED，X
 * 输出：无
 */
void OLED_SetCursorX(OLED_TypeDef *OLED, int16_t X);

/* 设置光标 Y
 * 输入：OLED，Y
 * 输出：无
 */
void OLED_SetCursorY(OLED_TypeDef *OLED, int16_t Y);

/* 移动光标
 * 输入：OLED，dX，dY
 * 输出：无
 */
void OLED_MoveCursor(OLED_TypeDef *OLED, int16_t dX, int16_t dY);

/* 横向移动光标
 * 输入：OLED，dX
 * 输出：无
 */
void OLED_MoveCursorX(OLED_TypeDef *OLED, int16_t dX);

/* 纵向移动光标
 * 输入：OLED，dY
 * 输出：无
 */
void OLED_MoveCursorY(OLED_TypeDef *OLED, int16_t dY);

/* 获取光标位置
 * 输入：OLED，pXOut，pYOut
 * 输出：通过指针返回当前光标坐标
 */
void OLED_GetCursor(OLED_TypeDef *OLED, int16_t *pXOut, int16_t *pYOut);

/* 获取光标 X
 * 输入：OLED
 * 输出：当前光标 X
 */
int16_t OLED_GetCursorX(OLED_TypeDef *OLED);

/* 获取光标 Y
 * 输入：OLED
 * 输出：当前光标 Y
 */
int16_t OLED_GetCursorY(OLED_TypeDef *OLED);


/* 设置画笔
 * 输入：OLED，Pen_Color，Width
 * 输出：无
 */
void OLED_SetPen(OLED_TypeDef *OLED, uint8_t Pen_Color, uint8_t Width);

/* 设置画刷
 * 输入：OLED，Brush_Color
 * 输出：无
 */
void OLED_SetBrush(OLED_TypeDef *OLED, uint8_t Brush_Color);


/* 画点
 * 输入：OLED
 * 输出：无
 */
void OLED_DrawDot(OLED_TypeDef *OLED);

/* 画线
 * 输入：OLED，X，Y
 * 输出：无
 */
void OLED_DrawLine(OLED_TypeDef *OLED, int16_t X, int16_t Y);

/* 画线并移动光标
 * 输入：OLED，X，Y
 * 输出：无
 */
void OLED_LineTo(OLED_TypeDef *OLED, int16_t X, int16_t Y);

/* 画圆
 * 输入：OLED，Radius
 * 输出：无
 */
void OLED_DrawCircle(OLED_TypeDef *OLED, uint16_t Radius);

/* 画矩形
 * 输入：OLED，Width，Height
 * 输出：无
 */
void OLED_DrawRect(OLED_TypeDef *OLED, uint16_t Width, uint16_t Height);

/* 画位图
 * 输入：OLED，Width，Height，pBitmap
 * 输出：无
 */
void OLED_DrawBitmap(OLED_TypeDef *OLED, uint16_t Width, uint16_t Height, const uint8_t *pBitmap);


/* 显示字符串
 * 输入：OLED，Str
 * 输出：无
 */
void OLED_DrawString(OLED_TypeDef *OLED, const char *Str);

/* 格式化显示字符串
 * 输入：OLED，Format，...
 * 输出：无
 */
void OLED_Printf(OLED_TypeDef *OLED, const char *Format, ...);

/* 开启文本区域
 * 输入：OLED，X，Y，Width，Height
 * 输出：无
 */
void OLED_StartTextRegion(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height);

/* 关闭文本区域
 * 输入：OLED
 * 输出：无
 */
void OLED_StopTextRegion(OLED_TypeDef *OLED);

/* 设置字体
 * 输入：OLED，Font
 * 输出：无
 */
void OLED_SetFont(OLED_TypeDef *OLED, const Font_TypeDef *Font);

/* 获取字符串宽度
 * 输入：OLED，Str
 * 输出：字符串像素宽度
 */
uint16_t OLED_GetStrWidth(OLED_TypeDef *OLED, const char *Str);

/* 获取字体高度
 * 输入：OLED
 * 输出：当前字体高度
 */
uint16_t OLED_GetFontHeight(OLED_TypeDef *OLED);

/* 开启剪切区域
 * 输入：OLED，X，Y，Width，Height
 * 输出：无
 */
void OLED_StartClipRegion(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height);

/* 关闭剪切区域
 * 输入：OLED
 * 输出：无
 */
void OLED_StopClipRegion(OLED_TypeDef *OLED);

#endif
