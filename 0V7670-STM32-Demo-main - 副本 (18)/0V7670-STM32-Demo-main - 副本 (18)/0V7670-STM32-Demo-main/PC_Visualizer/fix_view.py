#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
修复版查看器 - 处理不完整协议头
专门解决花屏问题
"""

import cv2
import numpy as np
from pathlib import Path
import sys

def find_data_start_flexible(data):
    """灵活查找数据起始位置"""

    # 方法1: 查找完整的IMG_START...\r\n
    header_start = data.find(b'IMG_START')
    if header_start != -1:
        # 查找换行符
        for i in range(header_start, header_start + 50):
            if i + 1 < len(data) and data[i] == 13 and data[i+1] == 10:  # \r\n
                print("找到完整协议头 \\r\\n 在位置:", i+1)
                return i + 1, True

            if data[i] == 10:  # 只有\n
                print("找到协议头 \\n 在位置:", i+1)
                return i + 1, True

        # 如果没找到换行符，尝试查找协议头结束（逗号后的数字）
        header = data[header_start:header_start+30]
        print("协议头内容:", header)

        # 查找协议头结束位置（通常是1,1后面）
        # IMG_START,320,240,16,1,1
        comma_count = 0
        pos = header_start
        while pos < len(data) and pos < header_start + 30:
            if data[pos] == ord(','):
                comma_count += 1
                if comma_count == 5:  # 第5个逗号后
                    # 跳过数字1
                    pos += 2
                    # 跳过可能的\r\n
                    while pos < len(data) and data[pos] in [13, 10]:
                        pos += 1
                    print("估计数据起始位置:", pos)
                    return pos, False
            pos += 1

    # 方法2: 尝试固定位置
    print("尝试固定位置26...")
    return 26, False

def analyze_data_quality(data, start):
    """分析数据质量"""
    if start + 153600 > len(data):
        print("数据不足")
        return False

    img_data = data[start:start+153600]

    # 检查数据分布
    zeros = np.count_nonzero(img_data == 0)
    ffs = np.count_nonzero(img_data == 255)
    total = len(img_data)

    zero_pct = zeros / total * 100
    ff_pct = ffs / total * 100

    print(f"0x00占比: {zero_pct:.2f}%")
    print(f"0xFF占比: {ff_pct:.2f}%")

    # 如果数据几乎全0或全FF，说明位置不对
    if zero_pct > 90 or ff_pct > 90:
        print("数据异常（几乎全0或全FF）")
        return False

    return True

def try_multiple_starts(data):
    """尝试多个起始位置"""
    print("\n尝试多个起始位置...")

    # 常见的起始位置
    candidates = [25, 26, 27, 28, 29, 30, 31, 32, 0]

    for start in candidates:
        if start + 153600 > len(data):
            continue

        print(f"\n尝试位置 {start}:")

        if not analyze_data_quality(data, start):
            continue

        try:
            img_data = data[start:start+153600]
            rgb565 = np.frombuffer(img_data, dtype=np.uint16).reshape(240, 320)

            # 检查RGB565值是否合理
            # RGB565范围：R(0-31), G(0-63), B(0-31)
            # 如果值都超出范围，说明位置不对

            # 取前10个像素检查
            sample = rgb565[:10, :10].flatten()
            r = (sample >> 11) & 0x1F
            g = (sample >> 5) & 0x3F
            b = sample & 0x1F

            # 检查是否有合理的值
            r_ok = np.any((r > 0) & (r < 32))
            g_ok = np.any((g > 0) & (g < 64))
            b_ok = np.any((b > 0) & (b < 32))

            if r_ok and g_ok and b_ok:
                print(f"  RGB值合理: R={r[0]}, G={g[0]}, B={b[0]}")

                # 尝试转换
                r = ((rgb565 >> 11) & 0x1F) << 3
                g = ((rgb565 >> 5) & 0x3F) << 2
                b = (rgb565 & 0x1F) << 3
                rgb888 = np.stack([r, g, b], axis=2).astype(np.uint8)
                bgr = cv2.cvtColor(rgb888, cv2.COLOR_RGB2BGR)

                return start, bgr
            else:
                print(f"  RGB值异常: R={r[0]}, G={g[0]}, B={b[0]}")

        except Exception as e:
            print(f"  失败: {e}")

    return None, None

def fix_view(filepath):
    """修复版查看器"""
    filepath = Path(filepath)

    if not filepath.exists():
        print("文件不存在")
        return False

    with open(filepath, 'rb') as f:
        data = f.read()

    print(f"文件大小: {len(data)} 字节")

    # 查找数据起始位置
    data_start, found_header = find_data_start_flexible(data)
    print(f"数据起始位置: {data_start}")

    # 分析数据质量
    if analyze_data_quality(data, data_start):
        # 尝试直接转换
        try:
            img_data = data[data_start:data_start+153600]
            rgb565 = np.frombuffer(img_data, dtype=np.uint16).reshape(240, 320)
            r = ((rgb565 >> 11) & 0x1F) << 3
            g = ((rgb565 >> 5) & 0x3F) << 2
            b = (rgb565 & 0x1F) << 3
            rgb888 = np.stack([r, g, b], axis=2).astype(np.uint8)
            bgr = cv2.cvtColor(rgb888, cv2.COLOR_RGB2BGR)

            cv2.imshow(f"Result (start={data_start})", bgr)
            print("显示成功！")
            cv2.waitKey(0)
            cv2.destroyAllWindows()

            output = filepath.parent / f"{filepath.stem}_fixed.jpg"
            cv2.imwrite(str(output), bgr)
            print(f"已保存: {output}")
            return True

        except Exception as e:
            print(f"直接转换失败: {e}")

    # 如果失败，尝试多个位置
    print("\n直接转换失败，尝试多个位置...")
    start, bgr = try_multiple_starts(data)

    if start is not None:
        cv2.imshow(f"Result (start={start})", bgr)
        print(f"成功！起始位置: {start}")
        cv2.waitKey(0)
        cv2.destroyAllWindows()

        output = filepath.parent / f"{filepath.stem}_fixed.jpg"
        cv2.imwrite(str(output), bgr)
        print(f"已保存: {output}")
        return True

    print("所有尝试都失败")
    return False

if __name__ == "__main__":
    if len(sys.argv) > 1:
        filepath = sys.argv[1]
    else:
        filepath = input("请输入DAT文件路径: ").strip()

    if filepath:
        fix_view(filepath)
