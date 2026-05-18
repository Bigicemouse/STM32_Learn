/**
  ******************************************************************************
  * @file    button.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年9月7日
  * @brief   按钮驱动程序
  ******************************************************************************
  */
	
#ifndef _BUTTON_H_
#define _BUTTON_H_

#include "stm32f10x.h"

/* 按钮初始化参数 */
typedef struct
{
	GPIO_TypeDef *GPIOx;   // 按钮所在 GPIO 端口
	uint16_t GPIO_Pin;     // 按钮所在引脚
	void (*button_pressed_cb)(void);           // 按下回调
	void (*button_released_cb)(void);          // 松开回调
	void (*button_clicked_cb)(uint8_t clicks); // 点击回调
	void (*button_long_pressed_cb)(uint8_t ticks); // 长按回调
	uint32_t LongPressTime; // 长按阈值，单位 ms
	uint32_t LongPressTickInterval; // 长按持续触发间隔，单位 ms
	uint32_t ClickInterval; // 连击判定间隔，单位 ms
	
} Button_InitTypeDef;

/* 按钮运行对象 */
typedef struct 
{
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	void (*button_pressed_cb)(void);
	void (*button_released_cb)(void);
	void (*button_clicked_cb)(uint8_t clicks);
	void (*button_long_pressed_cb)(uint8_t ticks);
	uint32_t LongPressThreshold;
	uint32_t LongPressTickInterval;
	uint32_t ClickInterval; 
	
	uint8_t  LastState;
	uint8_t  ChangePending;
	uint32_t PendingTime;
	
	uint32_t LastPressedTime;
	uint32_t LastReleasedTime;
	
	uint8_t LongPressTicks;
	uint32_t LastLongPressTickTime; 
	
	uint8_t ClickCnt;
	
} Button_TypeDef;

/* 初始化按钮对象
 * 输入：Button，Button_InistStruct
 * 输出：无
 */
void My_Button_Init(Button_TypeDef *Button, Button_InitTypeDef *Button_InistStruct);

/* 按钮轮询处理
 * 输入：Button
 * 输出：无
 */
void My_Button_Proc(Button_TypeDef *Button);

/* 获取按钮当前状态
 * 输入：Button
 * 输出：0 松开，1 按下
 */
uint8_t MyButton_GetState(Button_TypeDef *Button);

#endif
