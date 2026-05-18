/**
  ******************************************************************************
  * @file    oled.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月4日
  * @brief   OLED显示器驱动程序
  ******************************************************************************
  */
	
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "oled.h"
#include "oled_default_font.h"
#include <math.h>

/* 用法说明：
 * 1. 使用前先准备好底层 I2C 写回调，并在 OLED_Init() 中传入。
 * 2. 本模块采用“先写缓冲区，再刷新屏幕”的方式工作。
 * 3. 绘图、文字、位图接口默认都作用在内部缓冲区上，调用 OLED_SendBuffer() 后才会真正显示。
 * 4. 若需要分段刷新，可先调用 OLED_StartSendBuffer()，再循环调用 OLED_EndSendBuffer()。
 * 5. OLED_Init() 成功后默认使用 default_font、白色画笔、透明画刷。
 */

#define OLED_SCREEN_COLS 128 
#define OLED_SCREEN_ROWS  64  
#define OLED_SCREEN_PAGES 8 

#define SSD1306_CTRL_COMMAND           0x80
#define SSD1306_CTRL_COMMAND_STREAM    0x00
#define SSD1306_CTRL_DATA              0xc0
#define SSD1306_CTRL_DATA_STREAM       0x40

typedef struct
{
	int16_t X;
	int16_t Y;
	uint16_t Width;
	uint16_t Height;
}Rect;

static int OLED_SendCommand(OLED_TypeDef *OLED, const uint8_t Cmd, const uint8_t *Arg, uint16_t Size);
static int OLED_SendData(OLED_TypeDef *OLED, uint8_t *pData, uint16_t Size);
static void DrawCircleFrame(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Radius);
static void FillCircle(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Radius);
static void DrawRectFrame(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height);
static void FillRect(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height);
static int16_t unicode_2_glyph_idx(OLED_TypeDef *OLED, uint32_t Unicode);
static void DrawCharator(OLED_TypeDef *OLED, uint32_t Unicode);
static void BrushDot(OLED_TypeDef *OLED, int16_t x, int16_t y);
static void PenDot(OLED_TypeDef *OLED, int16_t x, int16_t y);
static void DrawBitmapEx(OLED_TypeDef *OLED,int16_t X, int16_t Y, uint16_t Width, uint16_t Height, const uint8_t *pBitmap);
static uint16_t GetGlyphWidth(OLED_TypeDef *OLED, uint32_t Unicode);

/* OLED 初始化入口
 * 成功后即可调用绘图和文字接口。
 */
int OLED_Init(OLED_TypeDef *OLED, OLED_InitTypeDef *OLED_InitStruct)
{
	OLED->i2c_write_cb = OLED_InitStruct->i2c_write_cb;
	
	OLED->pBuffer = (uint8_t *)malloc(OLED_SCREEN_COLS * OLED_SCREEN_PAGES * sizeof(uint8_t) + 1);
	
	if(OLED->pBuffer == 0)
	{
		return -2;
	}
	
	OLED->pBuffer++;
	
	
	memset(OLED->pBuffer, 0, OLED_SCREEN_COLS * OLED_SCREEN_PAGES * sizeof(uint8_t));
	
	uint8_t arg;
	
	if(OLED_SendCommand(OLED, 0xae, 0, 0)) return -1;
	arg = 0x80;
	if(OLED_SendCommand(OLED, 0xd5, &arg, 1)) return -1;
	arg = 0x3f;
	if(OLED_SendCommand(OLED, 0xa8, &arg, 1)) return -1;
	arg = 0x00;
	if(OLED_SendCommand(OLED, 0xd3, &arg, 1)) return -1;
	
	if(OLED_SendCommand(OLED, 0x40, 0, 0)) return -1;
	arg = 0x14;
	if(OLED_SendCommand(OLED, 0x8d, &arg, 1)) return -1;
	arg = 0x00;
	if(OLED_SendCommand(OLED, 0x20, &arg, 1)) return -1;
	
	if(OLED_SendCommand(OLED, 0xa1, 0, 0)) return -1;
	if(OLED_SendCommand(OLED, 0xc8, 0, 0)) return -1;
	
	arg = 0x12;
	if(OLED_SendCommand(OLED, 0xda, &arg, 1)) return -1;
	arg = 0xcf;
	if(OLED_SendCommand(OLED, 0x81, &arg, 1)) return -1;
	arg = 0xf1;
	if(OLED_SendCommand(OLED, 0xd9, &arg, 1)) return -1;
	arg = 0x20;
	if(OLED_SendCommand(OLED, 0xdb, &arg, 1)) return -1;
	
	if(OLED_SendCommand(OLED, 0x2e, 0, 0)) return -1;
	if(OLED_SendCommand(OLED, 0xa4, 0, 0)) return -1;
	if(OLED_SendCommand(OLED, 0xa6, 0, 0)) return -1;
	
	if(OLED_SendCommand(OLED, 0xaf, 0, 0)) return -1;
	
	OLED->PenWidth = 1;
	OLED->PenColor = PEN_COLOR_WHITE;
	OLED->Brush = BRUSH_TRANSPARENT;
	
	OLED->CursorX = 0;
	OLED->CursorY = 0;
	
	OLED->RefreshProgress = 0;
	
	OLED->TextRegionX = 0;
	OLED->TextRegionY = 0;
	OLED->TextRegionWidth = 0;
	OLED->TextRegionHeight = 0;
	
	OLED->Font = &default_font;
	
	return 0;
}

/* 开启剪切区域
 * 后续绘图只会作用在该区域内。
 */
void OLED_StartClipRegion(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height)
{
	OLED->ClipRegionX = X;
	OLED->ClipRegionY = Y;
	OLED->ClipRegionWidth = Width;
	OLED->ClipRegionHeight = Height;
}

/* 关闭剪切区域 */
void OLED_StopClipRegion(OLED_TypeDef *OLED)
{
	OLED->ClipRegionX = 0;
	OLED->ClipRegionY = 0;
	OLED->ClipRegionWidth = 0;
	OLED->ClipRegionHeight = 0;
}

/* 获取屏幕宽度 */
uint16_t OLED_GetScreenWidth(OLED_TypeDef *OLED)
{
	return OLED_SCREEN_COLS;
}

/* 获取屏幕高度 */
uint16_t OLED_GetScreenHeight(OLED_TypeDef *OLED)
{
	return OLED_SCREEN_ROWS;
}

/* 清空显示缓冲区 */
void OLED_Clear(OLED_TypeDef *OLED)
{
	memset(OLED->pBuffer, 0, OLED_SCREEN_COLS * OLED_SCREEN_PAGES);
}

/* 设置当前字体
 * 若想恢复默认字体，可传入 &default_font。
 */
void OLED_SetFont(OLED_TypeDef *OLED, const Font_TypeDef *Font)
{
	OLED->Font = Font;
}

/* 设置画笔
 * 用于后续点、线、圆、矩形边框等绘制操作。
 */
void OLED_SetPen(OLED_TypeDef *OLED, uint8_t Pen_Color, uint8_t Width)
{
	OLED->PenColor = Pen_Color;
	OLED->PenWidth = Width;
}

/* 设置画刷
 * 用于后续填充区域和文字背景绘制。
 */
void OLED_SetBrush(OLED_TypeDef *OLED, uint8_t Brush_Color)
{
	OLED->Brush = Brush_Color;
}

/* 设置光标位置 */
void OLED_SetCursor(OLED_TypeDef *OLED, int16_t X, int16_t Y)
{
	OLED->CursorX = X;
	OLED->CursorY = Y;
}

/* 设置光标 X */
void OLED_SetCursorX(OLED_TypeDef *OLED, int16_t X)
{
	OLED->CursorX = X;
}

/* 设置光标 Y */
void OLED_SetCursorY(OLED_TypeDef *OLED, int16_t Y)
{
	OLED->CursorY = Y;
}

/* 同时移动光标 X/Y */
void OLED_MoveCursor(OLED_TypeDef *OLED, int16_t dX, int16_t dY)
{
	OLED->CursorX += dX;
	OLED->CursorY += dY;
}

/* 横向移动光标 */
void OLED_MoveCursorX(OLED_TypeDef *OLED, int16_t dX)
{
	OLED->CursorX += dX;
}

/* 纵向移动光标 */
void OLED_MoveCursorY(OLED_TypeDef *OLED, int16_t dY)
{
	OLED->CursorY += dY;
}

/* 获取当前光标位置 */
void OLED_GetCursor(OLED_TypeDef *OLED, int16_t *pXOut, int16_t *pYOut)
{
	*pXOut = OLED->CursorX;
	*pYOut = OLED->CursorY;
}

/* 获取当前光标 X */
int16_t OLED_GetCursorX(OLED_TypeDef *OLED)
{
	return OLED->CursorX;
}

/* 获取当前光标 Y */
int16_t OLED_GetCursorY(OLED_TypeDef *OLED)
{
	return OLED->CursorY;
}

#define min(x1,x2) x1>x2?x2:x1
#define max(x1,x2) x1>x2?x1:x2

static Rect GetOverlappedRect(Rect rect1, Rect rect2)
{
	Rect ret = {0,0,0,0};
	
	int16_t xl = max(rect1.X,rect2.X);
	int16_t xr = min(rect1.X + rect1.Width, rect2.X + rect2.Width);
	
	int16_t yt = max(rect1.Y,rect2.Y);
	int16_t yb = min(rect1.Y + rect1.Height, rect2.Y + rect2.Height);
	
	if(xl<xr&&yt<yb)
	{
		ret.X = xl;
		ret.Y = yt;
		ret.Width = xr - xl;
		ret.Height = yb - yt;
	}
	
	return ret;
}

static uint16_t GetGlyphWidth(OLED_TypeDef *OLED, uint32_t Unicode)
{
	if(OLED->Font == NULL) return 0;
	
	int16_t idx = unicode_2_glyph_idx(OLED, Unicode);
	
	if(idx < 0) return 0;
	
	return OLED->Font->Glyphs[idx].Dwx0;
}

static void DrawCharator(OLED_TypeDef *OLED, uint32_t Unicode)
{
	if(OLED->Font == NULL) return;
	
	int16_t idx = unicode_2_glyph_idx(OLED, Unicode);
	
	const Glyph_TypeDef *pGlyph = 0;
	
	if(idx >=0)
	{
		pGlyph = &OLED->Font->Glyphs[idx];
	}
	
	int16_t clipRegionXCpy, clipRegionYCpy, clipRegionWidthCpy, clipRegionHeightCpy;
	
	if(OLED->TextRegionWidth!=0 && OLED->TextRegionHeight!=0)
	{
		clipRegionXCpy = OLED->ClipRegionX;
		clipRegionYCpy = OLED->ClipRegionY;
		clipRegionWidthCpy = OLED->ClipRegionWidth;
		clipRegionHeightCpy = OLED->ClipRegionHeight;
		
		if(OLED->ClipRegionWidth != 0 && OLED->ClipRegionHeight != 0)
		{
			Rect rect1 = {OLED->TextRegionX, OLED->TextRegionY, OLED->TextRegionWidth, OLED->TextRegionHeight};
			Rect rect2 = {OLED->ClipRegionX, OLED->ClipRegionY, OLED->ClipRegionWidth, OLED->ClipRegionHeight};
			Rect overlapped = GetOverlappedRect(rect1, rect2);
			
			OLED->ClipRegionX = overlapped.X;
			OLED->ClipRegionY = overlapped.Y;
			OLED->ClipRegionWidth = overlapped.Width;
			OLED->ClipRegionHeight = overlapped.Height;
		}
		else
		{
			OLED->ClipRegionX = OLED->TextRegionX;
			OLED->ClipRegionY = OLED->TextRegionY;
			OLED->ClipRegionWidth = OLED->TextRegionWidth;
			OLED->ClipRegionHeight = OLED->TextRegionHeight;
		}
		
		if(OLED->CursorX<OLED->TextRegionX || OLED->CursorX>=OLED->TextRegionX+OLED->TextRegionWidth)
		{
			OLED->CursorX = OLED->TextRegionX;
		}
		
		if(pGlyph != 0)
		{
			if(OLED->CursorX+pGlyph->Dwx0 >= OLED->TextRegionX+OLED->TextRegionWidth)
			{
				OLED->CursorX = OLED->TextRegionX;
				OLED->CursorY = OLED->CursorY + OLED->Font->FBBy + OLED->Font->FBBYoff;
			}
		}
		
		if(Unicode == '\r')
		{
			OLED->CursorX = OLED->TextRegionX;
		}
		else if(Unicode == '\n')
		{
			OLED->CursorY = OLED->CursorY + OLED->Font->FBBy + OLED->Font->FBBYoff;
		}
	}
	
	if(pGlyph != 0)
	{
		FillRect(OLED, OLED->CursorX, OLED->CursorY - OLED->Font->FBBYoff - OLED->Font->FBBy, pGlyph->Dwx0, OLED->Font->FBBy);
		
		DrawBitmapEx(OLED, OLED->CursorX + pGlyph->BBxoff0x, OLED->CursorY - pGlyph->BByoff0y - pGlyph->BBh, pGlyph->BBw, pGlyph->BBh, pGlyph->Bitmap);
	}
	
	if(OLED->TextRegionWidth!=0 && OLED->TextRegionHeight!=0)
	{
		OLED->ClipRegionX = clipRegionXCpy;
		OLED->ClipRegionY = clipRegionYCpy;
		OLED->ClipRegionWidth = clipRegionWidthCpy;
		OLED->ClipRegionHeight = clipRegionHeightCpy;
	}
	if(pGlyph != 0){
		OLED->CursorX += pGlyph->Dwx0;
	}
}

/* 从当前光标位置开始绘制字符串
 * 输入字符串按 UTF-8 编码解析。
 */
void OLED_DrawString(OLED_TypeDef *OLED, const char *Str)
{
	uint16_t i;
	uint32_t unicode;
	uint8_t first, second, third, forth;
	
	i=0;
	for(;;)
	{
		first = Str[i++];
		if(first == '\0') break; 
		
		if ((first & (1 << 7)) == 0)
		{
			unicode = first;
			DrawCharator(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5)) == (1 << 7 | 1 << 6))
		{
			first = first & 0x1f;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			unicode = ((uint32_t)first << 6) | second;
			DrawCharator(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4)) == (1 << 7 | 1 << 6 | 1 << 5))
		{
			first = first & 0x0f;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			third = Str[i++];
			if(third == '\0' || ((third & (1 << 7 | 1 << 6)) != 0x80)) break;
			third = third & 0x3f;
			
			unicode = ((uint32_t)first << 12) | ((uint32_t)second << 6) | third;
			DrawCharator(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3)) == (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4))
		{
			first = first & 0x07;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			third = Str[i++];
			if(third == '\0' || ((third & (1 << 7 | 1 << 6)) != 0x80)) break;
			third = third & 0x3f;
			
			forth = Str[i++];
			if(forth == '\0' || ((forth & (1 << 7 | 1 << 6)) != 0x80)) break;
			forth = forth & 0x3f;
			
			unicode = ((uint32_t)first << 18) | ((uint32_t)second << 12) | ((uint32_t)second << 6) | forth;
			
			DrawCharator(OLED, unicode);
		}
	}
}


/* 开启文本区域
 * 开启后光标会移动到区域起点，适合做文本排版。
 */
void OLED_StartTextRegion(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height)
{
	OLED->TextRegionX = X;
	OLED->TextRegionY = Y;
	OLED->TextRegionWidth = Width;
	OLED->TextRegionHeight = Height;
	
	OLED->CursorX = X;
	OLED->CursorY = Y + OLED_GetFontHeight(OLED);
}

/* 关闭文本区域 */
void OLED_StopTextRegion(OLED_TypeDef *OLED)
{
	OLED->TextRegionX = 0;
	OLED->TextRegionY = 0;
	OLED->TextRegionWidth = 0;
	OLED->TextRegionHeight = 0;
}

/* 格式化绘制字符串
 * 适合显示数值、状态和调试信息。
 */
void OLED_Printf(OLED_TypeDef *OLED, const char *Format, ...)
{
	char format_buffer[64];
	
	va_list argptr;
	__va_start(argptr, Format);
	vsprintf(format_buffer, Format, argptr);
	__va_end(argptr);
	OLED_DrawString(OLED, format_buffer);
}

/* 获取当前字体下字符串所占像素宽度 */
uint16_t OLED_GetStrWidth(OLED_TypeDef *OLED, const char *Str)
{
	uint16_t i;
	uint32_t unicode;
	uint8_t first, second, third, forth;
	uint16_t ret = 0;
	
	i=0;
	for(;;)
	{
		first = Str[i++];
		if(first == '\0') break; 
		
		if ((first & (1 << 7)) == 0)
		{
			unicode = first;
			ret += GetGlyphWidth(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5)) == (1 << 7 | 1 << 6))
		{
			first = first & 0x1f;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			unicode = ((uint32_t)first << 6) | second;
			ret += GetGlyphWidth(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4)) == (1 << 7 | 1 << 6 | 1 << 5))
		{
			first = first & 0x0f;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			third = Str[i++];
			if(third == '\0' || ((third & (1 << 7 | 1 << 6)) != 0x80)) break;
			third = third & 0x3f;
			
			unicode = ((uint32_t)first << 12) | ((uint32_t)second << 6) | third;
			ret += GetGlyphWidth(OLED, unicode);
		}
		else if ((first & (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4 | 1 << 3)) == (1 << 7 | 1 << 6 | 1 << 5 | 1 << 4))
		{
			first = first & 0x07;
			
			second = Str[i++];
			if(second == '\0' || ((second & (1 << 7 | 1 << 6)) != 0x80)) break;
			second = second & 0x3f;
			
			third = Str[i++];
			if(third == '\0' || ((third & (1 << 7 | 1 << 6)) != 0x80)) break;
			third = third & 0x3f;
			
			forth = Str[i++];
			if(forth == '\0' || ((forth & (1 << 7 | 1 << 6)) != 0x80)) break;
			forth = forth & 0x3f;
			
			unicode = ((uint32_t)first << 18) | ((uint32_t)second << 12) | ((uint32_t)second << 6) | forth;
			
			ret += GetGlyphWidth(OLED, unicode);
		}
	}
	
	return ret;
}

/* 获取当前字体高度 */
uint16_t OLED_GetFontHeight(OLED_TypeDef *OLED)
{
	return OLED->Font->FontSize;
}

#define swap(x,y) do{(x) = (x) + (y); (y) = (x) - (y); (x) = (x) - (y); }while(0)

/* 在当前光标处画点 */
void OLED_DrawDot(OLED_TypeDef *OLED)
{
	PenDot(OLED, OLED->CursorX, OLED->CursorY);
}	

/* 从当前光标位置画线到目标点 */
void OLED_DrawLine(OLED_TypeDef *OLED, int16_t X, int16_t Y)
{
	int16_t x, y;
	int16_t X0 = OLED->CursorX;
	int16_t Y0 = OLED->CursorY;
	int16_t X1 = X;
	int16_t Y1 = Y;
	
	if(OLED->PenColor == PEN_COLOR_TRANSPARENT) return;
	
	if(X0 != X1)
	{
		if(X0 > X1)
		{
			swap(X0, X1);
			swap(Y0, Y1);
		}
		for(x=X0; x < X1; x++)
		{
			if(x < 0 || x >= OLED_SCREEN_COLS) continue;
			y = (int16_t)round(1.0 * (Y1 - Y0) * (x - X0) / (X1 - X0) + Y0);
			if(y < 0 || y >= OLED_SCREEN_ROWS) continue;
			
			PenDot(OLED, x, y);
		}
	}
	if(Y0 != Y1)
	{
		if(Y0 > Y1)
		{
			swap(X0, X1);
			swap(Y0, Y1);
		}
		for(y=Y0; y < Y1; y++)
		{
			if(y < 0 || y >= OLED_SCREEN_ROWS) continue;
			x = (int16_t)round(1.0 * (y - Y0) * (X1 - X0) / (Y1 - Y0) + X0);
			if(x < 0 || x >= OLED_SCREEN_COLS) continue;
			
			PenDot(OLED, x, y);
		}
	}
}

/* 画线并把光标移动到终点 */
void OLED_LineTo(OLED_TypeDef *OLED, int16_t X, int16_t Y)
{
	OLED_DrawLine(OLED, X, Y);
	OLED->CursorX = X;
	OLED->CursorY = Y;
}

static void DrawCircleFrame(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Radius)
{
	int16_t x, y, distance;
	
	if(OLED->PenColor == PEN_COLOR_TRANSPARENT) return;

	if(X - Radius >= OLED_SCREEN_COLS || X + Radius < 0) return;
	if(Y - Radius >= OLED_SCREEN_ROWS || Y + Radius < 0) return;

	for(x=X-Radius; x<=X+Radius; x++)
	{
		if(x < 0 || x > OLED_SCREEN_COLS) continue;
		for(distance = 0; distance <= Radius; distance++)
		{
			if((x - X) * (x - X) + (distance + 1) * (distance + 1) > Radius * Radius)
			{
				if(Y+distance < OLED_SCREEN_ROWS && x >= 0 && x < OLED_SCREEN_COLS)
				{
					PenDot(OLED, x, Y+distance);
				}
				if(Y-distance > 0 && x >= 0 && x < OLED_SCREEN_COLS)
				{
					PenDot(OLED, x, Y-distance);
				}
				break;
			}
		}
	}

	for(y=Y-Radius; y<=Y+Radius; y++)
	{
		if(y < 0 || y > OLED_SCREEN_ROWS) continue;
		for(distance = 0; distance <= Radius; distance++)
		{
			if((y - Y) * (y - Y) + (distance + 1) * (distance + 1) > Radius * Radius)
			{
				if(X+distance < OLED_SCREEN_COLS && y >=0 && y < OLED_SCREEN_ROWS)
				{
					PenDot(OLED, X+distance, y);
				}
				if(X-distance > 0 && y >=0 && y < OLED_SCREEN_ROWS)
				{
					PenDot(OLED, X-distance, y);
				}
				break;
			}
		}
	}
}

static void FillCircle(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Radius)
{
	int16_t x, distance;
	
	if(OLED->Brush == BRUSH_TRANSPARENT) return;

	if(X - Radius >= OLED_SCREEN_COLS || X + Radius < 0) return;
	if(Y - Radius >= OLED_SCREEN_ROWS || Y + Radius < 0) return;

	for(x=X-Radius; x<=X+Radius; x++)
	{
		if(x < 0 || x > OLED_SCREEN_COLS) continue;
		for(distance = 0; distance <= Radius; distance++)
		{
			if((x - X) * (x - X) + distance*distance <= Radius * Radius)
			{
				if(Y+distance < OLED_SCREEN_ROWS)
				{
					BrushDot(OLED, x, Y+distance);
				}
				if(Y-distance > 0)
				{
					BrushDot(OLED, x, Y-distance);
				}
			}
			else
			{
				break;
			}
		}
	}
}

/* 以当前光标为圆心画圆 */
void OLED_DrawCircle(OLED_TypeDef *OLED, uint16_t Radius)
{
	int16_t X = OLED->CursorX, Y = OLED->CursorY;
	
	if(OLED->PenColor != PEN_COLOR_TRANSPARENT)
	{
		DrawCircleFrame(OLED, X, Y, Radius);
	}
	
	if(OLED->Brush != BRUSH_TRANSPARENT)
	{
		FillCircle(OLED, X, Y, Radius);
	}
}

static void DrawRectFrame(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height)
{
	int16_t x,y;
	
	if(OLED->PenColor == PEN_COLOR_TRANSPARENT) return;
	
	x = X;
	if(x>=0 && x<OLED_SCREEN_COLS)
	{
		for(y=max(0,Y);y<Y+Height;y++)
		{
			PenDot(OLED, x, y);
		}
	}
	
	x = X + Width -1;
	if(Width > 0 && x>=0 && x<OLED_SCREEN_COLS)
	{
		for(y=max(0,Y);y<Y+Height;y++)
		{
			PenDot(OLED, x, y);
		}
	}
	
	y = Y;
	if(y>=0 && y<OLED_SCREEN_ROWS)
	{
		for(x=max(0,X);x<X+Width;x++)
		{
			PenDot(OLED, x, y);
		}
	}
	
	y = Y + Height - 1;
	if(y>=0 && y<OLED_SCREEN_ROWS)
	{
		for(x=max(0,X);x<X+Width;x++)
		{
			PenDot(OLED, x, y);
		}
	}
}

static void FillRect(OLED_TypeDef *OLED, int16_t X, int16_t Y, uint16_t Width, uint16_t Height)
{
	if(OLED->Brush == BRUSH_TRANSPARENT) return;
	
	int16_t x,y;
	
	for(x=X;x<X+Width;x++)
	{
		for(y=Y;y<Y+Height;y++)
		{
			BrushDot(OLED, x, y);
		}
	}
}

/* 以当前光标为左上角画矩形 */
void OLED_DrawRect(OLED_TypeDef *OLED, uint16_t Width, uint16_t Height)
{
	if(OLED->PenColor != PEN_COLOR_TRANSPARENT)
	{
		DrawRectFrame(OLED, OLED->CursorX, OLED->CursorY, Width, Height);
	}
	if(OLED->Brush != BRUSH_TRANSPARENT)
	{
		FillRect(OLED, OLED->CursorX, OLED->CursorY, Width, Height);
	}
}

/* 开始分段刷新
 * 适合降低单次刷新阻塞时间。
 */
int OLED_StartSendBuffer(OLED_TypeDef *OLED)
{
	uint8_t arg[2];
	
	OLED->RefreshProgress = 0;
	
	arg[0] = 0x00;
	if(OLED_SendCommand(OLED, 0x20, arg, 1) != 0) return -1;

	arg[0] = 0x00;
	arg[1] = 0x7f;
	if(OLED_SendCommand(OLED, 0x21, arg, 2) != 0) return -1;

	arg[0] = 0x00;
	arg[1] = 0x07;
	if(OLED_SendCommand(OLED, 0x22, arg, 2) != 0) return -1;
	
	return 0;
}

/* 继续分段刷新
 * pMoreOut 为 0 表示已经刷新完成。
 */
int OLED_EndSendBuffer(OLED_TypeDef *OLED, uint8_t *pMoreOut)
{
	if(OLED->RefreshProgress >= 127)
	{
		*pMoreOut = 0;
	}
	else
	{
		*pMoreOut = 1;
	}
	
	if(OLED->RefreshProgress >= 128)
	{
		return -1;
	}
	
	OLED_SendData(OLED, &OLED->pBuffer[OLED->RefreshProgress * 8], 8);
	
	OLED->RefreshProgress = (OLED->RefreshProgress + 1) % 128;
	
	return 0;
}

/* 一次性整屏刷新 */
int OLED_SendBuffer(OLED_TypeDef *OLED)
{
	uint8_t arg[2];

	arg[0] = 0x00;
	if(OLED_SendCommand(OLED, 0x20, arg, 1) != 0) return -1;

	arg[0] = 0x00;
	arg[1] = 0x7f;
	if(OLED_SendCommand(OLED, 0x21, arg, 2)) return -1;

	arg[0] = 0x00;
	arg[1] = 0x07;
	if(OLED_SendCommand(OLED, 0x22, arg, 2)) return -1;
	
	if(OLED_SendData(OLED, OLED->pBuffer, OLED_SCREEN_COLS * OLED_SCREEN_PAGES) != 0)
	{
		return -1;
	}
	
	return 0;
}

static int OLED_SendCommand(OLED_TypeDef *OLED, uint8_t Cmd, const uint8_t *Arg, uint16_t Size)
{
	uint8_t buf[8];
	uint8_t i;
	
	buf[0] = SSD1306_CTRL_COMMAND_STREAM;
	buf[1] = Cmd;
	
	for(i=0;i<Size;i++)
	{
		buf[i + 2] =  Arg[i];
	}
	
	if(OLED->i2c_write_cb(OLED_SLAVE_ADDR, buf, i+2) != 0)
	{
		return -1;
	}
	
	return 0;
}

static int OLED_SendData(OLED_TypeDef *OLED, uint8_t *pData, uint16_t Size)
{
	int ret = 0;
	
	uint8_t tmp = *(pData - 1);
	*(pData - 1) = SSD1306_CTRL_DATA_STREAM;
	
	if(OLED->i2c_write_cb(OLED_SLAVE_ADDR, pData - 1, Size + 1) != 0)
	{
		ret = -1;
	}
	
	*(pData - 1) = tmp;
	
	return ret;
}

/* 在指定坐标绘制位图
 * 这是内部辅助函数，外部通常直接用 OLED_DrawBitmap()。
 */
static void DrawBitmapEx(OLED_TypeDef *OLED,int16_t X, int16_t Y, uint16_t Width, uint16_t Height, const uint8_t *pBitmap)
{
	int16_t x,y;
	
	if(OLED->Brush == BRUSH_TRANSPARENT && OLED->PenColor == PEN_COLOR_TRANSPARENT) return;
	
	uint16_t penWidthCpy = OLED->PenWidth;
	
	OLED->PenWidth = 1;
	
	uint16_t nBytesPerRow = (uint16_t)ceil(Width / 8.0);
	
	for(x=0;x<Width;x++)
	{
		for(y=0;y<Height;y++)
		{
			if((pBitmap[x/8+y*nBytesPerRow] & (0x80>>(x%8))) != 0)
			{
				PenDot(OLED, X+x, Y+y);
			}
			else
			{
				BrushDot(OLED, X+x, Y+y);
			}
		}
	}
	
	OLED->PenWidth = penWidthCpy;
}

/* 以当前光标为左上角绘制位图 */
void OLED_DrawBitmap(OLED_TypeDef *OLED, uint16_t Width, uint16_t Height, const uint8_t *pBitmap)
{
	DrawBitmapEx(OLED, OLED->CursorX, OLED->CursorY, Width, Height, pBitmap);
}

/* 内部查找字符对应字形序号 */
static int16_t unicode_2_glyph_idx(OLED_TypeDef *OLED, uint32_t Unicode)
{
	int16_t ret = -1;
	
	if(OLED->Font == 0)
	{
		return -1;
	}
	
	uint32_t i;
	
	for(i = 0; i < OLED->Font->nChars; i++)
	{
		if(OLED->Font->Map[i] == Unicode)
		{
			ret = i;
			break;
		}
	}
	
	return ret;
}

static void PenDot(OLED_TypeDef *OLED, int16_t x, int16_t y)
{
	if(OLED->PenColor == PEN_COLOR_TRANSPARENT)
	{
		return;
	}
	
	if(OLED->PenWidth == 0)
	{
		return;
	}
	
	uint16_t borderLeft, borderRight, borderTop, borderBottom;
	
	if(OLED->PenWidth % 2)
	{
		borderLeft = borderRight = borderTop = borderBottom = OLED->PenWidth / 2;
	}
	else
	{
		borderLeft = borderRight = borderTop = borderBottom = OLED->PenWidth / 2;
		borderLeft--;
		borderTop--;
	}
	
	uint8_t brushCpy = OLED->Brush;
	
	OLED->Brush = OLED->PenColor;
	
	int16_t i,j;
	
	for(i=x-borderLeft;i<=x+borderRight;i++)
	{
		for(j=y-borderTop;j<=y+borderBottom;j++)
		{
			BrushDot(OLED, i, j);
		}
	}
	
	OLED->Brush = brushCpy;
}

static void BrushDot(OLED_TypeDef *OLED, int16_t x, int16_t y)
{
	if(OLED->Brush == BRUSH_TRANSPARENT)
	{
		return;
	}
	
	if(x < 0 || x >= OLED_SCREEN_COLS)
	{
		return;
	}
	
	if(y<0 || y >= OLED_SCREEN_ROWS)
	{
		return;
	}
	
	if(OLED->ClipRegionWidth != 0 && OLED->ClipRegionHeight != 0)
	{
		if(x<OLED->ClipRegionX || x>=OLED->ClipRegionX + OLED->ClipRegionWidth)
		{
			return;
		}
		if(y<OLED->ClipRegionY || y>=OLED->ClipRegionY + OLED->ClipRegionHeight)
		{
			return;
		}
	}
	
	if(OLED->Brush == BRUSH_WHITE)
	{
		OLED->pBuffer[x + y / 8 * OLED_SCREEN_COLS] |= 1 << (y % 8);
	}
	else
	{
		OLED->pBuffer[x + y / 8 * OLED_SCREEN_COLS] &= ~(1 << (y % 8));
	}
}
