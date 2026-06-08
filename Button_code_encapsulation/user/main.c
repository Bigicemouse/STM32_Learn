#include "button.h"
#include "delay.h"
#include "stm32f10x.h"
#include "usart.h"

/* 全局变量 */
uint8_t value = 0;              /* 计数值 */
Button_TypeDef button = {0};    /* 按键实例 */

/* 函数声明 */
void App_LED_Init(void);
void App_USART1_Init(void);
void App_Button_Init(void);
void App_SendValue(void);
void button_clicked_cb(uint8_t click);
void button_long_pressed_cb(uint8_t click);

/**
 * @brief 主函数
 * @note 初始化各外设后进入按键轮询循环
 */
int main(void)
{
    App_LED_Init();      /* 初始化 LED */
    App_USART1_Init();   /* 初始化串口 */
    App_Button_Init();   /* 初始化按键 */
    App_SendValue();     /* 显示初始值 */

    while (1)
    {
        My_Button_Proc(&button);  /* 轮询处理按键事件 */
    }
}

/**
 * @brief 发送当前值到串口并更新 LED
 * @note value 非零时点亮 LED
 */
void App_SendValue(void)
{
    if (value == 0)
    {
        GPIO_ResetBits(GPIOA, GPIO_Pin_0);   /* 熄灭 LED */
    }
    else
    {
        GPIO_SetBits(GPIOA, GPIO_Pin_0);     /* 点亮 LED */
    }

    My_USART_Printf(USART1, "value = %d\r\n", value);
}

/**
 * @brief 按键点击回调
 * @param click 点击次数
 * @note 单击: 加一, 双击及以上: 清零
 */
void button_clicked_cb(uint8_t click)
{
    if (click == 1)
    {
        value++;
        App_SendValue();
    }
    else if (click >= 2)
    {
        value = 0;
        App_SendValue();
    }
}

/**
 * @brief 按键长按回调
 * @param click 长按计数 (未使用)
 * @note 长按时持续加一
 */
void button_long_pressed_cb(uint8_t click)
{
    (void)click;

    value++;
    App_SendValue();
}

/**
 * @brief 初始化用户按键 (PA1)
 */
void App_Button_Init(void)
{
    Button_InitTypeDef Button_InitStruct = {0};

    Button_InitStruct.GPIOx = GPIOA;
    Button_InitStruct.GPIO_Pin = GPIO_Pin_1;
    Button_InitStruct.LongPressTickInterval = 10;    /* 长按触发间隔 10 tick */
    Button_InitStruct.LongPressTime = 1000;          /* 长按判定时间 1000 ms */
    Button_InitStruct.ClickInterval = 100;           /* 双击间隔 100 ms */
    Button_InitStruct.button_clicked_cb = button_clicked_cb;
    Button_InitStruct.button_long_pressed_cb = button_long_pressed_cb;

    My_Button_Init(&button, &Button_InitStruct);
}

/**
 * @brief 初始化 LED (PA0, 推挽输出)
 */
void App_LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_ResetBits(GPIOA, GPIO_Pin_0);  /* 默认熄灭 */
}

/**
 * @brief 初始化调试串口 USART1 (PA9-TX, PA10-RX, 115200bps)
 */
void App_USART1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    USART_InitTypeDef USART_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    /* TX: PA9 - 复用推挽输出 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* RX: PA10 - 上拉输入 */
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置串口参数 */
    USART_InitStruct.USART_BaudRate = 115200;
    USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;
    USART_InitStruct.USART_StopBits = USART_StopBits_1;
    USART_InitStruct.USART_Parity = USART_Parity_No;
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_Init(USART1, &USART_InitStruct);

    USART_Cmd(USART1, ENABLE);
}
