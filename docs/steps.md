# 从 0 到“舵机稳定能动”的推荐步骤（BlackPill F411 + PCA9685）

目标：
- 先让单个舵机稳定动作（不抽动、不复位）
- 再逐个关节标定（中心/方向/限位）
- 最后再做 6 轴联动插补

相关文档：
- 接线：`docs/wiring.md`
- 供电：`docs/power.md`
- CubeMX 要点：`docs/cubemx_notes.md`
- 标定：`docs/calibration.md`
- ST-Link 调试看变量：`docs/debug_stlink.md`

---

## 1) 先把硬件闭环搭起来（只接 1 个舵机）

1. 只插 1 个舵机到 PCA9685（例如 CH0），先不要把 6 个全插上。
2. 严格按 `docs/power.md` 的要求供电与共地。
3. I2C 先用短线，避免长飞线。

验收标准：
- BlackPill 不会因为舵机动作而复位
- 舵机通电后不会一直“慢慢转/抖”

---

## 2) CubeMX 新建工程并开 I2C1

按 `docs/cubemx_notes.md` 配好 I2C1，建议：
- I2C1 模式选 `I2C`（不要选 SMBus）
- 速度先用 `100kHz`
- 引脚用 `PB8=SCL`、`PB9=SDA`（板子丝印通常写 `B8/B9`）

---

## 3) 把本仓库 firmware 加到你的 CubeIDE 工程

把这两个目录的文件拷贝到你的工程里（建议保持同名目录）：
- `firmware/Inc/*.h`
- `firmware/Src/*.c`

然后在 CubeIDE 工程里：
- 把 `firmware/Inc` 加入 include path
- 把 `firmware/Src/*.c` 加入编译（确保它们在工程树里、能参与 Build）

---

## 4) 先跑“最小化接入”（固定 1500us）

推荐先用 `examples/cube_main_pca_single_servo.c` 这种最小化程序：
- 只初始化 I2C + PCA9685
- 固定给 CH0 输出 1500us（50Hz）

验收标准：
- 舵机能稳定停在一个位置（中心附近）
- 不会随机抽动

---

## 5) 再切换到 arm 层（角度控制 + 插补）

确认最小化没问题后，再用 `examples/cube_main_arm_demo.c`：
- `arm_init(&g_arm, &hi2c1)` 初始化 PCA9685 + 6 路舵机参数
- `arm_set_target(...)` 设置目标角度
- 每 20ms 调一次 `arm_update(...)` 做插补更新

注意：
- `arm_set_target()` 只改目标，不会自动初始化硬件；必须先 `arm_init()`。

---

## 6) 开始逐个关节标定

见 `docs/calibration.md`（推荐用 ST-Link 调试器“在线改变量”的方式，无需按键）。

