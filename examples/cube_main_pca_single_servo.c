/*
 * 最小化接入：只初始化 PCA9685，然后固定输出 1500us 到 CH0。
 *
 * 用途：
 * - 排除供电/接线问题
 * - 排除 arm 插补/映射问题
 *
 * 你需要把这段逻辑移到 CubeIDE 生成的 main.c 的 USER CODE 区域里。
 */

#include "main.h"
#include "pca9685.h"

extern I2C_HandleTypeDef hi2c1;

void app_main_minimal_pca(void) {
  Pca9685 pca;

  HAL_Delay(50);
  if (pca9685_init(&pca, &hi2c1, 0x40, 50.0f) != HAL_OK) {
    while (1) {
    }
  }

  // 1500us 通常接近舵机中心
  if (pca9685_set_pulse_us(&pca, 0, 1500) != HAL_OK) {
    while (1) {
    }
  }

  while (1) {
    HAL_Delay(1000);
  }
}

