/**
 ******************************************************************************
 * @file    i2c.c
 * @author  铁头山羊
 * @version V 1.0.0
 * @date    2024年9月3日
 * @brief   i2c驱动源文件
 ******************************************************************************
 */
#include "i2c.h"

//
// @简介：初始化指定的 I2C 外设
//
// @参数 I2Cx：I2C1 或 I2C2
//         I2C1 -> PB8(SCL)/PB9(SDA)（重映射）
//         I2C2 -> PB10(SCL)/PB11(SDA)（固定引脚）
//
void My_I2C_Init(I2C_TypeDef *I2Cx)
{
    uint16_t scl_pin, sda_pin;
    uint32_t apb1_periph;

    if (I2Cx == I2C1)
    {
        // 重映射 I2C1 到 PB8/PB9
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_I2C1, ENABLE);
        scl_pin = GPIO_Pin_8;
        sda_pin = GPIO_Pin_9;
        apb1_periph = RCC_APB1Periph_I2C1;
    }
    else
    {
        // I2C2 固定在 PB10/PB11，无需重映射
        scl_pin = GPIO_Pin_10;
        sda_pin = GPIO_Pin_11;
        apb1_periph = RCC_APB1Periph_I2C2;
    }

    // 配置 SCL/SDA 为复用开漏，2MHz
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_InitStruct.GPIO_Pin = scl_pin | sda_pin;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 使能 I2C 外设时钟并复位
    RCC_APB1PeriphClockCmd(apb1_periph, ENABLE);
    RCC_APB1PeriphResetCmd(apb1_periph, ENABLE);
    RCC_APB1PeriphResetCmd(apb1_periph, DISABLE);

    // 400kHz 标准 I2C 模式
    I2C_InitTypeDef I2C_InitStruct = {0};
    I2C_InitStruct.I2C_ClockSpeed = 400000;
    I2C_InitStruct.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStruct.I2C_DutyCycle = I2C_DutyCycle_2;

    I2C_Init(I2Cx, &I2C_InitStruct);
    I2C_Cmd(I2Cx, ENABLE);
}

//
// @简介：通过I2C向从机写入多个字节
//
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pData：要发送的数据（数组）
// @参数 Size：要发送的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败， -2 - 数据被拒收
//
__weak int My_I2C_SendBytes(I2C_TypeDef *I2Cx, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
    // #1. 等待总线空闲
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == SET)
        ;

    // #2. 发送起始位
    I2C_GenerateSTART(I2Cx, ENABLE);

    // 确定起始位已经发送成功，Start bit 置位后才能继续发送地址，否则会导致总线错误（AF 置位）
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_SB) == RESET)
        ;

    // #3. 寻址阶段
    // 先清除AF，防止之前的错误影响当前操作
    // Acknowledge failure  1失败0成功
    I2C_ClearFlag(I2Cx, I2C_FLAG_AF);

    // 0xfe的二进制表示1111 1110  与&Addr 保证了最后一位清零（即保证最后的读写位为写）
    // Address sent    1成功应答
    I2C_SendData(I2Cx, Addr & 0xfe);

    // 等待从机应答，ADDR=1表示从机已应答，AF=1表示从机NACK
    while (1)
    {
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR) == SET)
        {
            break; // 从机应答成功
        }
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
        // Acknowledge failure=1表示从机NACK
        {
            I2C_GenerateSTOP(I2Cx, ENABLE); // 发送停止位释放总线
            return -1;                      // 寻址失败
        }
    }

    // 清除ADDR：ADDR置位后硬件会自动拉低SCL（时钟延展），总线暂停
    // 必须读SR1+SR2才能清除ADDR，否则SCL一直被钳住，总线卡死无法继续
    I2C_ReadRegister(I2Cx, I2C_Register_SR1);
    I2C_ReadRegister(I2Cx, I2C_Register_SR2);

    // #4. 发送数据
    for (uint16_t i = 0; i < Size; i++)
    {
        while (1)
        {
            if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
            {
                I2C_GenerateSTOP(I2Cx, ENABLE);
                return -2; // AF=1， 数据被拒收
            }
            if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_TXE) == SET)
            {
                break; // 数据寄存器空，可以发送下一个字节了
            }
        }

        I2C_SendData(I2Cx, pData[i]); // 发送数据
    }
    // 5 完成与释放总线
    while (1)
    {
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
        {
            I2C_GenerateSTOP(I2Cx, ENABLE);
            return -2; // AF=1， 数据被拒收
        }

        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BTF) == SET)
        {
            break; // BTF=1，表示数据寄存器空且数据已经被发送出去了，可以发送停止位了
                   // Byte transfer finished
        }
    }

    //  发送停止位
    I2C_GenerateSTOP(I2Cx, ENABLE);
    return 0; // 成功
}

//
// @简介：通过I2C从从机读多个字节
//
// @参数 I2Cx：填写要操作的I2C的名称，可以是I2C1或I2C2
// @参数 Addr：填写从机的地址，左对齐 - A6 A5 A4 A3 A2 A1 A0 0
// @参数 pBuffer：接收缓冲区（数组）
// @参数 Size：要读取的数据的数量，以字节为单位
//
// @返回值：0 - 发送成功， -1 - 寻址失败
//
__weak int My_I2C_ReceiveBytes(I2C_TypeDef *I2Cx, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
    if (Size == 0)
        return 0;

    // #1. 等待总线空闲
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_BUSY) == SET)
        ;

    // #2. 发送起始位
    I2C_GenerateSTART(I2Cx, ENABLE);

    // 等待起始位发送成功
    while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_SB) == RESET)
        ;

    // #3. 寻址阶段
    // 清除之前的 AF 标志
    I2C_ClearFlag(I2Cx, I2C_FLAG_AF);

    // 发送从机地址，最低位=1 表示读操作
    I2C_SendData(I2Cx, Addr | 0x01);

    // 等待从机应答，ADDR=1 表示从机已应答，AF=1 表示从机 NACK
    while (1)
    {
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_ADDR) == SET)
        {
            break; // 从机应答成功
        }
        if (I2C_GetFlagStatus(I2Cx, I2C_FLAG_AF) == SET)
        {
            I2C_GenerateSTOP(I2Cx, ENABLE); // 发送停止位释放总线
            return -1;                      // 寻址失败
        }
    }

    // #4. 数据读取
    if (Size == 1)
    {
        // ---- 只读 1 字节的特殊情况 ----
        // 必须在清除 ADDR 之前先发 NACK 和 STOP，否则从机会继续发送数据
        // 先写 NACK：告诉从机这是最后一个字节，读完不再应答
        I2C_AcknowledgeConfig(I2Cx, DISABLE);

        // 清除 ADDR（读 SR1+SR2 是硬件要求的清除流程）
        // 关中断防止在清除 ADDR 后、发 STOP 前被中断打断导致时序错乱
        __disable_irq();
        I2C_ReadRegister(I2Cx, I2C_Register_SR1);
        I2C_ReadRegister(I2Cx, I2C_Register_SR2);

        // 立即发送停止位，释放总线
        I2C_GenerateSTOP(I2Cx, ENABLE);
        __enable_irq();

        // 等待数据到达（RxNE=1）
        while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET)
            ;
        // 读取最后一个字节
        pBuffer[0] = I2C_ReceiveData(I2Cx);
    }
    else
    {
        // ---- 读多个字节 ----
        // 先写 ACK：告诉从机后续还有数据要读
        I2C_AcknowledgeConfig(I2Cx, ENABLE);

        // 清除 ADDR
        I2C_ReadRegister(I2Cx, I2C_Register_SR1);
        I2C_ReadRegister(I2Cx, I2C_Register_SR2);

        // 逐字节读取前 N-1 个字节（每读一个就 ACK）
        for (uint16_t i = 0; i < Size - 1; i++)
        {
            // 读到最后第 2 个字节时关中断，确保最后的 NACK+STOP 时序紧凑
            if (i == Size - 2)
            {
                __disable_irq();
            }
            // 等待接收缓冲区有数据（RxNE=1）
            while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET)
                ;
            // 读取数据
            pBuffer[i] = I2C_ReceiveData(I2Cx);
        }

        // 最后一个字节前发 NACK，告诉从机不再接收
        I2C_AcknowledgeConfig(I2Cx, DISABLE);
        // 发送停止位，释放总线
        I2C_GenerateSTOP(I2Cx, ENABLE);

        __enable_irq();

        // 等待最后一个字节到达（RxNE=1）
        while (I2C_GetFlagStatus(I2Cx, I2C_FLAG_RXNE) == RESET)
            ;
        // 读取最后一个字节
        pBuffer[Size - 1] = I2C_ReceiveData(I2Cx);
    }

    return 0;
}
