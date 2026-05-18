/**
  ******************************************************************************
  * @file    delay.h
  * @author  铁头山羊
  * @version V 1.0.0
  * @date    2022年8月30日
  * @brief   延迟函数头文件
  ******************************************************************************
  */

#ifndef _DELAY_H_
#define _DELAY_H_

#include "stm32f10x.h"

/* 初始化延时模块
 * 输入：无
 * 输出：无
 */
void Delay_Init(void);

/* 毫秒延时
 * 输入：ms - 延时时间，单位 ms
 * 输出：无
 */
void Delay(uint32_t ms);

/* 获取当前毫秒计时
 * 输入：无
 * 输出：当前毫秒数
 */
uint32_t GetTick(void);

/* 获取当前微秒计时
 * 输入：无
 * 输出：当前微秒数
 */
uint64_t GetUs(void);

/* 微秒延时
 * 输入：us - 延时时间，单位 us
 * 输出：无
 */
void DelayUs(uint32_t us);

#endif
