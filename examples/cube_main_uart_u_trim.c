/*
 * UART U-Trim (microsecond pulse trim) helper for STM32CubeIDE projects.
 *
 * 目标：
 * - 用最直观的方式“微调 U(微秒脉宽) 找中位”，然后再装舵盘/连杆
 * - 只控制一个 PCA9685 通道，避免 arm 层插补/角度映射干扰
 * - RX 中断只收字节 + ring buffer，避免丢字节
 * - 过滤 PuTTY 方向键/退格产生的控制序列，避免命令莫名 ERR
 *
 * 使用方法：
 * 1) CubeMX：启用 I2C1 + USART2，并生成代码
 *    - USART2：115200 8N1，勾选 “USART2 global interrupt”
 * 2) 将 firmware/Inc firmware/Src 加入工程并参与编译
 * 3) 将本文件加入工程并参与编译
 * 4) 在 CubeMX 生成的 main.c 的 USER CODE 区调用：
 *      app_main_uart_u_trim();
 *
 * 串口命令（ASCII 文本发送，末尾必须 CRLF/LF）：
 * - HELP              打印帮助
 * - P                 打印当前参数
 * - CH=0              选择 PCA9685 通道（0~15）
 * - U=1500            设定当前通道脉宽（us）
 * - +                 当前脉宽 += STEP
 * - -                 当前脉宽 -= STEP
 * - STEP=10           设置步进（us）
 * - MIN=500 / MAX=2500 设置限幅（us）
 * - SAVE              打印可复制的结果行（例如 U0=1490）
 *
 * 注意：
 * - 找中位时建议先空载（先别装舵盘/连杆），避免卡死烧舵机。
 * - 50Hz 下常见区间约 1000~2000us；不同舵机/机构会有偏差。
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#include "pca9685.h"

extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

// ---- 可按需要修改：I2C1 引脚（用于 BUS recovery）----
// 默认：PB6=SCL PB7=SDA
#ifndef APP_I2C1_GPIO_PORT
#define APP_I2C1_GPIO_PORT GPIOB
#endif
#ifndef APP_I2C1_SCL_PIN
#define APP_I2C1_SCL_PIN GPIO_PIN_6
#endif
#ifndef APP_I2C1_SDA_PIN
#define APP_I2C1_SDA_PIN GPIO_PIN_7
#endif

#define RX_BUF_SIZE 512u
static volatile uint8_t g_rx_buf[RX_BUF_SIZE];
static volatile uint16_t g_rx_head = 0;
static volatile uint16_t g_rx_tail = 0;
static uint8_t g_rx_byte = 0;

static void rx_push(uint8_t b) {
  uint16_t next = (uint16_t)((g_rx_head + 1u) % RX_BUF_SIZE);
  if (next == g_rx_tail) return;
  g_rx_buf[g_rx_head] = b;
  g_rx_head = next;
}

static bool rx_pop(uint8_t* out) {
  if (g_rx_tail == g_rx_head) return false;
  *out = g_rx_buf[g_rx_tail];
  g_rx_tail = (uint16_t)((g_rx_tail + 1u) % RX_BUF_SIZE);
  return true;
}

static void uart_start_rx_it(void) {
  (void)HAL_UART_Receive_IT(&huart2, &g_rx_byte, 1);
}

static void uart_send_str(const char* s) {
  if (s == NULL) return;
  (void)HAL_UART_Transmit(&huart2, (uint8_t*)s, (uint16_t)strlen(s), 200);
}

static void uart_sendf(const char* fmt, ...) {
  char buf[220];
  va_list ap;
  va_start(ap, fmt);
  (void)vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  uart_send_str(buf);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART2) {
    rx_push(g_rx_byte);
    uart_start_rx_it();
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart) {
  if (huart->Instance == USART2) {
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    uart_start_rx_it();
  }
}

static const char* skip_spaces(const char* s) {
  while (s != NULL && *s != '\0' && isspace((unsigned char)*s)) s++;
  return s;
}

static bool parse_u32(const char* s, uint32_t* out) {
  if (s == NULL || out == NULL) return false;
  char* end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s) return false;
  *out = (uint32_t)v;
  return true;
}

static uint16_t clamp_u16(uint32_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return (uint16_t)v;
}

static void i2c1_force_reset_periph(void) {
  __HAL_RCC_I2C1_FORCE_RESET();
  HAL_Delay(1);
  __HAL_RCC_I2C1_RELEASE_RESET();
  HAL_Delay(1);
}

static void i2c1_bus_recovery(void) {
  __HAL_RCC_I2C1_CLK_DISABLE();

  if (APP_I2C1_GPIO_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
#ifdef GPIOA
  if (APP_I2C1_GPIO_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
#endif

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  GPIO_InitStruct.Pin = APP_I2C1_SCL_PIN | APP_I2C1_SDA_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(APP_I2C1_GPIO_PORT, &GPIO_InitStruct);

  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN | APP_I2C1_SDA_PIN, GPIO_PIN_SET);
  HAL_Delay(2);

  for (int i = 0; i < 9; i++) {
    HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
  }

  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SDA_PIN, GPIO_PIN_RESET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SCL_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
  HAL_GPIO_WritePin(APP_I2C1_GPIO_PORT, APP_I2C1_SDA_PIN, GPIO_PIN_SET);
  HAL_Delay(1);
}

static HAL_StatusTypeDef pca_init_with_retries(Pca9685* pca) {
  HAL_Delay(500);

  for (int attempt = 1; attempt <= 10; attempt++) {
    (void)HAL_I2C_DeInit(&hi2c1);
    i2c1_force_reset_periph();
    (void)HAL_I2C_Init(&hi2c1);

    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(0x40u << 1), 2, 80) != HAL_OK) {
      uart_send_str("0x40 NACK -> recover\r\n");
      i2c1_bus_recovery();
      HAL_Delay(80);
      continue;
    }

    HAL_StatusTypeDef st = pca9685_init(pca, &hi2c1, 0x40u, 50.0f);
    if (st == HAL_OK) return HAL_OK;

    uart_sendf("pca_init FAIL (attempt %d) -> recover\r\n", attempt);
    i2c1_bus_recovery();
    HAL_Delay(120);
  }

  return HAL_ERROR;
}

static void print_help(void) {
  uart_send_str(
      "HELP/P/CH/U/+/-/STEP/MIN/MAX/SAVE\r\n"
      "Examples:\r\n"
      "  CH=0\r\n"
      "  U=1500\r\n"
      "  STEP=10\r\n"
      "  +\r\n"
      "  -\r\n"
      "  SAVE\r\n");
}

static void print_state(uint8_t ch, uint16_t us, uint16_t step, uint16_t us_min, uint16_t us_max) {
  uart_sendf("CH=%u U=%u STEP=%u MIN=%u MAX=%u\r\n", ch, us, step, us_min, us_max);
}

static bool str_ieq(const char* a, const char* b) {
  if (a == NULL || b == NULL) return false;
  while (*a != '\0' && *b != '\0') {
    char ca = (char)tolower((unsigned char)*a++);
    char cb = (char)tolower((unsigned char)*b++);
    if (ca != cb) return false;
  }
  return *a == '\0' && *b == '\0';
}

void app_main_uart_u_trim(void) {
  uart_send_str("UART ready\r\n");
  uart_send_str("U-trim mode (PCA9685)\r\n");

  uart_start_rx_it();

  Pca9685 pca;
  if (pca_init_with_retries(&pca) != HAL_OK) {
    uart_send_str("pca_init FINAL FAIL\r\n");
    while (1) HAL_Delay(200);
  }

  uart_send_str("PCA ready\r\n");
  print_help();

  uint8_t ch = 0;
  uint16_t us_min = 500;
  uint16_t us_max = 2500;
  uint16_t step = 10;
  uint16_t us = 1500;

  (void)pca9685_set_pulse_us(&pca, ch, us);
  print_state(ch, us, step, us_min, us_max);

  // 过滤 PuTTY 控制序列（方向键等）：ESC [ ... <final>
  uint8_t esc_state = 0;

  char line[96];
  uint32_t line_len = 0;

  while (1) {
    uint8_t b;
    while (rx_pop(&b)) {
      // ANSI ESC filtering
      if (esc_state == 0 && b == 0x1B) {
        esc_state = 1;
        continue;
      }
      if (esc_state == 1) {
        esc_state = (b == '[') ? 2 : 0;
        continue;
      }
      if (esc_state == 2) {
        if (b >= 0x40 && b <= 0x7E) esc_state = 0;
        continue;
      }

      // backspace / del
      if (b == 0x08 || b == 0x7F) {
        if (line_len > 0) line_len--;
        continue;
      }

      if (b < 0x20 && b != '\r' && b != '\n') continue;

      // echo
      (void)HAL_UART_Transmit(&huart2, &b, 1, 10);

      char c = (char)b;
      if (c == '\r' || c == '\n') {
        if (line_len == 0) continue;
        if (line_len >= sizeof(line)) line_len = sizeof(line) - 1;
        line[line_len] = '\0';
        line_len = 0;

        const char* cmd = skip_spaces(line);

        if (str_ieq(cmd, "HELP")) {
          uart_send_str("\r\n");
          print_help();
          continue;
        }
        if (str_ieq(cmd, "P")) {
          uart_send_str("\r\n");
          print_state(ch, us, step, us_min, us_max);
          continue;
        }
        if (str_ieq(cmd, "+")) {
          uart_send_str("\r\n");
          us = clamp_u16((uint32_t)us + (uint32_t)step, us_min, us_max);
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }
        if (str_ieq(cmd, "-")) {
          uart_send_str("\r\n");
          uint32_t v = (us > step) ? ((uint32_t)us - (uint32_t)step) : 0u;
          us = clamp_u16(v, us_min, us_max);
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }
        if (str_ieq(cmd, "SAVE")) {
          uart_send_str("\r\n");
          uart_sendf("SAVE: U%u=%u\r\n", ch, us);
          uart_sendf("HINT: record MIN/MAX too: MIN=%u MAX=%u STEP=%u\r\n", us_min, us_max, step);
          continue;
        }

        // key=value commands: CH= / U= / STEP= / MIN= / MAX=
        const char* eq = strchr(cmd, '=');
        if (eq == NULL) {
          uart_send_str("\r\nERR\r\n");
          continue;
        }

        char key[16];
        size_t kn = (size_t)(eq - cmd);
        if (kn >= sizeof(key)) kn = sizeof(key) - 1;
        memcpy(key, cmd, kn);
        key[kn] = '\0';

        const char* val = skip_spaces(eq + 1);
        uint32_t v = 0;
        if (!parse_u32(val, &v)) {
          uart_send_str("\r\nERR\r\n");
          continue;
        }

        uart_send_str("\r\n");

        if (str_ieq(key, "CH")) {
          if (v > 15u) {
            uart_send_str("ERR\r\n");
            continue;
          }
          ch = (uint8_t)v;
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }

        if (str_ieq(key, "U")) {
          us = clamp_u16(v, us_min, us_max);
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }

        if (str_ieq(key, "STEP")) {
          if (v == 0u) v = 1u;
          if (v > 200u) v = 200u;
          step = (uint16_t)v;
          print_state(ch, us, step, us_min, us_max);
          continue;
        }

        if (str_ieq(key, "MIN")) {
          if (v > 3000u) v = 3000u;
          us_min = (uint16_t)v;
          if (us_min > us_max) us_max = us_min;
          us = clamp_u16(us, us_min, us_max);
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }

        if (str_ieq(key, "MAX")) {
          if (v > 3000u) v = 3000u;
          us_max = (uint16_t)v;
          if (us_max < us_min) us_min = us_max;
          us = clamp_u16(us, us_min, us_max);
          (void)pca9685_set_pulse_us(&pca, ch, us);
          print_state(ch, us, step, us_min, us_max);
          continue;
        }

        uart_send_str("ERR\r\n");
      } else {
        if (line_len < (sizeof(line) - 1)) line[line_len++] = c;
        else line_len = 0;
      }
    }
  }
}

