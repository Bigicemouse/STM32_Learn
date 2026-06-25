#include "delay.h"
#include "stm32f10x.h"
#include "usart.h"

#define ADC_FILTER_SIZE       8U
#define ADC_FILTER_MASK       (ADC_FILTER_SIZE - 1U)
#define LIGHT_DARK_ON_MV      1000U
#define LIGHT_BRIGHT_OFF_MV   1300U

static void App_GPIO_Init(void);
static void App_ADC_Init(void);
static void App_USART1_Init(void);

/**
 * @brief  主函数 - ADC 光照传感器实验
 *         PA0 采集光敏电阻模拟电压，转换后通过 USART1 打印，
 *         光照强时点亮 PC13 LED，光照弱时熄灭。
 */
int main(void)
{
    Delay_Init();
    App_GPIO_Init();
    App_ADC_Init();
    App_USART1_Init();

    My_USART_Printf(USART1, "ADC light sensor start\r\n");
    My_USART_Printf(USART1, "A0->PA0, USART1 TX->PA9 RX->PA10, LED->PC13\r\n");

    uint32_t adc_sum = 0;                   // 滑动窗口内的 ADC 累加值
    uint16_t adc_buf[ADC_FILTER_SIZE] = {0};// 环形缓冲区，保存最近几次采样
    uint8_t  buf_idx = 0;                   // 当前写入位置
    uint8_t  buf_cnt = 0;                   // 已填充的采样数
    uint8_t  print_cnt = 0;                 // 串口打印计数器
    uint8_t  led_is_on = 0;                 // PC13 LED 当前状态，1 表示亮

    while (1)
    {
        // ---- 单次 ADC 采样 ----
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        while (ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET)
        {
        }
        uint16_t dr = ADC_GetConversionValue(ADC1);

        // ---- 滑动平均滤波 ----
        adc_sum -= adc_buf[buf_idx];   // 减去最旧的值
        adc_buf[buf_idx] = dr;         // 写入新值
        adc_sum += dr;                 // 加上新值
        buf_idx = (buf_idx + 1) & ADC_FILTER_MASK;
        if (buf_cnt < ADC_FILTER_SIZE) buf_cnt++;
        uint16_t adc_avg = (uint16_t)(adc_sum / buf_cnt);
        uint32_t voltage_mv = (uint32_t)adc_avg * 3300U / 4095U;

        // ---- 光照判断：低于 1000mV 认为变暗，高于 1300mV 认为变亮，中间保持原状态 ----
        if (voltage_mv < LIGHT_DARK_ON_MV)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET); // 点亮
            led_is_on = 1;
        }
        else if (voltage_mv > LIGHT_BRIGHT_OFF_MV)
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);   // 熄灭
            led_is_on = 0;
        }

        // 每 10 次采样打印一次（约 1 秒一次）
        if (++print_cnt >= 10)
        {
            print_cnt = 0;
            My_USART_Printf(USART1,
                            "adc_avg=%u, voltage=%lu mV, led=%s\r\n",
                            (unsigned int)adc_avg,
                            (unsigned long)voltage_mv,
                            led_is_on ? "ON" : "OFF");
        }

        Delay(100);
    }
}

/**
 * @brief  GPIO 初始化
 *         - PC13：开漏输出，驱动 LED（低电平点亮）
 *         - PA0：模拟输入，接光敏电阻 ADC 采集
 *         - PA9：USART1 TX（复用推挽输出）
 *         - PA10：USART1 RX（浮空输入）
 */
static void App_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能 GPIOA、GPIOC 和 AFIO 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO, ENABLE);

    // PC13 - LED（开漏输出，默认高电平熄灭）
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &GPIO_InitStruct);
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET); // 初始熄灭

    // PA0 - ADC 通道 0 模拟输入
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA9 - USART1 TX
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PA10 - USART1 RX
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStruct);
}

/**
 * @brief  ADC1 初始化
 *         独立模式、单通道、软件触发、右对齐
 *         ADC 时钟 = PCLK2 / 6 = 72MHz / 6 = 12MHz（不超过 14MHz）
 */
static void App_ADC_Init(void)
{
    ADC_InitTypeDef ADC_InitStruct = {0};

    // ADC 时钟分频：PCLK2 / 6 = 12MHz
    RCC_ADCCLKConfig(RCC_PCLK2_Div6);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // ADC 基本配置
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent;                  // 独立模式
    ADC_InitStruct.ADC_ScanConvMode = DISABLE;                       // 非扫描模式
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;                 // 关闭连续转换
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;              // 数据右对齐
    ADC_InitStruct.ADC_NbrOfChannel = 1;                             // 常规序列,转换通道数 1
    ADC_Init(ADC1, &ADC_InitStruct);

    // 配置规则通道：ADC1 通道 0。光敏模块 A0 输出阻抗较高，采样时间取长一些更稳。
    ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_239Cycles5);

    // 使能 ADC1
    ADC_Cmd(ADC1, ENABLE);

    // ADC 校准（上电后必须执行一次）
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET)
    {
    }

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET)
    {
    }
}

/**
 * @brief  USART1 初始化 - 115200 波特率，8N1，收发模式
 */
static void App_USART1_Init(void)
{
    USART_InitTypeDef USART_InitStruct = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitStruct.USART_BaudRate = 115200;                                    // 波特率
    USART_InitStruct.USART_WordLength = USART_WordLength_8b;                     // 8 位数据
    USART_InitStruct.USART_StopBits = USART_StopBits_1;                          // 1 位停止位
    USART_InitStruct.USART_Parity = USART_Parity_No;                             // 无校验
    USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;                 // 收发模式
    USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 无流控
    USART_Init(USART1, &USART_InitStruct);

    USART_Cmd(USART1, ENABLE); // 使能 USART1
}
