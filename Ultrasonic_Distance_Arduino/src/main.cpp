/**
 * @file    main.cpp
 * @brief   HC-SR04 超声波测距 (Arduino UNO)
 * @note    兼容 STM32 版本的测距逻辑，通过串口 115200 输出距离(cm)
 *          接线: VCC→5V, GND→GND, TRIG→D9, ECHO→D10
 */

#include <Arduino.h>

#define TRIG_PIN 9
#define ECHO_PIN 10
#define TIMEOUT_US 30000UL  // 30ms 超时，与 STM32 版本一致
#define MEASURE_INTERVAL 2000 // 测量间隔 2s

void setup()
{
    Serial.begin(115200);
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
}

void loop()
{
    // 发送 10us 触发脉冲
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // 测量 Echo 高电平持续时间(us)
    unsigned long pulse_width = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);

    if (pulse_width == 0)
    {
        // 超时，无回波
        Serial.println("timeout: no echo");
    }
    else
    {
        // 声速 340m/s，除以2得单程距离，乘 1e-6 转秒，乘 100 转 cm
        float distance_cm = pulse_width * 1.0e-6f * 340.0f / 2.0f * 100.0f;
        Serial.print(distance_cm, 3);
        Serial.println(" cm");
    }

    delay(MEASURE_INTERVAL);
}
