/**
  ******************************************************************************
  * 简介：适用于pal库oled驱动的中文字体文件
  *       包含"明天你好"四个中文字符
  * 字体名称：Chinese 16x16
  * 字体字号：16磅
  * 字符数量：4
  ******************************************************************************
*/

#ifndef _OLED_CHINESE_FONT_H_
#define _OLED_CHINESE_FONT_H_

#include "oled_font.h"

/* 中文字形数据 (16x16) */
// 明 (U+660E) - 16x16像素，每行2字节，共32字节
static const uint8_t chinese_font_GlyphBitmap_660E[] = {
    0x7C,0xFC,0x64,0xC4,0x64,0xC4,0x64,0xFC,
    0x7C,0xC4,0x64,0xC4,0x64,0xC4,0x64,0xFC,
    0x7C,0x84,0x64,0x84,0x60,0x84,0x01,0x84,
    0x03,0x0C,0x00,0x00,0x00,0x00,0x00,0x00
};

// 天 (U+5929) - 16x16像素，每行2字节，共32字节
static const uint8_t chinese_font_GlyphBitmap_5929[] = {
    0x3F,0xF8,0x01,0x80,0x01,0x80,0x01,0x80,
    0x7F,0xFE,0x7F,0xFE,0x01,0x80,0x03,0xC0,
    0x02,0x40,0x06,0x20,0x0C,0x30,0x38,0x0C,
    0x60,0x06,0x00,0x00,0x00,0x00,0x00,0x00
};

// 你 (U+4F60) - 16x16像素，每行2字节，共32字节
static const uint8_t chinese_font_GlyphBitmap_4F60[] = {
    0x0C,0x80,0x09,0x80,0x11,0xFE,0x11,0xFE,
    0x32,0x04,0x76,0x6C,0x50,0x60,0x11,0x68,
    0x11,0x6C,0x13,0x64,0x16,0x66,0x14,0x62,
    0x10,0x60,0x10,0xE0,0x00,0x00,0x00,0x00
};

// 好 (U+597D) - 16x16像素，每行2字节，共32字节
static const uint8_t chinese_font_GlyphBitmap_597D[] = {
    0x08,0x00,0x18,0xFE,0x10,0xFE,0x10,0x0C,
    0x7E,0x10,0x12,0x10,0x26,0x10,0x25,0xFE,
    0x64,0x10,0x1C,0x10,0x0C,0x10,0x16,0x10,
    0x22,0x10,0x40,0x70,0x00,0x00,0x00,0x00
};

/* 中文映射表 */
static const uint32_t chinese_font_Map[] = {
    0x660E, // 明
    0x5929, // 天
    0x4F60, // 你
    0x597D  // 好
};

/* 中文字形 */
static const Glyph_TypeDef chinese_font_Glyphs[] = {
    // 明
    {
        .Name = "660E",
        .Encoding = 0x660E,
        .Swx0 = 1000,
        .Swy0 = 0,
        .Dwx0 = 16,
        .Dwy0 = 0,
        .BBw = 16,
        .BBh = 16,
        .BBxoff0x = 0,
        .BByoff0y = 0,
        .nBytes = 32,
        .Bitmap = chinese_font_GlyphBitmap_660E,
    },
    // 天
    {
        .Name = "5929",
        .Encoding = 0x5929,
        .Swx0 = 1000,
        .Swy0 = 0,
        .Dwx0 = 16,
        .Dwy0 = 0,
        .BBw = 16,
        .BBh = 16,
        .BBxoff0x = 0,
        .BByoff0y = 0,
        .nBytes = 32,
        .Bitmap = chinese_font_GlyphBitmap_5929,
    },
    // 你
    {
        .Name = "4F60",
        .Encoding = 0x4F60,
        .Swx0 = 1000,
        .Swy0 = 0,
        .Dwx0 = 16,
        .Dwy0 = 0,
        .BBw = 16,
        .BBh = 16,
        .BBxoff0x = 0,
        .BByoff0y = 0,
        .nBytes = 32,
        .Bitmap = chinese_font_GlyphBitmap_4F60,
    },
    // 好
    {
        .Name = "597D",
        .Encoding = 0x597D,
        .Swx0 = 1000,
        .Swy0 = 0,
        .Dwx0 = 16,
        .Dwy0 = 0,
        .BBw = 16,
        .BBh = 16,
        .BBxoff0x = 0,
        .BByoff0y = 0,
        .nBytes = 32,
        .Bitmap = chinese_font_GlyphBitmap_597D,
    }
};

/* 中文字体结构 */
const Font_TypeDef chinese_font = {
    .SpecVersion = "2.1",
    .FontName = "Chinese 16x16",
    .ContentVersion = 1,
    .MetricsSet = 0,
    .FontSize = 16,
    .Xres = 72,
    .Yres = 72,
    .FBBx = 16,
    .FBBy = 16,
    .FBBXoff = 0,
    .FBBYoff = 0,
    .nChars = 4,
    .Map = chinese_font_Map,
    .Glyphs = chinese_font_Glyphs,
};

#endif
