#!/usr/bin/env python3
"""
颜色诊断工具 - 分析RGB565解码问题
"""

import numpy as np

def test_color_decoding():
    """测试各种颜色的解码"""

    # 标准RGB565颜色值
    test_colors = {
        "纯红":   (255, 0, 0),
        "纯绿":   (0, 255, 0),
        "纯蓝":   (0, 0, 255),
        "白色":   (255, 255, 255),
        "黑色":   (0, 0, 0),
        "黄色":   (255, 255, 0),
        "青色":   (0, 255, 255),
        "品红":   (255, 0, 255),
        "橙色":   (255, 165, 0),
        "紫色":   (128, 0, 128),
    }

    print("=" * 80)
    print("RGB565 颜色解码诊断")
    print("=" * 80)

    for name, (r, g, b) in test_colors.items():
        # 编码到RGB565
        r5 = r >> 3
        g6 = g >> 2
        b5 = b >> 3
        rgb565 = (r5 << 11) | (g6 << 5) | b5

        # 模拟STM32发送的字节
        byte_high = (rgb565 >> 8) & 0xFF
        byte_low = rgb565 & 0xFF

        print(f"\n{name}: RGB({r:3d},{g:3d},{b:3d})")
        print(f"  RGB565值: 0x{rgb565:04X} = {bin(rgb565):>18s}")
        print(f"  发送字节: [0x{byte_high:02X}, 0x{byte_low:02X}]")

        # PC端当前解码方式
        rgb565_decoded = (byte_high << 8) | byte_low

        r_decoded = ((rgb565_decoded >> 11) & 0x1F) << 3
        g_decoded = ((rgb565_decoded >> 5) & 0x3F) << 2
        b_decoded = ((rgb565_decoded & 0x1F)) << 3

        print(f"  解码结果: RGB({r_decoded:3d},{g_decoded:3d},{b_decoded:3d})")

        # 尝试不同的解码方式
        print("  尝试其他解码:")

        # 方式1: 交换字节序
        rgb565_swapped = (byte_low << 8) | byte_high
        r1 = ((rgb565_swapped >> 11) & 0x1F) << 3
        g1 = ((rgb565_swapped >> 5) & 0x3F) << 2
        b1 = ((rgb565_swapped & 0x1F)) << 3
        print(f"    交换字节: RGB({r1:3d},{g1:3d},{b1:3d})")

        # 方式2: BGR顺序
        # 假设摄像头输出BGR565而不是RGB565
        # 需要交换R和B通道
        r2 = b_decoded
        b2 = r_decoded
        g2 = g_decoded
        print(f"    交换R/B:  RGB({r2:3d},{g2:3d},{b2:3d})")

        # 方式3: 字节高低交换
        rgb565_v3 = (byte_low << 8) | byte_high
        r3 = ((rgb565_v3 >> 11) & 0x1F) << 3
        g3 = ((rgb565_v3 >> 5) & 0x3F) << 2
        b3 = ((rgb565_v3 & 0x1F)) << 3
        print(f"    字节交换: RGB({r3:3d},{g3:3d},{b3:3d})")

        # 方式4: BGR + 字节交换
        r4 = b3
        b4 = r3
        g4 = g3
        print(f"    BGR+交换: RGB({r4:3d},{g4:3d},{b4:3d})")

        # 检查哪个结果最接近
        results = [
            ("当前", r_decoded, g_decoded, b_decoded),
            ("交换字节", r1, g1, b1),
            ("交换R/B", r2, g2, b2),
            ("字节交换", r3, g3, b3),
            ("BGR+交换", r4, g4, b4),
        ]

        best = None
        min_diff = 999999
        for label, rr, gg, bb in results:
            diff = abs(rr - r) + abs(gg - g) + abs(bb - b)
            if diff < min_diff:
                min_diff = diff
                best = label

        if min_diff == 0:
            print(f"  ✓ 最佳匹配: {best}")
        else:
            print(f"  ⚠ 最佳匹配: {best} (误差{min_diff})")

def analyze_actual_data():
    """分析实际接收的数据"""
    print("\n" + "=" * 80)
    print("实际数据分析")
    print("=" * 80)

    # 假设从STM32接收的原始数据
    # 这里需要用户根据实际接收的数据填写
    print("\n请提供实际接收的字节数据:")
    print("例如: 红色物体区域的几个像素值")
    print("格式: [字节高, 字节低] -> RGB565 -> 解码RGB")
    print()
    print("测试方法:")
    print("1. 对准红色物体")
    print("2. 运行程序，观察接收的原始数据")
    print("3. 记录几个像素的字节值")
    print("4. 在这里分析")

if __name__ == "__main__":
    test_color_decoding()
    analyze_actual_data()
