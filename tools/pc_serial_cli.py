#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
一个极简“串口命令发送器”占位。

说明：
- 真正可用的串口工具需要 pyserial，但本仓库遵循“不要引入未声明依赖”的原则，
  所以这里先提供脚手架与协议示例。

推荐做法：
- 先用现成的串口调试助手（115200 8N1）发送命令；
- 或者你自己在本机安装 pyserial 后把这个脚本补全。

命令示例（末尾带回车/换行）：
  J0=90 J1=90 J2=90 J3=90 J4=90 J5=90 T=1000
  J0=60 T=800
  U0=1500
"""

import sys


def main() -> int:
  print("这个脚本是占位，建议先用串口调试助手发送命令：")
  print("115200 8N1，命令示例：")
  print("  J0=60 T=800")
  print("  U0=1500")
  print("")
  print("如果你想用 Python 直接发串口：安装 pyserial 后再补全本脚本。")
  return 0


if __name__ == "__main__":
  raise SystemExit(main())

