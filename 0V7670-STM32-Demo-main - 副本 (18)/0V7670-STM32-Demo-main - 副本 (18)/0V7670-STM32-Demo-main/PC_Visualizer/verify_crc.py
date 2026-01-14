#!/usr/bin/env python3
"""
验证STM32软件CRC32与Python zlib.crc32的匹配性
"""

import zlib

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

# 测试数据
test_data = b"Hello, World! This is a test for CRC32 verification."

# 两种计算方式
crc_python = zlib.crc32(test_data) & 0xFFFFFFFF
crc_software = crc32_software(test_data)

print("测试数据:", test_data)
print(f"Python zlib.crc32:  {hex(crc_python)}")
print(f"软件CRC32实现:      {hex(crc_software)}")
print(f"匹配: {crc_python == crc_software}")

# 测试图像数据
print("\n测试图像数据:")
image_data = bytes([i % 256 for i in range(153600)])  # 模拟图像数据
crc_python = zlib.crc32(image_data) & 0xFFFFFFFF
crc_software = crc32_software(image_data)

print(f"图像数据长度: {len(image_data)} 字节")
print(f"Python zlib.crc32:  {hex(crc_python)}")
print(f"软件CRC32实现:      {hex(crc_software)}")
print(f"匹配: {crc_python == crc_software}")

if crc_python == crc_software:
    print("\n✓ CRC32算法验证通过！STM32代码可以使用此实现。")
else:
    print("\n❌ CRC32不匹配，需要调整算法")
