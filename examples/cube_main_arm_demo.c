/*
 * 机械臂层示例：角度控制 + 插补更新。
 *
 * 注意：
 * - 这个示例假设你已经把 firmware/Inc 与 firmware/Src 拷进工程并参与编译。
 * - 先把供电与标定搞定，再做 6 轴联动。
 */

#include "main.h"
#include "arm.h"

extern I2C_HandleTypeDef hi2c1;

static Arm g_arm;

void app_main_arm_demo(void) {
  HAL_Delay(50);
  if (arm_init(&g_arm, &hi2c1) != HAL_OK) {
    while (1) {
    }
  }

  // 先只动 1 个关节做验证（J0 到 60°，用 1 秒插补过去）
  float t[ARM_JOINT_COUNT] = {90, 90, 90, 90, 90, 90};
  t[0] = 60;
  arm_set_target(&g_arm, t, 1000);

  uint32_t last = HAL_GetTick();
  while (1) {
    uint32_t now = HAL_GetTick();
    if ((now - last) >= ARM_UPDATE_PERIOD_MS) {
      last = now;
      (void)arm_update(&g_arm);
    }
  }
}

