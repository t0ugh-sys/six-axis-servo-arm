# CubeMX / STM32CubeIDE 配置要点（STM32F411 BlackPill）

## 1) I2C1 选哪种模式？

在 CubeMX 的 Connectivity 里开 I2C1 时：
- 选择 `I2C`（标准 I2C）
- 不要选 `SMBus-aert-mode` / `SMBus-two-wire-interface`

原因：PCA9685 是标准 I2C 设备，不需要 SMBus 的额外协议特性。

## 2) 在哪里选引脚？

常用两种方式（都可以）：
- 在 Pinout 视图直接点引脚，把 `PB8` 设为 `I2C1_SCL`、`PB9` 设为 `I2C1_SDA`
- 或者在左侧 I2C1 的 Parameter/Pin 里选择对应引脚（如果界面提供）

`GPIO Settings` 主要是看/改 GPIO 的模式、上下拉、速度等细节，不是“选外设引脚”的唯一入口。

## 3) 为什么板子丝印是 B8/B9？

BlackPill 的丝印通常用简写：
- `B8` 代表 `PB8`
- `B9` 代表 `PB9`

同理：
- `A2` 代表 `PA2`
- `A3` 代表 `PA3`

## 4) （可选）开串口是什么意思？

“开串口”一般指启用一个 UART，用来：
- 打印日志（比如 I2C 初始化是否成功）
- 后期做简单命令控制（比如输入角度/脉宽）

BlackPill 常用：
- `USART2`：`PA2=TX`、`PA3=RX`，115200 8N1

注意：你用 ST-Link 也能调试，但串口对排错非常有用（可选，不强制）。

