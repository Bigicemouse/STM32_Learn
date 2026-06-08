/**
 * @file    main.c
 * @brief   SPI 主机全双工收发示例（SPI1 重映射引脚）
 * @note    SPI1 引脚重映射：SCK=PB3, MISO=PB4, MOSI=PB5, NSS=PA15
 *          模式：CPOL=0, CPHA=0, MSB 先行，软件 NSS
 */

#include "Delay.h"
#include "stm32f10x.h"

/* 函数声明 */
void My_SPI_IOInit(void);
void App_SPI_MasterTransmitReceive(SPI_TypeDef *SPIx, const uint8_t *pDataTx, uint8_t *pDataRx, uint16_t Size);

/**
 * @brief  主函数
 * @note   初始化 SPI1（重映射引脚），发送 4 字节数据并接收应答
 */
int main(void)
{
    /* 初始化 SPI1 引脚和外设 */
    My_SPI_IOInit();

    /* 发送缓冲区：4 字节测试数据 */
    uint8_t pDataTx[] = {0, 1, 2, 3};
    /* 接收缓冲区 */
    uint8_t pDataRx[100];

    /* SPI 全双工收发：发送 4 字节，同时接收 4 字节 */
    App_SPI_MasterTransmitReceive(SPI1, pDataTx, pDataRx, 4);

    while (1)
    {
    }
}

/**
 * @brief  初始化 SPI1 引脚（重映射）和外设配置
 * @note   SPI1 默认引脚：PA5(SCK), PA6(MISO), PA7(MOSI), PA4(NSS)
 *         重映射后：PB3(SCK), PB4(MISO), PB5(MOSI), PA15(NSS)
 *         PA15 原为 JTDI，需禁用 JTAG 复用才能用作普通 GPIO
 */
void My_SPI_IOInit(void)
{
    /* ========== 第一步：配置引脚重映射 ========== */

    /* 开启 AFIO 时钟（重映射必须） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    /* SPI1 引脚重映射：SCK→PB3, MISO→PB4, MOSI→PB5 */
    GPIO_PinRemapConfig(GPIO_Remap_SPI1, ENABLE);

    /* 禁用 JTAG，释放 PA15、PB3、PB4 用作普通 GPIO（仅保留 SWD） */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

    /* ========== 第二步：初始化 GPIO 引脚 ========== */

    /* --- SCK(PB3) 和 MOSI(PB5)：复用推挽输出 --- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct1 = {0};
    GPIO_InitStruct1.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5;   /* SCK + MOSI */
    GPIO_InitStruct1.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct1.GPIO_Mode = GPIO_Mode_AF_PP;           /* 复用推挽 */
    GPIO_Init(GPIOB, &GPIO_InitStruct1);

    /* --- NSS(PA15)：软件控制，普通推挽输出 --- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct2 = {0};
    GPIO_InitStruct2.GPIO_Pin = GPIO_Pin_15;                /* NSS */
    GPIO_InitStruct2.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStruct2.GPIO_Mode = GPIO_Mode_Out_PP;          /* 普通推挽 */
    GPIO_Init(GPIOA, &GPIO_InitStruct2);

    /* NSS 默认高电平（未选中从机），注意：软件 NSS 需手动拉低选中 */
    GPIO_WriteBit(GPIOA, GPIO_Pin_15, Bit_SET);

    /* --- MISO(PB4)：上拉输入 --- */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct3 = {0};
    GPIO_InitStruct3.GPIO_Pin = GPIO_Pin_4;                 /* MISO */
    GPIO_InitStruct3.GPIO_Mode = GPIO_Mode_IPU;             /* 上拉输入 */
    GPIO_Init(GPIOB, &GPIO_InitStruct3);

    /* ========== 第三步：初始化 SPI1 外设 ========== */

    /* 开启 SPI1 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_InitTypeDef SPI_InitStruct = {0};
    SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex; /* 全双工 */
    SPI_InitStruct.SPI_Mode = SPI_Mode_Master;                      /* 主机模式 */
    SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;                  /* 8 位数据 */
    SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;                        /* 空闲时 SCK 低电平 */
    SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;                      /* 第一跳变沿采样 */
    SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;                 /* MSB 先行 */
    SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_64; /* 分频 64 */
    SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;                          /* 软件管理 NSS */

    SPI_Init(SPI1, &SPI_InitStruct);

    /* 软件 NSS 置高（内部 SSI=1），避免 MODF 错误 */
    SPI_NSSInternalSoftwareConfig(SPI1, SPI_NSSInternalSoft_Set);
}

/**
 * @brief  SPI 主机全双工收发函数
 * @param  SPIx    : SPI 外设指针（SPI1 或 SPI2）
 * @param  pDataTx : 发送数据缓冲区
 * @param  pDataRx : 接收数据缓冲区
 * @param  Size    : 数据长度（字节）
 * @note   利用 SPI 全双工特性：每发一个字节，同时收到一个字节
 *         发送 N 字节会收到 N 字节（来自从机的 MISO 数据）
 *         时序：先发第 0 字节 → 等 TXE → 发下一字节 → 等 RXNE → 读上一字节
 */
void App_SPI_MasterTransmitReceive(SPI_TypeDef *SPIx, const uint8_t *pDataTx, uint8_t *pDataRx, uint16_t Size)
{
    /* 使能 SPI 外设 */
    SPI_Cmd(SPIx, ENABLE);

    /* 发送第 0 字节（此时还没有数据可读，RXNE 不会置位） */
    SPI_I2S_SendData(SPIx, pDataTx[0]);

    /* 循环发送剩余 Size-1 字节，同时读取前一字节的接收数据 */
    for (int i = 0; i < Size - 1; i++)
    {
        /* 等待发送缓冲区空（TXE=1 表示可以写入下一字节） */
        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_TXE) == RESET)
            ;

        /* 写入下一字节（触发新的时钟周期，同时从机数据移入接收寄存器） */
        SPI_I2S_SendData(SPIx, pDataTx[i + 1]);

        /* 等待接收缓冲区非空（RXNE=1 表示上一字节已收到） */
        while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET)
            ;

        /* 读取接收到的字节 */
        pDataRx[i] = SPI_I2S_ReceiveData(SPIx);
    }

    /* 等待最后一字节接收完成 */
    while (SPI_I2S_GetFlagStatus(SPIx, SPI_I2S_FLAG_RXNE) == RESET)
        ;

    /* 读取最后一字节 */
    pDataRx[Size - 1] = SPI_I2S_ReceiveData(SPIx);

    /* 禁用 SPI 外设 */
    SPI_Cmd(SPIx, DISABLE);
}
