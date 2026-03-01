# 接线（BlackPill F411 + PCA9685 + 舵机）

## I2C（BlackPill ↔ PCA9685）

推荐使用 I2C1：
- BlackPill `B8(PB8)` → PCA9685 `SCL`
- BlackPill `B9(PB9)` → PCA9685 `SDA`
- BlackPill `GND` → PCA9685 `GND`
- BlackPill `3.3V` → PCA9685 `VCC`（逻辑电源）

说明：
- 板子口标注 `A0~A15 / B0~B9` 是“简化丝印”，本质仍然是 `PAx/PBx`。
- PCA9685 的 SDA/SCL 通常已经带上拉（取决于你买的板子），先不用额外加上拉电阻。

## 输出使能 OE

- `OE` 建议直接接 `GND`（使能输出）
- 不要接 5V（`OE` 高会禁用输出）

## 舵机电源（关键）

- 舵机电源 `5~6V` → PCA9685 `V+`
- 舵机电源 `GND` → PCA9685 `GND`
- 舵机三线：`GND / V+ / Signal` 分别接 PCA9685 对应三排针脚

强制要求：
- 舵机电源 GND、PCA9685 GND、BlackPill GND 必须共地

