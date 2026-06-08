/**
 ******************************************************************************
 * @file    si2c.c
 * @author  铁头山羊
 * @version V 1.0.0
 * @date    2022年9月3日
 * @brief   软件I2C驱动源文件
 ******************************************************************************
 *
 * @概述
 *   本文件实现了基于GPIO的软件模拟I2C协议，适用于STM32F103系列MCU。
 *   通过软件控制SCL和SDA引脚的高低电平来模拟I2C通信时序。
 *
 * @文件结构
 *   1. 宏定义：scl_w/sda_w/scl_r/sda_r - I2C引脚读写操作
 *   2. 延时函数：delay() - 微秒级延时
 *   3. 底层函数：
 *      - SendByte()    - 发送一个字节（MSB先发）
 *      - ReceiveByte() - 接收一个字节（MSB先收）
 *      - SendStop()    - 发送停止位
 *   4. 高层接口：
 *      - My_SI2C_Init()       - 初始化软件I2C
 *      - My_SI2C_SendBytes()  - 向从机写入多个字节
 *      - My_SI2C_ReceiveBytes() - 从从机读取多个字节
 *
 * @I2C协议要点
 *   - 通信速率：由delay()函数决定，标准模式100kHz，快速模式400kHz
 *   - 地址格式：7位地址左对齐，最低位为读写位（0=写，1=读）
 *   - 应答机制：每字节传输后从机发送ACK（低电平）表示应答
 *   - 时序要求：数据在SCL低电平期间变化，高电平期间保持稳定
 *
 * @使用示例
 *   // 初始化
 *   My_SI2C_Init(&myI2C);
 *
 *   // 写入数据
 *   uint8_t data[] = {0x01, 0x02, 0x03};
 *   My_SI2C_SendBytes(&myI2C, 0x68, data, 3);
 *
 *   // 读取数据
 *   uint8_t buf[2];
 *   My_SI2C_ReceiveBytes(&myI2C, 0x68, buf, 2);
 *
 ******************************************************************************
 */
#include "si2c.h"

// ============================================================================
// I2C引脚操作宏定义
// ============================================================================
//
// @宏 scl_w(v)：向SCL引脚写入电平
//   参数 v：1-写高电平，0-写低电平
//   说明：使用三目运算符将v转换为Bit_SET或Bit_RESET
//
// @宏 sda_w(v)：向SDA引脚写入电平
//   参数 v：1-写高电平，0-写低电平
//   说明：使用三目运算符将v转换为Bit_SET或Bit_RESET
//
// @宏 scl_r：读取SCL引脚电平
//   返回值：1-高电平，0-低电平
//   说明：读取GPIO输入数据寄存器，判断是否为Bit_SET
//
// @宏 sda_r：读取SDA引脚电平
//   返回值：1-高电平，0-低电平
//   说明：读取GPIO输入数据寄存器，判断是否为Bit_SET
//
// @注意：这些宏假设在函数内部已定义SI2C指针变量
//   调用这些宏的函数必须有名为SI2C的参数或变量
//
#define scl_w(v) GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, ((v) ? Bit_SET : Bit_RESET))
#define sda_w(v) GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, ((v) ? Bit_SET : Bit_RESET))

#define scl_r ((GPIO_ReadInputDataBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin) == Bit_SET) ? 1 : 0)
#define sda_r ((GPIO_ReadInputDataBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin) == Bit_SET) ? 1 : 0)

//
// @简介：简易软件延时函数
//
// @原理：
//   通过空循环消耗CPU周期来实现延时
//   基于STM32F103主频72MHz估算，每次循环约需9个时钟周期
//   因此循环8次约等于1微秒（实际取决于编译器优化等级）
//
// @参数 us：要延时的微秒数
//
// @注意事项：
//   - 此延时函数精度不高，受编译器优化和中断影响
//   - 对于I2C通信（100kHz/400kHz），此精度已足够
//   - 若系统时钟不是72MHz，需要调整循环次数
//   - 此延时会占用CPU时间，不适合长时间延时
//
void delay(uint32_t us)
{
    for (uint32_t i = 0; i < 8 * us; i++)
        ;
}

static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte);
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack);
static void SendStop(SI2C_TypeDef *SI2C);

//
// @简介：对软件I2C进行初始化
//
// @功能：
//   1. 使能SCL和SDA引脚所在GPIO端口的时钟
//   2. 将SCL和SDA引脚初始化为输出开漏模式(Open-Drain)
//   3. 初始状态将SCL和SDA拉高，确保总线空闲
//
// @参数 SI2C：指向软件I2C实例的指针，包含SCL和SDA的GPIO端口及引脚信息
//
// @注意事项：
//   - 开漏模式下，输出0时引脚被拉低，输出1时引脚处于高阻态（需外部上拉电阻）
//   - GPIO_Speed设为2MHz，足以满足I2C通信速率要求
//   - SDA引脚时钟使能在SCL之后，确保时序正确
//
__weak void My_SI2C_Init(SI2C_TypeDef *SI2C)
{
    // #1. 使能SCL引脚的时钟
    if (SI2C->SCL_GPIOx == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    }
    else if (SI2C->SCL_GPIOx == GPIOB)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
    else if (SI2C->SCL_GPIOx == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }
    else if (SI2C->SCL_GPIOx == GPIOD)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    }

    // #2. 对SCL和SDA写1
    GPIO_WriteBit(SI2C->SDA_GPIOx, SI2C->SDA_GPIO_Pin, Bit_SET);
    GPIO_WriteBit(SI2C->SCL_GPIOx, SI2C->SCL_GPIO_Pin, Bit_SET);

    // #2. 使能SDA引脚的时钟
    if (SI2C->SDA_GPIOx == GPIOA)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    }
    else if (SI2C->SDA_GPIOx == GPIOB)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    }
    else if (SI2C->SDA_GPIOx == GPIOC)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    }
    else if (SI2C->SDA_GPIOx == GPIOD)
    {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    }

    // #3. 初始化SCL引脚为输出开漏
    GPIO_InitTypeDef GPIO_InitStruct;

    GPIO_InitStruct.GPIO_Pin = SI2C->SCL_GPIO_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(SI2C->SCL_GPIOx, &GPIO_InitStruct);

    // #4. 初始化SDA引脚为输出开漏

    GPIO_InitStruct.GPIO_Pin = SI2C->SDA_GPIO_Pin;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(SI2C->SDA_GPIOx, &GPIO_InitStruct);
}

//
// @简介：通过软件I2C向从机写入多个字节
//
// @时序说明：
//   [S] [从机地址+W] [ACK] [数据1] [ACK] [数据2] [ACK] ... [数据n] [ACK] [P]
//   起始位  地址+写     应答    数据     应答    数据     应答         数据     应答    停止位
//
// @参数 SI2C：指向软件I2C实例的指针
// @参数 Addr：从机7位地址，左对齐（A6 A5 A4 A3 A2 A1 A0 0），最低位为读写位
// @param pData：要发送的数据缓冲区（数组指针）
// @param Size：要发送的数据字节数
//
// @返回值：
//   0  - 发送成功
//   -1 - 寻址失败（从机无应答）
//   -2 - 数据被拒收（从机在数据传输中发送NAK）
//
// @工作流程：
//   1. 先将SDA和SCL拉高，确保总线空闲
//   2. 发送起始位（SDA在SCL高电平时由高变低）
//   3. 发送从机地址+写位（最低位为0）
//   4. 等待从机应答，若无应答则发送停止位并返回-1
//   5. 逐字节发送数据，每字节后等待从机应答
//   6. 所有数据发送完毕后发送停止位
//
__weak int My_SI2C_SendBytes(SI2C_TypeDef *SI2C, uint8_t Addr, const uint8_t *pData, uint16_t Size)
{
    sda_w(1);
    scl_w(1);

    // #1. 发送起始位
    sda_w(0);
    delay(1);

    // #2. 发送从机地址+RW
    if (SendByte(SI2C, Addr & 0xfe) != 0)
    {
        // 1111 1110 0xFE 保证 最低位为0,表示写操作
        // SendByte  @返回值：0-ACK，其它-NAK
        SendStop(SI2C);
        return -1; // 寻址失败
    }

    // #3. 发送数据
    for (uint16_t i = 0; i < Size; i++)
    {
        if (SendByte(SI2C, pData[i]) != 0)
        {
            SendStop(SI2C);
            return -2; // 数据被拒收
        }
    }

    // #4. 发送停止位
    SendStop(SI2C);

    return 0;
}

//
// @简介：通过软件I2C从从机读多个字节
//
// @时序说明：
//   [S] [从机地址+R] [ACK] [数据1] [ACK] [数据2] [ACK] ... [数据n] [NAK] [P]
//   起始位  地址+读     应答    数据     应答    数据     应答         数据    无应答   停止位
//
// @参数 SI2C：指向软件I2C实例的指针
// @参数 Addr：从机7位地址，左对齐（A6 A5 A4 A3 A2 A1 A0 0），最低位为读写位
// @param pBuffer：接收数据的缓冲区（数组指针），读取的数据将存入此缓冲区
// @param Size：要读取的数据字节数
//
// @返回值：
//   0  - 接收成功
//   -1 - 寻址失败（从机无应答）
//
// @工作流程：
//   1. 先将SDA和SCL拉高，确保总线空闲
//   2. 发送起始位（SDA在SCL高电平时由高变低）
//   3. 发送从机地址+读位（最低位为1）
//   4. 等待从机应答，若无应答则发送停止位并返回-1
//   5. 逐字节接收数据，每字节后主机发送ACK（最后字节发NAK）
//   6. 所有数据接收完毕后发送停止位
//
// @注意：最后一个字节接收后主机发送NAK（无应答），通知从机停止发送
//
__weak int My_SI2C_ReceiveBytes(SI2C_TypeDef *SI2C, uint8_t Addr, uint8_t *pBuffer, uint16_t Size)
{
    sda_w(1);
    scl_w(1);

    // #1. 发送起始位
    sda_w(0);
    delay(1);

    // #2. 发送从机地址+RW
    if (SendByte(SI2C, Addr | 0x01) != 0)
    {
        SendStop(SI2C);
        return -1; // 寻址失败
    }

    // #3. 接收
    for (uint16_t i = 0; i < Size; i++)
    {
        pBuffer[i] = ReceiveByte(SI2C, (i == Size - 1) ? 1 : 0);
    }

    // #4. 发送停止位
    SendStop(SI2C);

    return 0;
}

//
// @简介：发送一个字节（MSB先发）
//
// @时序说明（以发送0xA5 = 10100101为例）：
//   SCL: _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_
//   SDA:  1   0   1   0   0   1   0   1   [释放]
//         ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
//        D7  D6  D5  D4  D3  D2  D1  D0  ACK
//
//   每个bit的传输过程：
//   1. SCL低电平期间：主机将数据位放到SDA上
//   2. SCL高电平期间：从机采样SDA上的数据
//   3. 第9个时钟周期：从机拉低SDA表示ACK，或保持高电平表示NAK
//
// @参数 SI2C：指向软件I2C实例的指针
// @参数 Byte：要发送的8位数据（MSB先发）
//
// @返回值：
//   0 - 从机应答(ACK)，表示从机成功接收数据
//   非0 - 从机无应答(NAK)，表示从机未接收或地址错误
//
// @内部调用关系：
//   被 My_SI2C_SendBytes() 和 My_SI2C_ReceiveBytes() 调用
//   调用 SendStop() 发送停止位（仅在NAK时）
//
static uint8_t SendByte(SI2C_TypeDef *SI2C, uint8_t Byte)
{
    // 发送8位数据，从最高位(MSB)到最低位(LSB)
    for (int8_t i = 7; i >= 0; i--)
    {
        // ① SCL低电平期间：主机准备数据
        scl_w(0);                            // 将SCL拉低，此时SDA可以变化
        sda_w((Byte & (0x01 << i)) ? 1 : 0); // 将第i位数据放到SDA线上
        delay(2);                            // 等待数据稳定（建立时间）

        // ② SCL高电平期间：从机采样数据
        scl_w(1); // 将SCL拉高，从机在此时读取SDA上的数据
        delay(2); // 等待从机完成采样（保持时间）
    }

    // ③ 第9个时钟周期：接收从机应答(ACK/NAK)+
    //    发送完8位后，主机释放SDA，由从机控制
    scl_w(0); // SCL拉低
    sda_w(1); // 主机释放SDA线（开漏模式下写1即释放总线）
    delay(2); // 等待数据稳定

    scl_w(1); // SCL拉高，此时从机应拉低SDA表示ACK
    delay(2); // 等待从机应答

    // 返回ACK状态：0=从机应答(ACK)，1=从机无应答(NAK)
    //
    // 示例1：从机正常应答 (返回0)
    //   场景：主机发送0xA5，从机成功接收并拉低SDA
    //   return sda_r = 0  → 表示ACK
    //
    // 示例2：从机无应答 (返回1)
    //   场景：从机地址错误或从机忙
    //   return sda_r = 1  → 表示NAK
    //
    return sda_r;
}

//
// @简介：发送I2C停止位(Stop Condition)
//
// @时序说明：
//   停止位的定义：在SCL为高电平时，SDA产生一个上升沿（由低变高）
//   时序图：
//     SCL: ___|‾‾‾‾‾|___
//     SDA: ___|_____↑‾‾‾|___
//                   ↑
//              停止位在此处产生
//
//   完整停止位产生过程：
//   1. 先将SCL拉低，准备停止条件
//   2. 将SDA拉低（为上升沿做准备）
//   3. 将SCL拉高
//   4. 将SDA拉高，产生停止条件
//
// @参数 SI2C：指向软件I2C实例的指针
//
// @注意事项：
//   - 停止位后总线进入空闲状态（SCL和SDA均为高电平）
//   - 停止位后必须等待一段时间才能发送下一个起始位
//   - 调用此函数前应确保SCL已被拉低
//
static void SendStop(SI2C_TypeDef *SI2C)
{
    scl_w(0); // scl拉低
    delay(1); // 延迟1/4周期
    sda_w(0); // sda拉低
    delay(1); // 延迟1/4周期
    scl_w(1); // scl拉高
    delay(1); // 延迟1/4周期
    sda_w(1); // sda拉高
    delay(1); // 延迟1/4周期
}

//
// @简介：从从机读取一个字节的数据（MSB先收）
//
// @时序说明（以接收0xA5 = 10100101为例）：
//   SCL: _|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_|‾|_
//   SDA:  1   0   1   0   0   1   0   1   [主机控制]
//         ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑   ↑
//        D7  D6  D5  D4  D3  D2  D1  D0  ACK/NAK
//
//   每个bit的接收过程：
//   1. SCL低电平期间：主机释放SDA（写1），从机将数据放到SDA上
//   2. SCL高电平期间：主机读取SDA上的数据
//   3. 第9个时钟周期：主机发送ACK（拉低SDA）或NAK（保持高电平）
//
// @参数 SI2C：指向软件I2C实例的指针
// @参数 Ack：应答控制位
//   0 - 回复NAK（无应答），通知从机停止发送
//   1 - 回复ACK（应答），通知从机继续发送下一个字节
//
// @返回值：接收到的8位数据（MSB先收）
//
// @内部调用关系：
//   被 My_SI2C_ReceiveBytes() 调用
//   调用 SendStop() 发送停止位（在所有字节接收完毕后）
//
static uint8_t ReceiveByte(SI2C_TypeDef *SI2C, uint8_t Ack)
{
    uint8_t ret = 0;

    for (int8_t i = 7; i >= 0; i--)
    {
        scl_w(0); // scl拉低
        sda_w(1); // 释放SDA
        delay(2); // 延迟1/2周期
        scl_w(1); // scl拉高
        delay(2); // 延迟1/2周期

        if (sda_r) // 如果读到的比特位为1
        {
            ret |= 0x01 << i; // 写入比特位   |（按位或）： 两个操作数的每一位，只要有一个是 1，结果就是 1。
        }
        else // 如果读到的比特位为0
        {
            // 什么也不干
        }
    }

    // 回复ACK或NAK

    scl_w(0); // scl拉低

    if (Ack)
    {
        sda_w(0); // sda拉低
    }
    else
    {
        sda_w(1); // sda拉高
    }

    delay(2); // 延迟1/2周期

    return ret;
}
