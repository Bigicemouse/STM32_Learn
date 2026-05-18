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

#define BUTTON_SETTLING_TIME             10
#define BUTTON_CLICK_INTERVAL            200
#define BUTTON_LONG_PRESS_THRESHOLD      1000
#define BUTTON_LONG_PRESS_TICK_INTERNVAL 100

static void OnButtonPressed(Button_TypeDef *Button);
static void OnButtonReleased(Button_TypeDef *Button);
static void OnButtonEveryPolled(Button_TypeDef *Button, uint8_t State, uint32_t currentTime);
static void GPIOClockCmd(GPIO_TypeDef *GPIOx, uint8_t Enable);

/* 用法说明：
 * 1. 先定义 Button_TypeDef 和 Button_InitTypeDef。
 * 2. 调用 My_Button_Init() 完成按钮对象初始化。
 * 3. 在 main 的 while(1) 中持续调用 My_Button_Proc()。
 * 4. 按下、松开、点击、长按事件通过回调函数通知用户代码。
 */

/* 初始化按钮对象
 * 会配置输入引脚，并把回调和阈值参数装载到运行对象里。
 */
void My_Button_Init(Button_TypeDef *Button, Button_InitTypeDef *Button_InistStruct)
{
	Button->GPIOx = Button_InistStruct->GPIOx;
	Button->GPIO_Pin = Button_InistStruct->GPIO_Pin;
	Button->button_pressed_cb = Button_InistStruct->button_pressed_cb;
	Button->button_released_cb = Button_InistStruct->button_released_cb;
	Button->button_clicked_cb = Button_InistStruct->button_clicked_cb;
	Button->button_long_pressed_cb = Button_InistStruct->button_long_pressed_cb;
	Button->LongPressThreshold = Button_InistStruct->LongPressTime;
	Button->ClickInterval = Button_InistStruct->ClickInterval;
	Button->LongPressTickInterval = Button_InistStruct->LongPressTickInterval;
	
	GPIOClockCmd(Button->GPIOx, 1);
	
	GPIO_InitTypeDef gpio_init_struct;
	
	gpio_init_struct.GPIO_Pin = Button->GPIO_Pin;
	gpio_init_struct.GPIO_Mode = GPIO_Mode_IPU;
	
	GPIO_Init(Button->GPIOx, &gpio_init_struct);
	
	if(Button->LongPressThreshold == 0)
	{
		Button->LongPressThreshold = BUTTON_LONG_PRESS_THRESHOLD;
	}
	
	if(Button->LongPressTickInterval == 0)
	{
		Button->LongPressTickInterval = BUTTON_LONG_PRESS_TICK_INTERNVAL;
	}
	
	if(Button->ClickInterval == 0)
	{
		Button->ClickInterval = BUTTON_CLICK_INTERVAL;
	}
	
	Button->LastState = 0;
	Button->ChangePending = 0; 
	Button->PendingTime = 0;
	Button->LastPressedTime = 0;
	Button->LastReleasedTime = 0;
	Button->LongPressTicks = 0;
	Button->ClickCnt = 0;
}

/* 按钮轮询处理函数
 * 需要在主循环中周期调用，调用间隔越稳定，效果越好。
 */
void My_Button_Proc(Button_TypeDef *Button)
{
	uint8_t currentState;
	
	uint32_t currentTime = GetTick();
	
	if(Button->ChangePending)
	{
		if (currentTime >= Button->PendingTime + BUTTON_SETTLING_TIME)
		{
			currentState = GPIO_ReadInputDataBit(Button->GPIOx, Button->GPIO_Pin) == Bit_RESET ? 1 : 0;
			
			if(currentState != Button->LastState)
			{
				if(currentState == 1) 
					OnButtonPressed(Button);
				else 
					OnButtonReleased(Button);
			}
			Button->LastState = currentState;
			Button->ChangePending = 0;
		}
	}
	else
	{
		currentState = GPIO_ReadInputDataBit(Button->GPIOx, Button->GPIO_Pin) == Bit_RESET ? 1 : 0;
		
		if(currentState != Button->LastState)
		{
			Button->PendingTime = currentTime;
			Button->ChangePending = 1;
		}
	}
	
	OnButtonEveryPolled(Button, Button->LastState, currentTime);
}

/* 获取按钮当前状态
 * 返回值：0 松开，1 按下。
 */
uint8_t MyButton_GetState(Button_TypeDef *Button)
{
	return Button->LastState;
}

/* 处理按钮按下事件 */
static void OnButtonPressed(Button_TypeDef *Button)
{
	Button->LastPressedTime = GetTick();
	
	if(Button->button_pressed_cb != 0)
	{
		Button->button_pressed_cb();
	}
}

/* 处理按钮松开事件 */
static void OnButtonReleased(Button_TypeDef *Button)
{
	Button->LastReleasedTime = GetTick();
	
	if(Button->button_released_cb != 0)
	{
		Button->button_released_cb();
	}
	
	Button->LongPressTicks = 0;
	
	if(Button->LastReleasedTime - Button->LastPressedTime < Button->LongPressThreshold)
	{
		Button->ClickCnt++;
	}
	else
	{
		Button->ClickCnt = 0;
	}
}

/* 轮询期间的附加处理
 * 负责长按和连击逻辑。
 */
static void OnButtonEveryPolled(Button_TypeDef *Button, uint8_t State, uint32_t CurrentTime)
{
	if(Button->LastState == 1)
	{
		if(Button->LongPressTicks == 0)
		{
			if(Button->LastPressedTime!= 0 
				&& CurrentTime - Button->LastPressedTime > Button->LongPressThreshold)
			{
				Button->LongPressTicks = 1;
			
				if(Button->button_long_pressed_cb)
				{
					Button->button_long_pressed_cb(Button->LongPressTicks);
				}
				
				Button->LastLongPressTickTime = GetTick();
			}
		}
		else
		{
			if(CurrentTime - Button->LastLongPressTickTime > Button->LongPressTickInterval)
			{
				Button->LastLongPressTickTime = GetTick();
				
				Button->LongPressTicks++;
				
				if(Button->button_long_pressed_cb)
				{
					Button->button_long_pressed_cb(Button->LongPressTicks);
				}
			}
		}
	}

	if(Button->ClickCnt > 0 && Button->LastState == 0 && (GetTick() - Button->LastReleasedTime) > Button->ClickInterval)
	{
		if(Button->button_clicked_cb)
		{
			Button->button_clicked_cb(Button->ClickCnt);
		}
		
		Button->ClickCnt = 0;
	}
}

static void GPIOClockCmd(GPIO_TypeDef *GPIOx, uint8_t Enable)
{
	FunctionalState newState = Enable ? ENABLE : DISABLE;
	
	if(GPIOx == GPIOA)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, newState);
	}
	else if(GPIOx == GPIOB)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, newState);
	}
	else if(GPIOx == GPIOC)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, newState);
	}
	else if(GPIOx == GPIOD)
	{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, newState);
	}
}
