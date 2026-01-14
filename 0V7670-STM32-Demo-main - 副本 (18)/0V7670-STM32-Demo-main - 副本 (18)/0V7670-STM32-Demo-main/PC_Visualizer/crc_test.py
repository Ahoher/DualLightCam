#!/usr/bin/env python3
"""
CRC32兼容性测试
验证STM32硬件CRC与Python计算的匹配性
"""

import struct

def stm32_crc32(data_bytes):
    """
    模拟STM32硬件CRC32计算
    STM32使用标准CRC-32多项式，但不进行输入输出反转
    """
    # CRC-32多项式 (标准)
    poly = 0x04C11DB7

    # 初始化寄存器
    crc = 0xFFFFFFFF

    # 处理每个字节
    for byte in data_bytes:
        crc ^= byte << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF

    # STM32硬件CRC不反转输出
    return crc

def python_crc32(data_bytes):
    """Python标准CRC32"""
    import zlib
    return zlib.crc32(data_bytes) & 0xFFFFFFFF

# 测试
test_data = b"Hello, World! This is a test for CRC32 verification."

print("测试数据:", test_data)
print("数据长度:", len(test_data), "字节")
print()

stm32_crc = stm32_crc32(test_data)
python_crc = python_crc32(test_data)

print(f"STM32硬件CRC: {hex(stm32_crc)}")
print(f"Python zlibCRC: {hex(python_crc)}")
print(f"匹配: {stm32_crc == python_crc}")

# 如果不匹配，尝试反转
def reverse_bits32(n):
    """反转32位"""
    result = 0
    for i in range(32):
        if n & (1 << i):
            result |= (1 << (31 - i))
    return result

def reverse_bytes32(n):
    """反转字节序"""
    return struct.unpack('>I', struct.pack('<I', n))[0]

print()
print("尝试不同反转方式:")
print(f"反转位: {hex(reverse_bits32(stm32_crc))} == {hex(python_crc)} ? {reverse_bits32(stm32_crc) == python_crc}")
print(f"反转字节: {hex(reverse_bytes32(stm32_crc))} == {hex(python_crc)} ? {reverse_bytes32(stm32_crc) == python_crc}")
print(f"反转位+字节: {hex(reverse_bytes32(reverse_bits32(stm32_crc)))} == {hex(python_crc)} ? {reverse_bytes32(reverse_bits32(stm32_crc)) == python_crc}")
