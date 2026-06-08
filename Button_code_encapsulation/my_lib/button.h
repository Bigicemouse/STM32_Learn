/**
 ******************************************************************************
 * @file    button.h
 * @author  铁头山羊
 * @version V 1.0.0
 * @date    2022年9月7日
 * @brief   按钮驱动程序
 *          支持：消抖、单击、多击、长按检测，通过回调函数通知应用层
 ******************************************************************************
 */

#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "stm32f10x.h"

/**
 * @brief 按钮初始化配置结构体（用户填写）
 * @note  用于 My_Button_Init()，将配置参数传入驱动实例
 */
typedef struct
{
    GPIO_TypeDef *GPIOx; /* 按钮连接的GPIO端口，取值：GPIOA ~ GPIOD */

    uint16_t GPIO_Pin; /* 按钮连接的GPIO引脚，取值：GPIO_Pin_0 ~ GPIO_Pin_15 */

    /* 回调函数 —— 按键事件触发时由驱动调用 */
    void (*button_pressed_cb)(void);               // 按下瞬间触发
    void (*button_released_cb)(void);              // 松开瞬间触发
    void (*button_clicked_cb)(uint8_t clicks);     // 单击/多击触发，clicks为累计点击次数
    void (*button_long_pressed_cb)(uint8_t ticks); // 长按触发，ticks为长按持续的tick数

    uint32_t LongPressTime;         // 长按判定阈值，单位：ms
    uint32_t LongPressTickInterval; // 长按期间连续触发回调的间隔，单位：ms
    uint32_t ClickInterval;         // 多击的最大间隔，两次点击超过此值则重新计数，单位：ms

} Button_InitTypeDef;

/**
 * @brief 按钮驱动实例结构体（驱动内部使用）
 * @note  包含初始化参数 + 运行时状态，每个按键对应一个实例
 */
typedef struct
{
    /* ---- 初始化参数（由 My_Button_Init 从配置结构体复制） ---- */
    GPIO_TypeDef *GPIOx;                           // GPIO端口
    uint16_t GPIO_Pin;                             // GPIO引脚
    void (*button_pressed_cb)(void);               // 按下回调
    void (*button_released_cb)(void);              // 松开回调
    void (*button_clicked_cb)(uint8_t clicks);     // 单击/多击回调
    void (*button_long_pressed_cb)(uint8_t ticks); // 长按回调

    uint32_t LongPressThreshold;    // 长按阈值(ms)
    uint32_t LongPressTickInterval; // 长按连续触发间隔(ms)
    uint32_t ClickInterval;         // 多击间隔(ms)

    /* ---- 消抖状态 ---- */
    uint8_t LastState;     // 上次采样的电平：0=松开，1=按下
    uint8_t ChangePending; // 是否检测到电平变化，正在消抖中：0=消抖结束，1=消抖中
    uint32_t PendingTime;  // 检测到变化的时刻(ms)，用于消抖计时

    /* ---- 事件追踪 ---- */
    uint32_t LastPressedTime;  // 上次按下的时刻(ms)
    uint32_t LastReleasedTime; // 上次松开的时刻(ms)

    uint8_t LongPressTicks;         // 长按期间已触发回调的次数
    uint32_t LastLongPressTickTime; // 上次触发长按回调的时刻(ms)

    uint8_t ClickCnt; // 当前累计点击次数（用于多击检测）

} Button_TypeDef;

/**
 * @brief  初始化按钮实例，将配置结构体的参数复制到驱动实例
 * @param  Button: 指向要初始化的 Button_TypeDef 实例
 * @param  Button_InistStruct: 指向用户填写的配置结构体
 */
void My_Button_Init(Button_TypeDef *Button, Button_InitTypeDef *Button_InistStruct);

/**
 * @brief  按钮处理函数，需在主循环或定时器中周期调用
 * @param  Button: 要处理的 Button_TypeDef 实例
 * @note   内部完成消抖、单击/多击判定、长按检测，并调用对应的回调函数
 */
void My_Button_Proc(Button_TypeDef *Button);

/**
 * @brief  获取按钮当前状态
 * @param  Button: 要查询的 Button_TypeDef 实例
 * @retval 0=松开，1=按下
 */
uint8_t MyButton_GetState(Button_TypeDef *Button);

#endif
