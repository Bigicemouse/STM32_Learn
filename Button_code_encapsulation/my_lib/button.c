/**
 ******************************************************************************
 * @file    button.c
 * @author  铁头山羊
 * @version V 1.0.0
 * @date    2022年9月7日
 * @brief   按钮驱动程序
 ******************************************************************************
 */

#include "button.h"
#include "delay.h"

#define Button_Settling_Time 10              // 按钮消抖延迟
#define Button_Click_Interval 200            // 按钮多击时每次点击的时间最大时间间隔
#define Button_Long_Press_Threshold 1000     // 按钮长按最小时间
#define Button_Long_Press_Tick_Internval 100 // 长按后持续触发的时间间隔

static void OnButtonPressed(Button_TypeDef *Button);
static void OnButtonReleased(Button_TypeDef *Button);
static void OnButtonEveryPolled(Button_TypeDef *Button, uint8_t State, uint32_t currentTime);
static void GPIOClockCmd(GPIO_TypeDef *GPIOx, uint8_t Enable);

//
// @简介：用于初始化按钮的驱动
// @参数：Button - 按钮的名称
// @返回值：无
//
void My_Button_Init(Button_TypeDef *Button, Button_InitTypeDef *Button_InistStruct)
{

    // 复制初始化参数到驱动实例
    Button->GPIOx = Button_InistStruct->GPIOx;
    Button->GPIO_Pin = Button_InistStruct->GPIO_Pin;
    Button->button_pressed_cb = Button_InistStruct->button_pressed_cb;
    Button->button_released_cb = Button_InistStruct->button_released_cb;
    Button->button_clicked_cb = Button_InistStruct->button_clicked_cb;
    Button->button_long_pressed_cb = Button_InistStruct->button_long_pressed_cb;
    Button->LongPressThreshold = Button_InistStruct->LongPressTime;
    Button->ClickInterval = Button_InistStruct->ClickInterval;
    Button->LongPressTickInterval = Button_InistStruct->LongPressTickInterval;

    // #1. 使能GPIOx的时钟
    GPIOClockCmd(Button->GPIOx, 1);

    // #2. 初始化IO引脚
    GPIO_InitTypeDef gpio_init_struct;

    gpio_init_struct.GPIO_Pin = Button->GPIO_Pin;
    gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;

    GPIO_Init(Button->GPIOx, &gpio_init_struct);

    // 未配置则使用默认的长按触发阈值
    if (Button->LongPressThreshold == 0)
    {
        Button->LongPressThreshold = Button_Long_Press_Threshold;
    }

    // 未配置则使用默认的长按连触间隔
    if (Button->LongPressTickInterval == 0)
    {
        Button->LongPressTickInterval = Button_Long_Press_Tick_Internval;
    }

    // 未配置则使用默认的多击间隔
    if (Button->ClickInterval == 0)
    {
        Button->ClickInterval = Button_Click_Interval;
    }

    // 初始化运行时状态
    Button->LastState = 0; // 初始状态下假设按钮是松开的
    Button->ChangePending = 0;
    Button->PendingTime = 0;
    Button->LastPressedTime = 0;
    Button->LastReleasedTime = 0;
    Button->LongPressTicks = 0;
    Button->ClickCnt = 0;
}

/**
 * @brief 按键轮询处理函数
 * @param Button 按键实例指针
 * @note 需要在 main 函数的 while 循环中周期性调用
 *
 * 实现非阻塞消抖状态机:
 *   阶段 A: 检测到电平变化 -> 记录时间戳，标记 ChangePending
 *   阶段 B: 消抖窗口到达后二次采样 -> 确认变化则触发 Press/Release 事件
 */
void My_Button_Proc(Button_TypeDef *Button)
{
    uint8_t currentState;

    uint32_t currentTime = GetTick(); /* 获取当前系统时间 (ms) */

    /* 消抖状态机 */
    if (Button->ChangePending)
    {
        /* 阶段 B: 等待消抖窗口结束 */
        if (currentTime >= Button->PendingTime + Button_Settling_Time)
        {
            /* 二次采样: 读取当前电平 (低电平=按下=1, 高电平=松开=0) */
            currentState = GPIO_ReadInputDataBit(Button->GPIOx, Button->GPIO_Pin) == Bit_RESET ? 1 : 0;

            if (currentState != Button->LastState)
            {
                if (currentState == 1)
                    OnButtonPressed(Button); /* 电平变化确认: 按下事件 */
                else
                    OnButtonReleased(Button); /* 电平变化确认: 松开事件 */
            }
            Button->LastState = currentState; /* 更新状态 */
            Button->ChangePending = 0;        /* 清除待确认标记 */
        }
        /* 消抖窗口未到，继续等待 */
    }
    else  // ChangePending == 0，即"空闲态"
    {
        /* 阶段 A: 检测电平边沿 读取当前电平 (低电平=按下=1, 高电平=松开=0) */
        currentState = GPIO_ReadInputDataBit(Button->GPIOx, Button->GPIO_Pin) == Bit_RESET ? 1 : 0;

        if (currentState != Button->LastState)
        {
            Button->PendingTime = currentTime; /* 记录电平变化的时刻 */
            Button->ChangePending = 1;         /* 标记待消抖确认 */
        }
    }

    /* 持续回调: 无论状态是否变化都调用，用于长按周期性触发 */
    OnButtonEveryPolled(Button, Button->LastState, currentTime);
}

//
// @简介：返回按钮的当前状态
//
// @返回值：0 - 按钮松开  1 - 按钮按下
//
uint8_t MyButton_GetState(Button_TypeDef *Button)
{
    return Button->LastState;
}

//
// @简介：处理按钮按下的动作
//
static void OnButtonPressed(Button_TypeDef *Button)
{
    Button->LastPressedTime = GetTick();

    // 调用按钮按下的回调函数
    if (Button->button_pressed_cb != 0)
    {
        Button->button_pressed_cb();
    }
}

//
// @简介：处理按钮松开的动作
//
static void OnButtonReleased(Button_TypeDef *Button)
{
    Button->LastReleasedTime = GetTick();

    // 调用按钮松开的回调函数
    if (Button->button_released_cb != 0)
    {
        Button->button_released_cb();
    }

    // 松开后长按计数清零
    Button->LongPressTicks = 0;

    if (Button->LastReleasedTime - Button->LastPressedTime < Button->LongPressThreshold)
    {
        Button->ClickCnt++;
    }
    else
    {
        Button->ClickCnt = 0;
    }
}

//
// @简介：处理每一次按钮轮询的动作
//
static void OnButtonEveryPolled(Button_TypeDef *Button, uint8_t State, uint32_t CurrentTime)
{
    /* 处理按钮长按的动作 */

    if (Button->LastState == 1)
    {
        if (Button->LongPressTicks == 0) // 如果长按未被触发
        {
            if (Button->LastPressedTime != 0 && CurrentTime - Button->LastPressedTime > Button->LongPressThreshold) // 且已超过触发时间
            {
                Button->LongPressTicks = 1;

                if (Button->button_long_pressed_cb)
                {
                    Button->button_long_pressed_cb(Button->LongPressTicks); // 触发长按回调函数
                }

                Button->LastLongPressTickTime = GetTick();
            }
        }
        else
        {
            if (CurrentTime - Button->LastLongPressTickTime > Button->LongPressTickInterval) // 超过Tick间隔
            {
                Button->LastLongPressTickTime = GetTick();

                Button->LongPressTicks++;

                if (Button->button_long_pressed_cb)
                {
                    Button->button_long_pressed_cb(Button->LongPressTicks); // 触发长按回调函数
                }
            }
        }
    }

    /* 处理按钮连击动作 */

    if (Button->ClickCnt > 0 && Button->LastState == 0 && (GetTick() - Button->LastReleasedTime) > Button->ClickInterval)
    {
        if (Button->button_clicked_cb)
        {
            Button->button_clicked_cb(Button->ClickCnt);
        }

        Button->ClickCnt = 0; // 清除连击记录
    }
}

static void GPIOClockCmd(GPIO_TypeDef *GPIOx, uint8_t Enable)
{
    FunctionalState newState = Enable ? ENABLE : DISABLE;

    if (GPIOx == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, newState);
    }
    else if (GPIOx == GPIOB)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, newState);
    }
    else if (GPIOx == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, newState);
    }
    else if (GPIOx == GPIOD)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, newState);
    }
}
