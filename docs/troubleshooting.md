# 常见问题排查（FAQ）

## 1) 舵机抽动/乱转/慢慢漂移

优先按“供电问题”排查：
- 舵机电源必须独立（5~6V 大电流），不要用开发板 5V
- 必须共地：舵机电源 GND、PCA9685 GND、STM32 GND
- PCA9685 `VCC=3.3V`（逻辑），`V+=5~6V`（舵机）
- `OE` 不要接 5V；建议直接接 GND
- 在 PCA9685 `V+` 端并 1000uF~2200uF 电解电容

软件侧建议：
- 先跑 `examples/cube_main_pca_single_servo.c` 固定 1500us 排除插补/映射问题
- I2C 先用 100kHz，线尽量短

## 2) 舵机不动

- 确认 PCA9685 有两路电：
  - `VCC=3.3V`（逻辑）
  - `V+` 有 5~6V（舵机）
- 确认共地
- 确认舵机插在正确通道（例如 CH0）
- 确认 I2C 地址是 0x40（或你改过 A0~A5）

## 3) 一接 ST-Link 就异常（抽动/复位/跑飞）

常见原因是“用 ST-Link 3.3V 给板子供电”导致不稳：
- 推荐：BlackPill 自己 USB 供电
- ST-Link 只接 SWDIO/SWCLK/GND（可选 NRST）

## 4) Debug 一连上程序就停住不跑

这是正常现象：进入调试会话 CPU 可能会 halt。
- 点一次 Resume（F8）让它继续跑

## 5) I2C 不通

- 确认 `PB8=SCL`、`PB9=SDA`（丝印 B8/B9）
- 确认没有接反 SDA/SCL
- 先用 100kHz
- 线太长会出问题，先用短线验证

