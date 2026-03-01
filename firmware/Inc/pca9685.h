#pragma once

#include <stdint.h>

#include "stm32f4xx_hal.h"

typedef struct {
  I2C_HandleTypeDef* hi2c;
  uint8_t addr_7bit;
  float pwm_freq_hz;
} Pca9685;

// 初始化 PCA9685（设置输出模式 + PWM 频率）
HAL_StatusTypeDef pca9685_init(Pca9685* dev, I2C_HandleTypeDef* hi2c, uint8_t addr_7bit, float pwm_freq_hz);

// 设置 PWM 频率（Hz），舵机一般 50Hz
HAL_StatusTypeDef pca9685_set_pwm_freq(Pca9685* dev, float pwm_freq_hz);

// 设置单通道 PWM（tick: 0..4095）
HAL_StatusTypeDef pca9685_set_pwm(Pca9685* dev, uint8_t channel, uint16_t on_tick, uint16_t off_tick);

// 设置所有通道 PWM（tick: 0..4095）
HAL_StatusTypeDef pca9685_set_all_pwm(Pca9685* dev, uint16_t on_tick, uint16_t off_tick);

// 便于舵机：以“脉宽 us”设置占空比（on=0, off=tick）
HAL_StatusTypeDef pca9685_set_pulse_us(Pca9685* dev, uint8_t channel, uint16_t pulse_us);

