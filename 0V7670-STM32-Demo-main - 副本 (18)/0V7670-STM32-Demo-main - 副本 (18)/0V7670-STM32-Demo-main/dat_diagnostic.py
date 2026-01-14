#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
DAT文件诊断工具 - 分析文件结构和内容
"""

import sys
from pathlib import Path

def diagnose_dat_file(filepath):
    """诊断DAT文件的结构"""
    filepath = Path(filepath)

    if not filepath.exists():
        print(f"ERROR: 文件不存在: {filepath}")
        return

    print(f"\n{'='*60}")
    print(f"DAT文件诊断: {filepath}")
    print(f"{'='*60}\n")

    # 读取文件
    with open(filepath, 'rb') as f:
        raw_data = f.read()

    file_size = len(raw_data)
    print(f"文件大小: {file_size} 字节 ({file_size/1024:.2f} KB)\n")

    # 查找协议头
    header_start = raw_data.find(b'IMG_START,')
    print(f"IMG_START 位置: {header_start}")

    if header_start == -1:
        print("ERROR: 未找到IMG_START协议头")
        print("\n文件前100字节:")
        print(raw_data[:100])
        return

    # 查找协议头结束
    header_end = raw_data.find(b'\r\n', header_start)
    print(f"协议头结束位置: {header_end}")

    if header_end == -1:
        print("ERROR: 未找到协议头结束符")
        return

    # 提取并显示协议头
    header_bytes = raw_data[header_start:header_end]
    header_str = header_bytes.decode('ascii', errors='replace')
    print(f"\n协议头内容: {header_str}")

    # 解析协议头
    header_parts = header_str.split(',')
    print(f"\n协议头解析:")
    for i, part in enumerate(header_parts):
        print(f"   [{i}] {part}")

    if len(header_parts) >= 6:
        width = int(header_parts[1])
        height = int(header_parts[2])
        bpp = int(header_parts[3])
        img_type = int(header_parts[4])
        crc = header_parts[5]

        expected_size = width * height * (bpp // 8)

        print(f"\n图像参数:")
        print(f"   宽度: {width}")
        print(f"   高度: {height}")
        print(f"   色深: {bpp} 位")
        print(f"   类型: {img_type}")
        print(f"   CRC: {crc}")
        print(f"   期望数据大小: {expected_size} 字节")

    # 查找数据开始位置
    data_start = header_end + 2  # 跳过 \r\n
    print(f"\n数据开始位置: {data_start}")

    # 查找协议尾
    footer_start = raw_data.find(b'\r\nIMAGE_END\r\n', data_start)
    print(f"协议尾位置: {footer_start}")

    if footer_start == -1:
        print("ERROR: 未找到IMAGE_END协议尾")
        print("\n从数据开始到文件末尾的内容:")
        print(raw_data[data_start:data_start+200])
        data_size = file_size - data_start
    else:
        data_size = footer_start - data_start

    print(f"\n数据段信息:")
    print(f"   数据起始: {data_start}")
    print(f"   数据长度: {data_size} 字节")

    if len(header_parts) >= 6:
        print(f"   期望长度: {expected_size} 字节")
        print(f"   差异: {data_size - expected_size} 字节")

        if data_size < expected_size:
            print(f"\n警告: 数据不完整!")
            print(f"   完成度: {data_size/expected_size*100:.2f}%")

    # 显示数据的前50字节（十六进制）
    if data_size > 0:
        data_end = min(data_start + 50, file_size)
        hex_data = raw_data[data_start:data_end].hex(' ', 2)
        print(f"\n数据前50字节 (HEX):")
        print(f"   {hex_data}")

    # 显示文件末尾
    if footer_start != -1:
        footer = raw_data[footer_start:footer_start+20]
        print(f"\n协议尾内容: {footer}")
    else:
        # 显示文件最后50字节
        print(f"\n文件最后50字节:")
        print(f"   {raw_data[-50:].hex(' ', 2)}")

    print(f"\n{'='*60}\n")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        diagnose_dat_file(sys.argv[1])
    else:
        filepath = input("请输入DAT文件路径: ").strip()
        if filepath:
            diagnose_dat_file(filepath)
        else:
            print("未输入文件路径")
