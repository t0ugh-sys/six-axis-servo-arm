#pragma once

#include <stdint.h>

#include "arm_config.h"
#include "servo.h"

typedef struct {
  Pca9685 pca;
  Servo joints[ARM_JOINT_COUNT];

  float current_deg[ARM_JOINT_COUNT];
  float target_deg[ARM_JOINT_COUNT];

  // 方便标定/排错：记录上一次写出的脉宽（us）
  uint16_t last_pulse_us[ARM_JOINT_COUNT];

  uint32_t move_total_steps;
  uint32_t move_step_idx;
  float step_delta_deg[ARM_JOINT_COUNT];
} Arm;

// 初始化：传入 CubeMX 生成的 I2C 句柄
HAL_StatusTypeDef arm_init(Arm* arm, I2C_HandleTypeDef* hi2c);

// 立即写当前角度（不插补）
HAL_StatusTypeDef arm_write_now(Arm* arm);

// 设置目标角度并启动插补（duration_ms=0 则立刻到位）
void arm_set_target(Arm* arm, const float deg[ARM_JOINT_COUNT], uint32_t duration_ms);

// 每 ARM_UPDATE_PERIOD_MS 调一次，输出到 PCA9685
HAL_StatusTypeDef arm_update(Arm* arm);

