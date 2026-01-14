#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
配置文件 - 统一管理所有PC端程序的设置
"""

# ========== 串口配置 ==========
# 重要：必须与STM32的波特率一致！
# STM32当前波特率：921600
BAUDRATE = 921600

# ========== 图像参数 ==========
IMAGE_WIDTH = 320
IMAGE_HEIGHT = 240
BYTES_PER_PIXEL = 2  # RGB565

# ========== 接收设置 ==========
RECEIVE_TIMEOUT = 0.1  # 串口读取超时（秒）
DISPLAY_INTERVAL = 0.5  # 最小显示间隔（秒）
SAVE_DIR = "captures"  # 保存目录

# ========== 调试选项 ==========
DEBUG = False  # 打印详细调试信息
SHOW_PROGRESS = True  # 显示接收进度

# ========== 自动配置 ==========
ROW_SIZE = IMAGE_WIDTH * BYTES_PER_PIXEL  # 每行字节数
FRAME_SIZE = IMAGE_WIDTH * IMAGE_HEIGHT * BYTES_PER_PIXEL  # 一帧总字节数

# 显示配置信息（仅在直接运行此文件时）
if __name__ == "__main__":
    print(f"配置加载完成:")
    print(f"  波特率: {BAUDRATE}")
    print(f"  图像尺寸: {IMAGE_WIDTH}x{IMAGE_HEIGHT}")
    print(f"  每帧数据: {FRAME_SIZE}字节")
    print(f"  预计时间: {FRAME_SIZE * 10 / BAUDRATE:.2f}秒/帧")