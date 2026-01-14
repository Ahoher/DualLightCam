#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
STM32 OV7670 DAT文件查看器
用于读取和显示STM32保存的IMG_XXX.DAT图像文件

作者: 浮浮酱 (猫娘工程师)
版本: 1.0
"""

import struct
import numpy as np
import cv2
from pathlib import Path
from typing import Tuple, Dict, Optional


class DATImageLoader:
    """STM32 DAT文件加载器 - 解析并显示自定义协议图像文件"""

    def __init__(self, width: int = 320, height: int = 240):
        """
        初始化加载器

        Args:
            width: 图像宽度 (默认320)
            height: 图像高度 (默认240)
        """
        self.width = width
        self.height = height
        self.bpp = 16  # RGB565格式

    def parse_dat_file(self, filepath: str) -> Dict:
        """
        解析DAT文件，提取图像数据和元信息

        协议格式:
        IMG_START,width,height,bpp,type,crc\r\n
        [RGB565二进制数据]
        \r\nIMAGE_END\r\n

        Args:
            filepath: DAT文件路径

        Returns:
            dict: 包含图像数据和元信息的字典

        Raises:
            ValueError: 文件格式错误或校验失败
        """
        filepath = Path(filepath)
        if not filepath.exists():
            raise FileNotFoundError(f"文件不存在: {filepath}")

        with open(filepath, 'rb') as f:
            raw_data = f.read()

        # 查找协议头
        header_start = raw_data.find(b'IMG_START,')
        if header_start == -1:
            raise ValueError("未找到IMG_START协议头，文件格式可能错误")

        header_end = raw_data.find(b'\r\n', header_start)
        if header_end == -1:
            raise ValueError("未找到协议头结束符")

        # 解析协议头: IMG_START,width,height,bpp,type,crc
        header_str = raw_data[header_start:header_end].decode('ascii')
        header_parts = header_str.split(',')

        if len(header_parts) != 6:
            raise ValueError(f"协议头格式错误: {header_parts}")

        # 提取元信息
        img_start, width, height, bpp, img_type, crc_str = header_parts
        width = int(width)
        height = int(height)
        bpp = int(bpp)
        img_type = int(img_type)
        crc_stored = int(crc_str, 16) if crc_str.startswith('0x') else int(crc_str)

        # 查找数据结束符
        data_start = header_end + 2  # 跳过\r\n
        footer_start = raw_data.find(b'\r\nIMAGE_END\r\n', data_start)
        if footer_start == -1:
            raise ValueError("未找到IMAGE_END协议尾")

        # 提取二进制图像数据
        image_data = raw_data[data_start:footer_start]

        # 验证数据长度（允许153600或153604，后者包含CRC）
        expected_size = width * height * (bpp // 8)
        actual_size = len(image_data)

        if actual_size == expected_size + 4:
            # 数据段包含了CRC，提取纯图像数据
            print(f"WARN:  检测到数据段包含CRC，自动分离...")
            image_data = image_data[:expected_size]
            # CRC在数据段末尾4字节，但我们已经在header中提取了CRC
        elif actual_size != expected_size:
            raise ValueError(
                f"数据长度不匹配: 期望{expected_size}字节, 实际{actual_size}字节"
            )

        # 计算CRC32并验证
        calculated_crc = self._calculate_crc32(image_data)
        if calculated_crc != crc_stored:
            raise ValueError(
                f"CRC校验失败: 文件CRC=0x{crc_stored:08X}, "
                f"计算CRC=0x{calculated_crc:08X}"
            )

        return {
            'width': width,
            'height': height,
            'bpp': bpp,
            'type': img_type,
            'crc': crc_stored,
            'data': image_data,
            'filename': filepath.name,
            'filepath': str(filepath)
        }

    def _calculate_crc32(self, data: bytes) -> int:
        """
        计算CRC32校验值 (与STM32代码一致)

        Args:
            data: 二进制数据

        Returns:
            int: CRC32校验值
        """
        crc = 0xFFFFFFFF
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 1:
                    crc = (crc >> 1) ^ 0xEDB88320
                else:
                    crc >>= 1
        return crc ^ 0xFFFFFFFF

    def rgb565_to_rgb888(self, rgb565_data: bytes, width: int, height: int) -> np.ndarray:
        """
        RGB565 转 RGB888

        RGB565格式: R(5位) G(6位) B(5位) = 16位
        RGB888格式: R(8位) G(8位) B(8位) = 24位

        Args:
            rgb565_data: bytes - RGB565二进制数据
            width: 图像宽度
            height: 图像高度

        Returns:
            numpy.ndarray: RGB888图像数据 (HxWx3)
        """
        # 将bytes转换为numpy数组 (uint16, 小端序)
        rgb565_array = np.frombuffer(rgb565_data, dtype=np.uint16).reshape(height, width)

        # RGB565解码
        # 位布局: [R4 R3 R2 R1 R0 | G5 G4 G3 G2 G1 G0 | B4 B3 B2 B1 B0]
        r = ((rgb565_array >> 11) & 0x1F) << 3  # 取高5位，左移3位扩展到8位
        g = ((rgb565_array >> 5) & 0x3F) << 2   # 取中间6位，左移2位扩展到8位
        b = (rgb565_array & 0x1F) << 3          # 取低5位，左移3位扩展到8位

        # 组合成RGB888 (HxWx3)
        rgb888 = np.stack([r, g, b], axis=2).astype(np.uint8)

        return rgb888

    def get_type_name(self, img_type: int) -> str:
        """
        获取拍照类型名称

        Args:
            img_type: 类型代码

        Returns:
            str: 类型名称
        """
        type_map = {
            0: "无补光 (PA8, PA15)",
            1: "可见光补光 (PC14, PB3)",
            2: "红外补光 (PC15, PB4)",
            3: "自定义模式1",
            4: "自定义模式2",
            5: "自定义模式3"
        }
        return type_map.get(img_type, f"未知模式({img_type})")

    def load_and_display(self, filepath: str, display: bool = True,
                        save_output: Optional[str] = None,
                        window_delay: int = 0) -> Tuple[np.ndarray, Dict]:
        """
        加载DAT文件并显示图像

        Args:
            filepath: DAT文件路径
            display: 是否显示图像窗口
            save_output: 保存路径 (可选, 如 "output.jpg")
            window_delay: 窗口显示延迟(毫秒), 0表示等待按键

        Returns:
            tuple: (RGB888图像数据, 元信息字典)
        """
        # 1. 解析DAT文件
        print(f"\n{'='*60}")
        print(f"FILE: 正在加载: {filepath}")
        print(f"{'='*60}")

        metadata = self.parse_dat_file(filepath)

        # 2. 显示元信息
        print(f"OK: 解析成功!")
        print(f"   文件名:     {metadata['filename']}")
        print(f"   分辨率:     {metadata['width']} x {metadata['height']}")
        print(f"   色深:       {metadata['bpp']} 位")
        print(f"   拍照模式:   {self.get_type_name(metadata['type'])}")
        print(f"   CRC32:      0x{metadata['crc']:08X}")
        print(f"   数据大小:   {len(metadata['data'])} 字节")

        # 3. RGB565转RGB888
        print(f"\n🎨 正在转换RGB565 → RGB888...")
        rgb888 = self.rgb565_to_rgb888(
            metadata['data'],
            metadata['width'],
            metadata['height']
        )
        print(f"OK: 转换完成! 图像形状: {rgb888.shape}")

        # 4. 显示图像
        if display:
            # OpenCV使用BGR格式，需要转换
            rgb888_bgr = cv2.cvtColor(rgb888, cv2.COLOR_RGB2BGR)

            # 创建窗口标题
            type_name = self.get_type_name(metadata['type'])
            title = f"{metadata['filename']} - {type_name}"

            # 显示图像
            cv2.imshow(title, rgb888_bgr)
            print(f"\n🖥️  正在显示图像窗口: {title}")

            if window_delay == 0:
                print("   按任意键关闭窗口...")
                cv2.waitKey(0)
            else:
                print(f"   {window_delay}毫秒后自动关闭...")
                cv2.waitKey(window_delay)

            cv2.destroyAllWindows()

        # 5. 保存输出
        if save_output:
            rgb888_bgr = cv2.cvtColor(rgb888, cv2.COLOR_RGB2BGR)
            cv2.imwrite(save_output, rgb888_bgr)
            print(f"\n💾 已保存: {save_output}")

        print(f"\n{'='*60}\n")

        return rgb888, metadata

    def batch_process(self, folder_path: str, output_folder: str = None,
                     display: bool = False) -> None:
        """
        批量处理文件夹中的所有DAT文件

        Args:
            folder_path: DAT文件夹路径
            output_folder: 输出文件夹路径 (默认为原文件夹)
            display: 是否显示每个图像
        """
        folder = Path(folder_path)
        if not folder.exists():
            raise FileNotFoundError(f"文件夹不存在: {folder_path}")

        dat_files = list(folder.glob("IMG_*.DAT"))
        if not dat_files:
            print(f"WARN:  未找到IMG_*.DAT文件: {folder_path}")
            return

        print(f"\n📂 发现 {len(dat_files)} 个DAT文件")
        print(f"{'='*60}")

        if output_folder:
            output_path = Path(output_folder)
            output_path.mkdir(parents=True, exist_ok=True)
        else:
            output_path = folder

        success_count = 0
        fail_count = 0

        for i, dat_file in enumerate(dat_files, 1):
            try:
                print(f"\n[{i}/{len(dat_files)}] 处理: {dat_file.name}")

                output_name = dat_file.stem + "_converted.jpg"
                output_file = output_path / output_name

                self.load_and_display(
                    filepath=str(dat_file),
                    display=display,
                    save_output=str(output_file),
                    window_delay=500 if display else 0
                )

                success_count += 1

            except Exception as e:
                print(f"❌ 处理失败: {e}")
                fail_count += 1

        print(f"\n{'='*60}")
        print(f"📊 批量处理完成!")
        print(f"   成功: {success_count}")
        print(f"   失败: {fail_count}")
        print(f"{'='*60}")


# ==================== 主程序 ====================

def main():
    """主函数 - 交互式模式"""
    print("="*60)
    print("STM32 OV7670 DAT文件查看器")
    print("浮浮酱 (猫娘工程师) - v1.0")
    print("="*60)

    # 创建加载器
    loader = DATImageLoader(width=320, height=240)

    # 交互式输入
    print("\n请选择模式:")
    print("1. 查看单个DAT文件")
    print("2. 批量处理文件夹")
    print("3. 退出")

    choice = input("\n输入选项 (1/2/3): ").strip()

    if choice == "1":
        filepath = input("请输入DAT文件路径 (如: IMG_101.DAT): ").strip()
        try:
            loader.load_and_display(filepath, display=True, save_output="output.jpg")
            print("OK: 查看完成!")
        except Exception as e:
            print(f"❌ 错误: {e}")

    elif choice == "2":
        folder = input("请输入DAT文件夹路径 (如: ./photos): ").strip()
        output = input("请输入输出文件夹路径 (直接回车使用原文件夹): ").strip()
        display_choice = input("是否显示每个图像? (y/n): ").strip().lower()

        try:
            loader.batch_process(
                folder_path=folder,
                output_folder=output if output else None,
                display=(display_choice == 'y')
            )
        except Exception as e:
            print(f"❌ 错误: {e}")

    elif choice == "3":
        print("👋 再见!")
    else:
        print("❌ 无效选项")


# ==================== 命令行模式 ====================

if __name__ == "__main__":
    import sys

    # 命令行参数模式
    if len(sys.argv) > 1:
        filepath = sys.argv[1]
        loader = DATImageLoader(width=320, height=240)

        try:
            if sys.argv[1] == "--batch" and len(sys.argv) > 2:
                # 批量模式: python dat_viewer.py --batch ./photos
                folder = sys.argv[2]
                output = sys.argv[3] if len(sys.argv) > 3 else None
                loader.batch_process(folder, output, display=False)
            else:
                # 单文件模式: python dat_viewer.py IMG_101.DAT
                loader.load_and_display(filepath, display=True)
        except Exception as e:
            print(f"❌ 错误: {e}")
            sys.exit(1)
    else:
        # 交互模式
        main()
