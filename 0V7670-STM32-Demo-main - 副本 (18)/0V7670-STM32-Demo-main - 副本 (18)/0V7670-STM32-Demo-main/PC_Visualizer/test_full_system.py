#!/usr/bin/env python3
"""
完整系统测试 - 模拟STM32发送和PC端接收
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

def simulate_stm32_send():
    """模拟STM32发送一帧数据"""
    # 生成模拟图像数据
    image_data = bytearray(153600)
    for i in range(153600):
        image_data[i] = (i * 3 + 17) % 256  # 伪随机数据

    # 计算CRC
    crc_value = crc32_software(image_data)

    # 构建数据包
    packet = b''
    packet += b'IMG_START,320,240,16,1\r\n'  # 帧头
    packet += image_data  # 图像数据
    packet += struct.pack('>I', crc_value)  # CRC32 (大端序)
    packet += b'\r\nIMAGE_END\r\n'  # 帧尾

    return packet, crc_value

def pc_receive_and_verify(packet):
    """PC端接收并验证"""
    # 解析帧头
    header_end = packet.find(b'\n')
    header = packet[:header_end].decode()
    print(f"帧头: {header}")

    # 提取图像数据
    image_start = header_end + 1
    image_data = packet[image_start:image_start + 153600]

    # 提取CRC
    crc_start = image_start + 153600
    received_crc = struct.unpack('>I', packet[crc_start:crc_start+4])[0]

    # 验证CRC
    calculated_crc = zlib.crc32(image_data) & 0xFFFFFFFF

    print(f"收到CRC:     {hex(received_crc)}")
    print(f"计算CRC:     {hex(calculated_crc)}")
    print(f"数据长度:    {len(image_data)} 字节")
    print(f"CRC校验:     {'通过' if received_crc == calculated_crc else '失败'}")

    return received_crc == calculated_crc

# 运行测试
print("=" * 60)
print("完整系统测试")
print("=" * 60)

print("\n1. STM32端模拟发送...")
packet, sent_crc = simulate_stm32_send()
print(f"   数据包总长度: {len(packet)} 字节")
print(f"   发送CRC: {hex(sent_crc)}")

print("\n2. PC端接收验证...")
result = pc_receive_and_verify(packet)

print("\n" + "=" * 60)
if result:
    print("✓ 测试通过！CRC校验工作正常")
    print("  STM32和PC端的CRC算法完全兼容")
else:
    print("❌ 测试失败！需要检查CRC实现")
print("=" * 60)
