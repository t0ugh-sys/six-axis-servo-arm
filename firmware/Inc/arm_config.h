#pragma once

// 你需要根据实际机械结构与标定结果修改这里。

#define ARM_JOINT_COUNT 6

// PCA9685 I2C 7-bit 地址（默认通常是 0x40，取决于 A0~A5 跳线/焊盘）
#define ARM_PCA9685_ADDR 0x40

// 舵机 PWM 频率（标准舵机一般 50Hz）
#define ARM_SERVO_FREQ_HZ 50.0f

// 默认的脉宽范围（仅用于“起步能动”与标定，最终请按每个关节单独标定）
#define ARM_DEFAULT_US_MIN 500
#define ARM_DEFAULT_US_MAX 2500

// 插补更新周期（ms）；建议与 50Hz 对齐（20ms）
#define ARM_UPDATE_PERIOD_MS 20

