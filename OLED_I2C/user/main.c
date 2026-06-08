#include "i2c.h"
#include "oled.h"
#include "oled_chinese_font.h"
#include "stm32f10x.h"

static int OLED_I2C_Write(uint8_t addr, const uint8_t *pdata, uint16_t size)
{
    return My_I2C_SendBytes(I2C1, addr, pdata, size);
}

int main(void)
{
    OLED_TypeDef OLED = {0};
    OLED_InitTypeDef OLED_InitStruct = {0};

    My_I2C_Init(I2C1);
    OLED_InitStruct.i2c_write_cb = OLED_I2C_Write;

    if (OLED_Init(&OLED, &OLED_InitStruct) == 0)
    {
        OLED_Clear(&OLED);
        OLED_SetFont(&OLED, &chinese_font);
        OLED_SetCursor(&OLED, 0, 16);
        OLED_DrawString(&OLED, "明天你好");

        OLED_SetFont(&OLED, &default_font);
        OLED_SetCursor(&OLED, 0, 40);
        OLED_DrawString(&OLED, "Hello tomorrow");

        OLED_SendBuffer(&OLED);
    }

    while (1)
    {
    }
}
