/*
 * 标定用（推荐）：用调试器在线修改 g_calib_us / g_calib_ch。
 *
 * 使用方法：
 * - 只接 1 个舵机到 PCA9685 的 CH0
 * - 烧录 Debug
 * - 用 ST-Link 进入 Debug，在 Expressions 里改 g_calib_us（建议每次 10~20us）
 */

#include "main.h"
#include "pca9685.h"

extern I2C_HandleTypeDef hi2c1;

// 用 volatile，方便调试器在线改值
volatile uint8_t g_calib_ch = 0;
volatile uint16_t g_calib_us = 1500;
volatile uint8_t g_calib_enable = 1;

static uint16_t clamp_u16(uint16_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

void app_main_calib_debugger(void) {
  Pca9685 pca;

  HAL_Delay(50);
  if (pca9685_init(&pca, &hi2c1, 0x40, 50.0f) != HAL_OK) {
    while (1) {
    }
  }

  uint32_t last = HAL_GetTick();
  while (1) {
    uint32_t now = HAL_GetTick();
    if ((now - last) >= 20) {
      last = now;

      if (g_calib_enable) {
        uint16_t us = clamp_u16(g_calib_us, 500, 2500);
        (void)pca9685_set_pulse_us(&pca, g_calib_ch, us);
      } else {
        // 关闭输出（保持低）
        (void)pca9685_set_pwm(&pca, g_calib_ch, 0, 0);
      }
    }
  }
}

