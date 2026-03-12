/*
 * ========================================================
 * 串口 printf 输出示例程序
 * 功能：配置 USART1 串口通信，实现 printf 函数输出到串口
 * 原理：通过重定向 fputc 函数将标准输出重新定向到串口
 * ========================================================
 */

#include "Delay.h"     // 延时函数库
#include "stm32f10x.h" // STM32F10x 标准外设库头文件
#include "usart.h"     // 串口驱动库
#include <stdio.h>     // C 标准库：包含 printf() 和 fputc() 接口

/* ========================================================
 * 函数声明：通用 USART 初始化函数
 * ======================================================== */
void my_USART_Init(USART_TypeDef *USARTx, uint32_t BaudRate, GPIO_TypeDef *GPIOx,
                   uint16_t TX_Pin, uint16_t RX_Pin, uint32_t GPIO_Remap);

/* ========================================================
 * 主函数：程序入口
 * ======================================================== */
int main(void)
{
    /* ---- 初始化 USART1 串口 ---- */
    /* 参数解释：
     *   USART1                 - 使用 USART1 外设
     *   115200                 - 波特率：115200 bps
     *   GPIOB                  - 使用 GPIO 端口 B
     *   GPIO_Pin_6             - TX 发送引脚（PB6）
     *   GPIO_Pin_7             - RX 接收引脚（PB7）
     *   GPIO_Remap_USART1      - 启用 USART1 的引脚重映射配置
     */
    my_USART_Init(USART1, 115200, GPIOB, GPIO_Pin_6, GPIO_Pin_7, GPIO_Remap_USART1);

    /* ==================== 发送测试数据 ==================== */

    /* 以下注释掉的函数在当前库中未实现，可选用 */
    // My_USART_SendString(USART1, "hello");               // 发送字符串（库函数）
    // My_USART_Printf(USART1, "Temperature: %d\r\n", 25); // 发送格式化字符串（库函数）

    /* 使用标准 C 库的 printf 函数，通过 fputc() 重定向到串口发送 */
    printf("hello\r\n");               // 发送 "hello" 加回车换行
    printf("Temperature: %d\r\n", 25); // 发送格式化字符串：温度值为 25

    /* ==================== 主循环 ==================== */
    while (1)
    {
        /* 程序在此循环等待，保持设备运行 */
        /* 实际应用中这里可以添加按键检测、传感器读取等操作 */
    }

    return 0; // 一般不会执行到此，因为 while(1) 会一直循环
}

/* ========================================================
 * @brief  USART 通用初始化函数（全局使用）
 *
 * 功能：完整配置一个 USART 外设，包括时钟、GPIO、引脚重映射、
 *       波特率、数据格式等，一次调用即可完全初始化串口。
 *
 * @param  USARTx        - 指定要初始化的 USART 外设
 *                        (USART1/USART2/USART3 等)
 * @param  BaudRate      - 波特率，单位 bps
 *                        常用值：9600, 19200, 38400, 115200 等
 * @param  GPIOx         - USART 的 GPIO 端口
 *                        (GPIOA/GPIOB/GPIOC/GPIOD/GPIOE)
 * @param  TX_Pin        - 发送引脚（GPIO_Pin_0 ~ GPIO_Pin_15）
 * @param  RX_Pin        - 接收引脚（GPIO_Pin_0 ~ GPIO_Pin_15）
 * @param  GPIO_Remap    - GPIO 引脚重映射配置
 *                        有重映射时填 GPIO_Remap_USART1 等
 *                        无重映射时填 0
 *
 * @return 无返回值
 *
 * @note   标准配置：
 *         - 数据位：8 位
 *         - 停止位：1 位
 *         - 奇偶校验：无
 *         - 收发模式：同时支持接收和发送
 *
 * 使用示例：
 *   // USART1 使用重映射（PB6-TX, PB7-RX）
 *   my_USART_Init(USART1, 115200, GPIOB, GPIO_Pin_6, GPIO_Pin_7, GPIO_Remap_USART1);
 *
 *   // USART2 不使用重映射（PA2-TX, PA3-RX）
 *   my_USART_Init(USART2, 115200, GPIOA, GPIO_Pin_2, GPIO_Pin_3, 0);
 * ======================================================== */
void my_USART_Init(USART_TypeDef *USARTx, uint32_t BaudRate, GPIO_TypeDef *GPIOx,
                   uint16_t TX_Pin, uint16_t RX_Pin, uint32_t GPIO_Remap)
{
    /* ================ 第 1 阶段：GPIO 与 AFIO 时钟和重映射配置 ================ */

    /* 步骤 1.1：开启 AFIO 时钟
     * 说明：在使用 GPIO 引脚重映射（Remap）功能时必需开启 AFIO 时钟
     *       AFIO 是 Alternate Function I/O（替代功能 I/O），控制引脚的复用功能
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* 步骤 1.2：条件执行引脚重映射
     * 说明：如果 GPIO_Remap 不为 0，则执行重映射
     *       重映射可将串口 TX/RX 迁移到不同的引脚上，灵活设计 PCB
     */
    if (GPIO_Remap != 0)
    {
        GPIO_PinRemapConfig(GPIO_Remap, ENABLE);
    }

    /* 步骤 1.3：根据不同的 GPIO 端口开启对应时钟
     * 说明：在配置 GPIO 前必须开启其时钟，STM32 使用此机制节省功耗
     *       每个 GPIO 端口都有独立的时钟控制位
     */
    if (GPIOx == GPIOA)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    else if (GPIOx == GPIOB)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    else if (GPIOx == GPIOC)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    else if (GPIOx == GPIOD)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    else if (GPIOx == GPIOE)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

    /* 步骤 1.4：初始化 GPIO 配置结构体（将其清零）
     * 说明：gpio_initStruct 用于存储 GPIO 的配置参数，先清零防止垃圾数据
     */
    GPIO_InitTypeDef gpio_initStruct = {0};

    /* ================ 第 2 阶段：配置 TX（发送）引脚 ================ */
    /* 步骤 2.1：指定 TX 引脚编号 */
    gpio_initStruct.GPIO_Pin = TX_Pin;

    /* 步骤 2.2：设置 TX 引脚模式为复用推挽输出
     * GPIO_Mode_AF_PP = Alternate Function Push-Pull（复用推挽）
     * 说明：
     *   - 复用（AF）：由外设（USART）驱动，而不是 GPIO 软件
     *   - 推挽（PP）：能主动拉高拉低电压，适合输出驱动
     *   - 这是 UART 发送线的标准配置
     */
    gpio_initStruct.GPIO_Mode = GPIO_Mode_AF_PP;

    /* 步骤 2.3：设置 TX 引脚输出速率为 10MHz
     * 说明：设置 GPIO 的切换速度，10MHz 对于串口通信足够
     *       其他选项有 2MHz（低功耗）、50MHz（高速）
     */
    gpio_initStruct.GPIO_Speed = GPIO_Speed_10MHz;

    /* 步骤 2.4：将配置应用到 TX 引脚 */
    GPIO_Init(GPIOx, &gpio_initStruct);

    /* ================ 第 3 阶段：配置 RX（接收）引脚 ================ */
    /* 步骤 3.1：指定 RX 引脚编号 */
    gpio_initStruct.GPIO_Pin = RX_Pin;

    /* 步骤 3.2：设置 RX 引脚模式为上拉输入
     * GPIO_Mode_IPU = Input with Pull-Up（上拉输入）
     * 说明：
     *   - 输入（I）：接收外部信号，高阻抗态
     *   - 上拉（PU）：内部上拉电阻让引脚趋向高电平
     *   - UART 空闲时为高电平，所以需要上拉保证电平稳定
     */
    gpio_initStruct.GPIO_Mode = GPIO_Mode_IPU;

    /* 步骤 3.3：将配置应用到 RX 引脚
     * 注意：GPIO_Speed 对输入模式无影响，可不设置
     */
    GPIO_Init(GPIOx, &gpio_initStruct);

    /* ================ 第 4 阶段：USART 外设时钟配置 ================ */
    /* 步骤 4.1：根据不同的 USART 开启对应的时钟
     * 说明：
     *   - USART1 挂在 APB2 总线上（高速总线）
     *   - USART2/USART3 挂在 APB1 总线上（低速总线）
     *   - STM32F10x 的时钟树决定了这个分配
     */
    if (USARTx == USART1)
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    else if (USARTx == USART2)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    else if (USARTx == USART3)
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    /* ================ 第 5 阶段：USART 参数配置 ================ */
    /* 步骤 5.1：初始化 USART 配置结构体 */
    USART_InitTypeDef unit_initStruct = {0};

    /* 步骤 5.2：设置波特率
     * 说明：波特率是串口通信的速度，单位 bps（比特/秒）
     *       常见: 9600, 115200 等
     *       波特率过高会增加误码率；过低会浪费时间
     */
    unit_initStruct.USART_BaudRate = BaudRate;

    /* 步骤 5.3：同时启用接收和发送模式
     * USART_Mode_Rx | USART_Mode_Tx = 既可接收也可发送
     * 说明：
     *   - Rx（接收）：监听 RX 线，接收别人的数据
     *   - Tx（发送）：驱动 TX 线，向外发送数据
     *   - 使用 | 运算符同时启用两个模式（全双工通信）
     */
    unit_initStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    /* 步骤 5.4：设置数据帧格式
     * 说明：定义每次发送/接收的数据位数和校验方式
     */

    /* 数据位：8 位
     * 说明：每个字符由 8 个数据位组成，可表示 0~255 的值
     *       8 位是工业标准，支持完整的 ASCII 字符集
     */
    unit_initStruct.USART_WordLength = USART_WordLength_8b;

    /* 停止位：1 位
     * 说明：在 8 个数据位之后插入 1 个停止位，标记字符的结束
     *       停止位让接收器有时间处理数据，为下一个字符做准备
     *       也可选择 2 位停止位（USART_StopBits_2）
     */
    unit_initStruct.USART_StopBits = USART_StopBits_1;

    /* 奇偶校验：无
     * 说明：不使用奇偶校验位，因为串口线短距离传输噪声小
     *       如需可靠性可选 Odd（奇数）或 Even（偶数）校验
     */
    unit_initStruct.USART_Parity = USART_Parity_No;

    /* 步骤 5.5：将参数配置应用到 USART 寄存器
     * 说明：USART_Init 函数更新 USART 的波特率生成器和控制寄存器
     */
    USART_Init(USARTx, &unit_initStruct);

    /* ================ 第 6 阶段：使能串口 ================ */
    /* 步骤 6.1：启用 USART 外设
     * 说明：虽然已配置参数，但必须显式使能才能工作
     *       USART_Cmd(..., ENABLE) 设置 USART_CR1 的 UE 位
     */
    USART_Cmd(USARTx, ENABLE);

    /* 初始化完成，此时 USART 已完全可用 */
}

/* ================================================================
 * 函数名称: fputc（File PUT Character）
 *
 * 功能描述：
 *   重定向 C 标准库的文件 I/O 到串口
 *   当程序调用 printf() 时，标准库会逐个字符地调用 fputc()
 *   通过重定义此函数，我们可以将输出重定向到串口而非默认的调试器
 *
 * 工作原理：
 *   1. printf("format", args) 在内部处理格式字符串和参数
 *   2. 对于每个要输出的字符，printf 调用 fputc(ch, stdout)
 *   3. 我们的 fputc 函数将字符通过 USART_SendData 发送到串口
 *   4. 接收端（如串口助手）读取发送的数据并显示
 *
 * @param  ch - 待发送字符的 ASCII 值（int 类型）
 *             范围：0~127 用于标准 ASCII，0~255 用于扩展 ASCII
 *
 * @param  f  - FILE 指针（标准库中的抽象文件指针）
 *            此参数通常为 stdout（标准输出），但在此也可忽略
 *            函数收到此参数是为了兼容其他输出流
 *
 * @return ch - 返回发送的字符值
 *            成功时返回字符本身；某些错误情况下返回 EOF(-1)
 *            此处总是返回 ch，表示发送成功
 *
 * @note
 *   1. 此函数由标准库自动调用，程序员无需手动调用
 *   2. 必须包含 <stdio.h> 头文件，否则链接器找不到 fputc
 *   3. 函数中使用 USART1（硬编码），若需多串口切换需改为全局变量
 *   4. 等待标志位时会阻塞，若没有硬件可能卡死
 *
 * 执行流程图：
 *   printf("hello")
 *     ↓
 *   printf 内部逐字符调用 fputc
 *     ↓
 *   fputc('h', stdout) 被调用
 *     ↓
 *   等待 TXE 标志（发送数据寄存器空）
 *     ↓
 *   USART_SendData(USART1, 'h') 发送字符
 *     ↓
 *   等待 TC 标志（发送完成）
 *     ↓
 *   返回 'h'，printf 继续发送下一个字符...
 * ================================================================ */
int fputc(int ch, FILE *f)
{
    volatile uint32_t timeout;
    (void)f;

    /* 将 LF 转为 CRLF，适配大多数串口终端显示 */
    if (ch == '\n')
    {
        timeout = 0x1FFFF;
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
        {
            if (--timeout == 0)
                return EOF;
        }
        USART_SendData(USART1, (uint16_t)'\r');
    }

    /* 等待发送数据寄存器空，再写入数据；加超时防止硬件异常卡死 */
    timeout = 0x1FFFF;
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET)
    {
        if (--timeout == 0)
            return EOF;
    }

    USART_SendData(USART1, (uint16_t)(uint8_t)ch);

    /* 不必每个字符都等待 TC，提升 printf 输出效率 */
    return ch;
}