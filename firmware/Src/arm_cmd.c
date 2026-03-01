#include "arm_cmd.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char* skip_spaces(const char* s) {
  while (s != NULL && *s != '\0' && isspace((unsigned char)*s)) s++;
  return s;
}

static bool parse_u32(const char* s, uint32_t* out, const char** endp) {
  if (s == NULL || out == NULL) return false;
  char* end = NULL;
  unsigned long v = strtoul(s, &end, 10);
  if (end == s) return false;
  *out = (uint32_t)v;
  if (endp != NULL) *endp = end;
  return true;
}

static bool parse_i32(const char* s, int32_t* out, const char** endp) {
  if (s == NULL || out == NULL) return false;
  char* end = NULL;
  long v = strtol(s, &end, 10);
  if (end == s) return false;
  *out = (int32_t)v;
  if (endp != NULL) *endp = end;
  return true;
}

void arm_cmd_init(ArmCmd* cmd, uint32_t default_duration_ms) {
  cmd->default_duration_ms = default_duration_ms;
}

bool arm_cmd_execute_line(ArmCmd* cmd, Arm* arm, const char* line) {
  if (cmd == NULL || arm == NULL || line == NULL) return false;

  float target[ARM_JOINT_COUNT];
  for (uint32_t i = 0; i < ARM_JOINT_COUNT; i++) target[i] = arm->target_deg[i];

  uint32_t duration = cmd->default_duration_ms;
  bool any = false;

  const char* p = line;
  while (p != NULL && *p != '\0') {
    p = skip_spaces(p);
    if (*p == '\0') break;

    // 解析 token：Jx=val / Ux=us / T=ms
    if ((p[0] == 'J' || p[0] == 'j') && isdigit((unsigned char)p[1])) {
      uint32_t idx = 0;
      const char* q = NULL;
      if (!parse_u32(p + 1, &idx, &q)) return false;
      if (idx >= ARM_JOINT_COUNT) return false;
      q = skip_spaces(q);
      if (*q != '=') return false;
      q++;

      int32_t deg_i = 0;
      if (!parse_i32(q, &deg_i, &p)) return false;
      target[idx] = (float)deg_i;
      any = true;
      continue;
    }

    if ((p[0] == 'U' || p[0] == 'u') && isdigit((unsigned char)p[1])) {
      uint32_t ch = 0;
      const char* q = NULL;
      if (!parse_u32(p + 1, &ch, &q)) return false;
      if (ch > 15) return false;
      q = skip_spaces(q);
      if (*q != '=') return false;
      q++;

      uint32_t us = 0;
      if (!parse_u32(q, &us, &p)) return false;
      if (us > 3000) us = 3000;
      (void)pca9685_set_pulse_us(&arm->pca, (uint8_t)ch, (uint16_t)us);
      any = true;
      continue;
    }

    if (p[0] == 'T' || p[0] == 't') {
      const char* q = skip_spaces(p + 1);
      if (*q != '=') return false;
      q++;
      uint32_t ms = 0;
      if (!parse_u32(q, &ms, &p)) return false;
      duration = ms;
      any = true;
      continue;
    }

    // 不认识的 token：跳过到下一个空白
    while (*p != '\0' && !isspace((unsigned char)*p)) p++;
  }

  if (!any) return false;
  arm_set_target(arm, target, duration);
  return true;
}

