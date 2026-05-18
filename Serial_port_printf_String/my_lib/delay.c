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

#define DELAY_SYSTICK_RELOAD_MAX    ((uint32_t)0x00FFFFFFU)

/* SysTick 中断每 1 ms 自增一次
 * 该变量在 user/stm32f10x_it.c 的 SysTick_Handler() 中维护
 */
__IO uint32_t ulTicks = 0U;

static uint8_t delay_initialized_flag = 0U;
static uint32_t systick_reload_value = 0U;
static uint32_t systick_counts_per_ms = 0U;

/* 初始化延迟模块
 * 执行内容：
 * 1. 配置 SysTick 周期为 1 ms
 * 2. 使能 SysTick 中断
 * 3. 缓存计数参数，供 GetUs() 做整数换算
 */
void Delay_Init(void)
{
	if(delay_initialized_flag != 0U)
	{
		return;
	}

	RCC_ClocksTypeDef clockinfo = {0};
	uint32_t ctrl_value = SysTick_CTRL_TICKINT;
	uint32_t reload_count;

	RCC_GetClocksFreq(&clockinfo);

	/* 先关闭 SysTick，避免重配期间产生不确定状态 */
	SysTick->CTRL = 0U;
	SysTick->LOAD = 0U;
	SysTick->VAL = 0U;
	ulTicks = 0U;

	/* 优先使用 HCLK 作为时钟源；若重装值超范围，再退回 HCLK/8 */
	reload_count = clockinfo.HCLK_Frequency / 1000U;
	if(reload_count > DELAY_SYSTICK_RELOAD_MAX)
	{
		reload_count = clockinfo.HCLK_Frequency / 8000U;
	}
	else
	{
		ctrl_value |= SysTick_CTRL_CLKSOURCE;
	}

	/* 对于当前 STM32F103 环境，reload_count 正常情况下不会为 0
	 * 此处保留保护，避免异常时出现 LOAD = 0xFFFFFFFF 的错误配置
	 */
	if(reload_count == 0U)
	{
		reload_count = 1U;
	}

	systick_counts_per_ms = reload_count;
	systick_reload_value = reload_count - 1U;

	SysTick->LOAD = systick_reload_value;
	SysTick->VAL = 0U;

	/* 使用 CMSIS 接口设置 SysTick 优先级，避免直接写错误的 SHP 下标 */
	NVIC_SetPriority(SysTick_IRQn, 0U);

	SysTick->CTRL = ctrl_value | SysTick_CTRL_ENABLE;

	delay_initialized_flag = 1U;
}

/* 毫秒级阻塞延时
 * 采用差值比较，避免 ulTicks 回绕后直接失效
 */
void Delay(uint32_t ms)
{
	Delay_Init();

	if(ms == 0U)
	{
		return;
	}

	uint32_t start_tick = ulTicks;

	while((uint32_t)(ulTicks - start_tick) < ms)
	{
	}
}

/* 获取当前毫秒节拍 */
uint32_t GetTick(void)
{
	Delay_Init();
	
	return ulTicks;
}

/* 获取当前微秒时间
 * 为避免浮点运算，使用整数比例换算当前毫秒内的偏移量
 */
uint64_t GetUs(void)
{
	Delay_Init();

	uint32_t primask;
	uint32_t tick_ms;
	uint32_t current_value;
	uint64_t tick_us;

	/* 进入短临界区，保证 ulTicks 与 SysTick->VAL 的读取一致
	 * 若 SysTick 已经回卷但中断尚未来得及执行，则通过 PENDSTSET 做补偿
	 */
	primask = __get_PRIMASK();
	__disable_irq();

	tick_ms = ulTicks;
	current_value = SysTick->VAL;

	if((SCB->ICSR & SCB_ICSR_PENDSTSET) != 0U)
	{
		tick_ms++;
		current_value = SysTick->VAL;
	}

	if(primask == 0U)
	{
		__enable_irq();
	}

	tick_us = (uint64_t)tick_ms * 1000U;
	tick_us += ((uint64_t)(systick_reload_value - current_value) * 1000U) / systick_counts_per_ms;

	return tick_us;
}

/* 微秒级阻塞延时
 * 基于 GetUs() 的时间差做等待，避免目标时间点比较带来的边界问题
 */
void DelayUs(uint32_t us)
{
	Delay_Init();

	if(us == 0U)
	{
		return;
	}

	uint64_t start_us = GetUs();

	while((GetUs() - start_us) < us)
	{
	}
}

