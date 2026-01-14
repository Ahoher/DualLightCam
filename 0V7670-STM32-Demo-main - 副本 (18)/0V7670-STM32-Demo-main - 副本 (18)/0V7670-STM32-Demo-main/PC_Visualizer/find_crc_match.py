#!/usr/bin/env python3
"""
找出与STM32硬件CRC匹配的Python实现
"""

import struct
import zlib

def stm32_crc32_sw(data_bytes):
    """
    STM32硬件CRC32的软件实现
    根据参考手册：CRC-32 (ISO 3309), 多项式 0x04C11DB7
    不进行输入/输出反转
    """
    poly = 0x04C11DB7
    crc = 0xFFFFFFFF

    for byte in data_bytes:
        crc ^= byte << 24
        for _ in range(8):
            if crc & 0x80000000:
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFFFFFFFF

    return crc

def test_variations(data):
    """测试各种CRC变体"""
    results = {}

    # 1. 标准zlib
    results['zlib'] = zlib.crc32(data) & 0xFFFFFFFF

    # 2. STM32模拟
    results['stm32_sw'] = stm32_crc32_sw(data)

    # 3. 各种反转组合
    def rev_bits(n):
        r = 0
        for i in range(32):
            if n & (1 << i):
                r |= (1 << (31 - i))
        return r

    def rev_bytes(n):
        return struct.unpack('>I', struct.pack('<I', n))[0]

    results['stm32_sw_rev_bits'] = rev_bits(results['stm32_sw'])
    results['stm32_sw_rev_bytes'] = rev_bytes(results['stm32_sw'])
    results['stm32_sw_rev_both'] = rev_bits(rev_bytes(results['stm32_sw']))

    # 4. zlib变体
    results['zlib_rev_bits'] = rev_bits(results['zlib'])
    results['zlib_rev_bytes'] = rev_bytes(results['zlib'])
    results['zlib_rev_both'] = rev_bits(rev_bytes(results['zlib']))

    return results

# 测试数据
test_data = b"Hello, World! This is a test."

print("=" * 70)
print("CRC32匹配测试")
print("=" * 70)
print(f"测试数据: {test_data}")
print(f"长度: {len(test_data)} 字节")
print()

results = test_variations(test_data)

print("各种CRC计算结果:")
for name, value in results.items():
    print(f"  {name:20s}: {hex(value)}")

print()
print("=" * 70)
print("匹配分析:")
print("=" * 70)

# 假设我们从STM32收到一个CRC值，我们需要找到匹配的Python计算方式
# 这里我们假设STM32的CRC是某个值，然后反向查找

target = results['stm32_sw']  # 假设这是STM32的输出
print(f"假设STM32输出: {hex(target)}")
print()

# 找出哪个计算方式等于这个值
for name, value in results.items():
    if value == target:
        print(f"✓ 匹配: {name}")

print()
print("推荐方案:")
print("  STM32端: 使用硬件CRC")
print("  PC端: 使用自定义CRC32实现，匹配STM32硬件")
print()
print("或者:")
print("  STM32端: 使用软件CRC实现")
print("  PC端: 使用zlib.crc32")
