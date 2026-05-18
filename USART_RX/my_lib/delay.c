/**
  ******************************************************************************
  * @file    delay.c
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年8月30日
  * @brief   延迟函数源文件
  ******************************************************************************
  */

#include "delay.h"

__IO uint32_t ulTicks;

static uint8_t delay_initialized_flag = 0;
static float us_per_mini_tick;

/* 用法说明：
 * 1. Delay_Init() 一般在系统启动后调用 1 次，也可以直接调用其他接口，由模块内部自动初始化。
 * 2. Delay() 适合简单阻塞延时，不适合放在对实时性要求高的流程中。
 * 3. GetTick() / GetUs() 适合做超时判断和时间戳记录。
 * 4. DelayUs() 适合短时间微秒级等待。
 */

/* 初始化延时模块
 * 配置 SysTick 为本模块提供毫秒计时基准。
 */
void Delay_Init(void)
{
	if(!delay_initialized_flag)
	{
		delay_initialized_flag = 1;
		
		RCC_ClocksTypeDef clockinfo = {0};
		uint32_t tmp;
		
		SysTick->CTRL &= ~SysTick_CTRL_ENABLE;

		ulTicks = 0;

		RCC_GetClocksFreq(&clockinfo);

		SysTick->CTRL |= SysTick_CTRL_TICKINT;
		
		SCB->SHP[7] = 0;

		tmp =  clockinfo.HCLK_Frequency / 1000;
		if(tmp > 0x00ffffff)
		{
			tmp = tmp / 8;
			SysTick->CTRL &= ~SysTick_CTRL_CLKSOURCE; 
		}
		else
		{
			SysTick->CTRL |= SysTick_CTRL_CLKSOURCE; 
		}
		SysTick->LOAD = tmp - 1;

		SysTick->CTRL |= SysTick_CTRL_ENABLE; 
		
		us_per_mini_tick = 1000.0 / ((SysTick->LOAD & 0x00ffffff) + 1);
	}
}

/* 毫秒级阻塞延时
 * 调用后 CPU 会一直等待到指定时间，不会主动让出执行权。
 */
void Delay(uint32_t Delay)
{
	Delay_Init();
	
	uint64_t expiredTime = ulTicks + Delay;

	while(ulTicks <  expiredTime){}
}

/* 获取当前毫秒时间
 * 常用于轮询超时、按键扫描、状态机计时。
 */
uint32_t GetTick(void)
{
	Delay_Init();
	
	return ulTicks;
}

/* 获取当前微秒时间
 * 适合更细粒度的时间测量。
 */
uint64_t GetUs(void)
{
	Delay_Init();
	
	uint64_t tick;
	uint32_t mini_tick;
	
	SysTick->CTRL &= ~SysTick_CTRL_COUNTFLAG;
	
	tick = ulTicks;
	mini_tick = SysTick->VAL;
	
	while(SysTick->CTRL & SysTick_CTRL_COUNTFLAG) 
	{
		mini_tick = SysTick->VAL;
		tick = ulTicks;
	}
	
	tick *= 1000;
	tick += (uint32_t)((SysTick->LOAD - mini_tick) * us_per_mini_tick);
	
	return tick;
}

/* 微秒级阻塞延时
 * 适合总线时序等待、短脉冲延时等场景。
 */
void DelayUs(uint32_t us)
{
	Delay_Init();
	
	uint64_t expiredTime = GetUs() + us + 1;
	
	while(GetUs() < expiredTime);
}

