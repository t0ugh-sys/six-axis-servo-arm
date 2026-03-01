#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arm.h"

// 一个非常轻量的命令解析器（可选）：
// - 输入：一行 ASCII 文本（以 \r 或 \n 结束）
// - 输出：更新机械臂目标
//
// 支持示例：
// - "J0=90 J1=45 J2=90 J3=90 J4=90 J5=90 T=1000"
// - "U0=1500"（直接设置通道脉宽，常用于标定）
typedef struct {
  uint32_t default_duration_ms;
} ArmCmd;

void arm_cmd_init(ArmCmd* cmd, uint32_t default_duration_ms);

// 返回 true 表示成功识别并执行；false 表示无效/不完整
bool arm_cmd_execute_line(ArmCmd* cmd, Arm* arm, const char* line);

