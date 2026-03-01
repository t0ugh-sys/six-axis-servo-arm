# 标定流程（不使用按键，推荐用 ST-Link 在线改脉宽）

目标：给每个关节得到一组“可用参数”：
- `us_min/us_max`：不顶死、不发热的安全脉宽范围
- `offset_deg`：机械零位偏置（让“90 度”对应你想要的零位）
- `inverted`：方向是否需要反转
- `angle_min_deg/angle_max_deg`：角度限位（保护机械结构）

下面给两种“无按键”标定方式，你可以二选一：

---

## 方式 A：调试器在线改 `g_calib_us`（最推荐）

思路：程序每 20ms 把 `g_calib_us` 写到 PCA9685 的某一路；你在 Debug 里直接改 `g_calib_us`，舵机会立刻响应。

1) 只接 1 个舵机到 CH0（先别装连杆，先空载）。
2) 把 `examples/cube_main_calib_debugger.c` 复制到你的 `main.c`（或照着抄 USER CODE 部分）。
3) 烧录一次（Debug 配置）。
4) 进入 Debug，会话里打开 Expressions：
   - 看 `g_calib_us`
   - 直接修改它，例如 1500 → 1400 → 1600（小步慢改）
5) 记录：
   - 舵机“刚好不顶死”的最小 `us_min`
   - 舵机“刚好不顶死”的最大 `us_max`
6) 把结果填回 `servo_init(...)`（见 `firmware/Src/arm.c`）或你自己做一份配置表。

建议步进：
- 每次 10~20us，观察是否卡住/发热/打齿
- 有任何异常立刻回到 1500us

---

## 方式 B：自动慢扫（不用改变量）

思路：程序自动在一个保守范围内来回扫（例如 1200~1800us），你观察机械极限，再逐步扩大范围。

优点：不需要调试器改变量。  
缺点：不如方式 A 灵活，容易扫到危险区（要保守设置）。

---

## 标定完怎么变成“角度控制”？

这个仓库的角度控制逻辑是：

- `servo_write_angle()`：把 `angle_deg` 线性映射到 `[us_min, us_max]`
- 90 度是否是“真正中心”取决于 `angle_min/max` 与 `offset_deg`

所以你标定后，一般做法是：
- 把 `us_min/us_max` 写进每个关节
- 根据安装的机械零位决定 `offset_deg`
- 如果方向反了，把 `inverted=true`

