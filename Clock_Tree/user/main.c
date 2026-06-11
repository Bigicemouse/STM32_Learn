/*
 * main.c
 * 简单示例：使用标准外设库在 PC13 上闪烁 LED
 * 延时使用空循环，按图片中计算：500ms 大约需要 400000 次空循环（基于 8MHz、每次循环约 10 个时钟周期）
 */

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/* 延时计数（可根据具体编译器优化和时钟微调） */
#define DELAY_500MS 400000UL

int main(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能 PC 口时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    /* 配置 PC13 为推挽输出 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    while (1)
    {
        /* 点亮 LED（多数开发板 PC13 为板载 LED，通常低电平点亮） */
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
        for (volatile uint32_t i = 0; i < DELAY_500MS; i++)
        {
        }

        /* 熄灭 LED */
        GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
        for (volatile uint32_t i = 0; i < DELAY_500MS; i++)
        {
        }
    }
}
