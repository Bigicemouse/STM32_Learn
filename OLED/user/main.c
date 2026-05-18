#include "oled.h"
#include "si2c.h"
#include "stm32f10x.h"

/*
 * 本例程功能：
 *   使用 PB8/PB9 模拟 I2C 总线，驱动 128x64 OLED 显示屏，
 *   上电后在屏幕上显示一个简单的演示页面。
 *
 * OLED12864 接线说明：
 *   OLED VCC -> 3.3V
 *   OLED GND -> GND
 *   OLED SCL -> PB8
 *   OLED SDA -> PB9
 *
 * 注意：
 *   本工程使用 STM32 标准外设库和软件 I2C，不依赖硬件 I2C 外设。
 */

/* 软件 I2C 控制对象：记录 SCL/SDA 所在 GPIO 端口和引脚 */
static SI2C_TypeDef g_oled_i2c;

/* OLED 驱动对象：保存显存、画笔、光标、字体和 I2C 写回调等状态 */
static OLED_TypeDef g_oled;

int main(void)
{
    OLED_InitTypeDef oled_init;

    /*
     * 初始化 OLED 通信引脚。
     * OLED_GPIO_Init() 内部会把 PB8/PB9 配置成软件 I2C 所需的 GPIO。
     */
    OLED_GPIO_Init();

    /*
     * 给 OLED 驱动注册底层写函数。
     * OLED 驱动本身不直接操作 GPIO，而是通过该回调发送 I2C 数据。
     */
    oled_init.i2c_write_cb = OLED_I2C_Write;

    /*
     * 初始化 OLED 驱动对象。
     * 返回 0 表示初始化成功；成功后先绘制演示页面，再刷新到屏幕。
     */
    if (OLED_Init(&g_oled, &oled_init) == 0)
    {
        OLED_ShowDemoPage();
    }

    /*
     * 主循环保持空转。
     * 当前例程只需要上电显示静态页面，没有周期性刷新任务。
     */
    while (1)
    {
    }
}

/* ---------- 以下为各函数实现 ---------- */

/**
 * @brief OLED 驱动使用的 I2C 写数据回调函数。
 * @param addr  OLED 的 I2C 从机地址，通常由 OLED 驱动传入。
 * @param pdata 指向待发送数据缓冲区的指针。
 * @param size  本次需要发送的数据字节数。
 * @return 0 表示发送成功，非 0 表示发送失败。
 *
 * OLED 驱动在初始化命令、刷新显存、发送显示数据时都会调用该函数。
 * 这里把驱动层请求转交给 My_SI2C_SendBytes()，由软件 I2C 完成实际时序。
 */
static int OLED_I2C_Write(uint8_t addr, const uint8_t *pdata, uint16_t size)
{
    return My_SI2C_SendBytes(&g_oled_i2c, addr, pdata, size);
}

/**
 * @brief 初始化 OLED 所用的软件 I2C 引脚。
 *
 * 当前接线使用：
 *   SCL -> PB8
 *   SDA -> PB9
 *
 * 设置好引脚信息后，调用 My_SI2C_Init() 完成 GPIO 模式配置。
 */
static void OLED_GPIO_Init(void)
{
    /* 指定软件 I2C 的时钟线 SCL 使用 GPIOB 的 PB8 */
    g_oled_i2c.SCL_GPIOx = GPIOB;
    g_oled_i2c.SCL_GPIO_Pin = GPIO_Pin_8;

    /* 指定软件 I2C 的数据线 SDA 使用 GPIOB 的 PB9 */
    g_oled_i2c.SDA_GPIOx = GPIOB;
    g_oled_i2c.SDA_GPIO_Pin = GPIO_Pin_9;

    /* 根据上面的端口和引脚配置，初始化软件 I2C 总线 */
    My_SI2C_Init(&g_oled_i2c);
}

/**
 * @brief 在 OLED 上绘制静态演示页面。
 *
 * 页面内容：
 *   1. 全屏边框
 *   2. OLED 型号
 *   3. SCL/SDA 接线信息
 *   4. MCU 型号
 *
 * OLED 驱动采用显存方式绘图：前面的绘制函数只修改 g_oled 的显存，
 * 最后必须调用 OLED_SendBuffer()，屏幕才会真正更新。
 */
static void OLED_ShowDemoPage(void)
{
    /* 清空显存，避免上一次显示内容残留 */
    OLED_Clear(&g_oled);

    /*
     * 设置绘图样式并画边框。
     * 光标 (0, 0) 是屏幕左上角，矩形宽 128、高 64，刚好覆盖整块 OLED。
     */
    OLED_SetPen(&g_oled, PEN_COLOR_WHITE, 1);
    OLED_SetBrush(&g_oled, BRUSH_TRANSPARENT);
    OLED_SetCursor(&g_oled, 0, 0);
    OLED_DrawRect(&g_oled, 128, 64);

    /* 第 1 行文字：显示屏型号 */
    OLED_SetCursor(&g_oled, 8, 16);
    OLED_DrawString(&g_oled, "OLED 12864");

    /* 第 2 行文字：软件 I2C 时钟线连接到 PB8 */
    OLED_SetCursor(&g_oled, 8, 32);
    OLED_DrawString(&g_oled, "SCL: PB8");

    /* 第 3 行文字：软件 I2C 数据线连接到 PB9 */
    OLED_SetCursor(&g_oled, 8, 44);
    OLED_DrawString(&g_oled, "SDA: PB9");

    /* 第 4 行文字：当前例程面向 STM32F103C8T6 */
    OLED_SetCursor(&g_oled, 8, 56);
    OLED_DrawString(&g_oled, "STM32F103C8T6");

    /* 将显存中的全部绘图结果一次性发送到 OLED 屏幕 */
    OLED_SendBuffer(&g_oled);
}
