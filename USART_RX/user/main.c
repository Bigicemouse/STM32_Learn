#include "Delay.h"
#include "stm32f10x.h"

/* PC13 LED GPIO 初始化 */
void my_GPIO_Init(void);
/* USART1 初始化，使用重映射后的 PB6/PB7 */
void my_USART_Init(void);

int main(void)
{
    /* 初始化 LED 控制引脚 */
    my_GPIO_Init();
    /* 初始化串口 1，用于接收上位机发送的数据 */
    my_USART_Init();
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET); // 点亮 LED，表示系统已启动
    Delay(1000);
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET); // 熄灭 LED，进入主循环等待串口数据

    while (1)
    {
        /* 轮询等待接收数据寄存器非空 */
        while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);  
        //接收数据寄存器空吗---不空---有数据正在传输---进入循环等待   

        /* 读取 1 字节接收数据 */
        uint8_t receiveData = USART_ReceiveData(USART1);

        /* 兼容文本模式字符 '1'/'0' 与原始字节 0x01/0x00 */
        if ((receiveData == (uint8_t)'1') || (receiveData == (uint8_t)0x01))
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_RESET);
        }
        else if ((receiveData == (uint8_t)'0') || (receiveData == (uint8_t)0x00))
        {
            GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
        }
        else if ((receiveData == (uint8_t)'\r') || (receiveData == (uint8_t)'\n'))
        {
            /* 忽略串口工具可能附带的行尾字符 */
        }
    }
}

/* PC13 配置为开漏输出，用于控制板载 LED */
void my_GPIO_Init()
{
    /* 使能 GPIOC 外设时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Pin = GPIO_Pin_13;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 默认输出高电平，LED 熄灭 */
    GPIO_WriteBit(GPIOC, GPIO_Pin_13, Bit_SET);
}

/* USART1 使用部分重映射：
 * TX -> PB6
 * RX -> PB7
 */
void my_USART_Init()
{
    /* -------- 第一步：使能 AFIO 时钟并开启引脚重映射 --------
     * USART1 默认引脚为 PA9(TX) / PA10(RX)，但本板硬件连线使用
     * PB6/PB7，因此必须开启 AFIO 时钟并执行部分重映射，
     * 将 USART1 的 TX/RX 从 PA9/PA10 切换到 PB6/PB7 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_USART1, ENABLE);

    /* -------- 第二步：配置 GPIOB 引脚 --------
     * USART1 重映射后，TX/RX 位于 GPIOB 端口，需要先使能 GPIOB 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* 配置 PB6 为 USART1_TX：复用推挽输出，速度 10MHz
     * - GPIO_Mode_AF_PP（复用推挽）：引脚由 USART 外设控制输出
     * - 速度 10MHz 足以满足 115200 波特率的信号边沿速率要求 */
    GPIO_InitTypeDef gpio_initStruct2 = {0};
    gpio_initStruct2.GPIO_Pin = GPIO_Pin_6;
    gpio_initStruct2.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_initStruct2.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOB, &gpio_initStruct2);

    /* 配置 PB7 为 USART1_RX：上拉输入
     * - GPIO_Mode_IPU（上拉输入）：空闲时 RX 线保持高电平，
     *   防止浮空引脚产生随机起始位导致误触发 */
    gpio_initStruct2.GPIO_Pin = GPIO_Pin_7;
    gpio_initStruct2.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio_initStruct2);

    /* -------- 第三步：配置 USART1 外设通信参数 --------
     * - USART1 挂载在 APB2 总线上，必须使能 APB2 外设时钟
     * - 波特率 115200，与上位机串口工具保持一致
     * - 数据格式：8 位数据位，1 位停止位，无校验（常用 8N1 格式）
     * - 同时启用收发模式，使同一串口既能发送也能接收 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    USART_InitTypeDef unit_initStruct = {0};
    unit_initStruct.USART_BaudRate = 115200;
    unit_initStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    unit_initStruct.USART_WordLength = USART_WordLength_8b;
    unit_initStruct.USART_StopBits = USART_StopBits_1;
    unit_initStruct.USART_Parity = USART_Parity_No;
    USART_Init(USART1, &unit_initStruct);

    /* 最后使能 USART1 外设，配置立即生效，串口可以开始收发数据 */
    USART_Cmd(USART1, ENABLE);
}
