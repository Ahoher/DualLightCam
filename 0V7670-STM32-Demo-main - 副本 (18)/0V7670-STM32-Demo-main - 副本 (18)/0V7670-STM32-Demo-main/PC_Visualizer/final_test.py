#!/usr/bin/env python3
"""
最终验证测试 - 模拟STM32逐行发送 + CRC计算
"""

import zlib
import struct

def crc32_software(data_bytes):
    """STM32软件CRC32实现"""
    crc = 0xFFFFFFFF
    for byte in data_bytes:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xEDB88320
            else:
                crc >>= 1
    return ~crc & 0xFFFFFFFF

def simulate_stm32逐行发送():
    """模拟STM32逐行读取、发送、计算CRC"""
    print("模拟STM32端:")
    print("-" * 50)

    # 生成测试图像数据
    image_data = bytearray(153600)
    for i in range(153600):
        image_data[i] = (i * 3 + 17) % 256

    # 模拟逐行处理
    crc_value = 0xFFFFFFFF
    sent_data = bytearray()

    for row in range(240):
        row_data = image_data[row * 640:(row + 1) * 640]

        # 发送行数据
        sent_data.extend(row_data)

        # 计算CRC（逐行累加）
        for byte in row_data:
            crc_value ^= byte
            for _ in range(8):
                if crc_value & 1:
                    crc_value = (crc_value >> 1) ^ 0xEDB88320
                else:
                    crc_value >>= 1

        if row % 40 == 0:
            print(f"  行 {row:3d}/240 - 发送 {len(row_data)} 字节")

    # 完成CRC
    crc_value = ~crc_value & 0xFFFFFFFF
    print(f"  CRC计算完成: {hex(crc_value)}")

    return sent_data, crc_value

def pc端验证(image_data, received_crc):
    """PC端接收验证"""
    print("\nPC端验证:")
    print("-" * 50)

    # 长度检查
    if len(image_data) != 153600:
        print(f"  ❌ 数据长度错误: {len(image_data)}")
        return False

    print(f"  ✓ 数据长度正确: {len(image_data)} 字节")

    # CRC验证
    calculated_crc = zlib.crc32(image_data) & 0xFFFFFFFF
    print(f"  收到CRC: {hex(received_crc)}")
    print(f"  计算CRC: {hex(calculated_crc)}")

    if received_crc == calculated_crc:
        print("  ✓ CRC校验通过")
        return True
    else:
        print("  ❌ CRC校验失败")
        return False

# 运行测试
print("=" * 60)
print("最终系统验证 - 逐行发送 + CRC")
print("=" * 60)
print()

# 模拟STM32
image_data, crc = simulate_stm32逐行发送()

# PC验证
result = pc端验证(image_data, crc)

print()
print("=" * 60)
if result:
    print("✓ 测试通过！")
    print("  STM32和PC端的CRC算法完全兼容")
    print("  可以安全使用此CRC实现")
else:
    print("❌ 测试失败！")
print("=" * 60)
