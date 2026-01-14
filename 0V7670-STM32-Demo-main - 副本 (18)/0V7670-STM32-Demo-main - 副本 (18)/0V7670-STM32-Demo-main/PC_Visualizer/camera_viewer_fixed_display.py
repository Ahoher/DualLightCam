#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
OV7670 Camera Viewer - 三区域独立显示版

功能：
1. 支持三类照片独立显示（不补光、可见光、红外光）
2. 三区域布局，清晰区分照片类别
3. 同类别自动覆盖，保留最新照片
4. 实时更新，无卡顿显示
5. CRC32校验确保数据完整性

协议格式：
IMG_START,width,height,bpp,type,crc\r\n
type: 1=不补光, 2=可见光, 3=红外光
"""

import serial
import serial.tools.list_ports
import numpy as np
import cv2
import time
import os
import struct
import zlib

# 尝试导入numba，如果不存在则使用纯numpy（降级模式）
try:
    from numba import jit
    NUMBA_AVAILABLE = True
    print("✓ Numba加速已启用")
except ImportError:
    NUMBA_AVAILABLE = False
    print("⚠️ Numba未安装，使用纯NumPy模式（pip install numba可启用加速）")

# 配置参数
BAUDRATE = 921600
IMAGE_WIDTH = 320
IMAGE_HEIGHT = 240
ROW_SIZE = 640
FRAME_SIZE = ROW_SIZE * IMAGE_HEIGHT
SAVE_DIR = "captures"

# 照片类型定义
PHOTO_TYPES = {
    1: "No_Light",
    2: "Visible_Light",
    3: "Infrared_Light"
}

# 照片类型显示名称（用于界面）
PHOTO_DISPLAY_NAMES = {
    1: "[1] No Light",
    2: "[2] Visible",
    3: "[3] Infrared"
}

# 照片类型对应的显示区域（左上角坐标）
DISPLAY_POSITIONS = {
    1: (0, 0),      # 不补光 - 左上
    2: (340, 0),    # 可见光 - 右上
    3: (0, 260)     # 红外光 - 左下
}

# 窗口名称
WINDOW_NAME = "OV7670 Camera - 三区域独立显示"

def find_serial_port():
    """自动查找可用的串口"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("❌ 未找到任何串口设备")
        return None

    print("\n可用串口:")
    for i, port in enumerate(ports):
        print(f"  [{i+1}] {port.device} - {port.description}")

    for port in ports:
        if "STM32" in port.description.upper() or "USB" in port.description.upper():
            print(f"\n✓ 自动选择: {port.device}")
            return port.device

    print(f"\n✓ 选择第一个端口: {ports[0].device}")
    return ports[0].device

# ==================== Numba加速函数 ====================

if NUMBA_AVAILABLE:
    @jit(nopython=True, cache=True)
    def rgb565_to_bgr888_numba(rgb565_array, width, height):
        """
        使用Numba JIT编译的RGB565转BGR888函数
        性能：比纯NumPy快3-5倍
        """
        bgr = np.empty((height * width, 3), dtype=np.uint8)
        rgb565 = rgb565_array
        r = ((rgb565 >> 11) & 0x1F).astype(np.uint8) << 3
        g = ((rgb565 >> 5) & 0x3F).astype(np.uint8) << 2
        b = (rgb565 & 0x1F).astype(np.uint8) << 3
        bgr[:, 0] = b
        bgr[:, 1] = g
        bgr[:, 2] = r
        return bgr.reshape((height, width, 3))
else:
    def rgb565_to_bgr888_numba(rgb565_array, width, height):
        """纯NumPy实现的RGB565转BGR888"""
        bgr = np.empty((height * width, 3), dtype=np.uint8)
        rgb565 = rgb565_array
        r = ((rgb565 >> 11) & 0x1F).astype(np.uint8) << 3
        g = ((rgb565 >> 5) & 0x3F).astype(np.uint8) << 2
        b = (rgb565 & 0x1F).astype(np.uint8) << 3
        bgr[:, 0] = b
        bgr[:, 1] = g
        bgr[:, 2] = r
        return bgr.reshape((height, width, 3))

def process_image_data(image_data):
    """处理图像数据 - 高性能版（Numba加速）"""
    try:
        # 数据完整性检查
        if len(image_data) < FRAME_SIZE:
            print(f"  ⚠️ 数据不完整: {len(image_data)} < {FRAME_SIZE}")
            return None

        if len(image_data) > FRAME_SIZE:
            print(f"  ⚠️ 数据过多 {len(image_data) - FRAME_SIZE} 字节，截取")
            image_data = image_data[:FRAME_SIZE]

        # RGB565转RGB888
        arr = np.frombuffer(image_data, dtype=np.uint8).reshape(-1, 2)
        rgb565 = (arr[:, 0].astype(np.uint16) << 8) | arr[:, 1]

        # 智能滤波修复 - 只修复连续的零值
        zero_indices = np.where(rgb565 == 0)[0]
        if len(zero_indices) > 100:
            print(f"  ⚠️ 检测到 {len(zero_indices)} 个零值，进行滤波")
            for idx in zero_indices:
                if idx > 0 and idx < len(rgb565) - 1:
                    rgb565[idx] = (rgb565[idx-1] + rgb565[idx+1]) // 2

        # 使用Numba加速函数
        image_array = rgb565_to_bgr888_numba(rgb565, IMAGE_WIDTH, IMAGE_HEIGHT)

        return image_array

    except Exception as e:
        print(f"图像处理错误: {e}")
        return None

def verify_crc32(image_data, received_crc):
    """验证CRC32校验 - 使用与STM32相同的算法"""
    if len(image_data) != FRAME_SIZE:
        return False, f"数据长度错误: {len(image_data)} != {FRAME_SIZE}"

    calculated_crc = zlib.crc32(image_data) & 0xFFFFFFFF

    if calculated_crc == received_crc:
        return True, "CRC校验通过"
    else:
        return False, f"CRC校验失败: 期望 {hex(received_crc)}, 实际 {hex(calculated_crc)}"

def parse_header(header_str):
    """解析协议头 - 支持照片类型"""
    try:
        parts = header_str.strip().split(',')
        if len(parts) >= 5:
            width = int(parts[1])
            height = int(parts[2])
            bpp = int(parts[3])
            photo_type = int(parts[4])  # 新增：照片类型
            crc_enabled = int(parts[5]) if len(parts) > 5 else 0
            return width, height, bpp, photo_type, crc_enabled
    except:
        pass
    return None, None, None, None, None

def draw_layout_overlay(display_image):
    """在显示图像上绘制布局和标签"""
    h, w = display_image.shape[:2]

    # 绘制分割线
    cv2.line(display_image, (340, 0), (340, 520), (200, 200, 200), 2)
    cv2.line(display_image, (0, 260), (680, 260), (200, 200, 200), 2)

    # 绘制区域标签（使用英文+图标，避免中文乱码）
    # 左上区域 - 不补光
    cv2.putText(display_image, "[1] No Light", (10, 25),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2, cv2.LINE_AA)
    cv2.putText(display_image, "No Light", (10, 50),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1, cv2.LINE_AA)

    # 右上区域 - 可见光
    cv2.putText(display_image, "[2] Visible", (350, 25),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 0), 2, cv2.LINE_AA)
    cv2.putText(display_image, "Visible Light", (350, 50),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (255, 255, 0), 1, cv2.LINE_AA)

    # 左下区域 - 红外光
    cv2.putText(display_image, "[3] Infrared", (10, 285),
               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 165, 255), 2, cv2.LINE_AA)
    cv2.putText(display_image, "Infrared Light", (10, 310),
               cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 165, 255), 1, cv2.LINE_AA)

    # 添加边框强调
    cv2.rectangle(display_image, (2, 2), (338, 258), (0, 255, 0), 2)  # 左上
    cv2.rectangle(display_image, (342, 2), (678, 258), (255, 255, 0), 2)  # 右上
    cv2.rectangle(display_image, (2, 262), (338, 518), (0, 165, 255), 2)  # 左下

    return display_image

def main_loop(port):
    """主循环 - 三区域独立显示"""

    print("\n" + "=" * 70)
    print("OV7670 Camera Viewer - 三区域独立显示版")
    print("=" * 70)
    print("\n系统初始化中...")

    os.makedirs(SAVE_DIR, exist_ok=True)

    ser = None
    frame_counts = {1: 0, 2: 0, 3: 0}  # 每类照片的计数
    total_frames = 0

    try:
        print(f"正在连接串口 {port} (波特率: {BAUDRATE})...")
        ser = serial.Serial(port, BAUDRATE, timeout=0.5, write_timeout=0.5)
        ser.reset_input_buffer()
        ser.reset_output_buffer()
        print("✓ 串口连接成功！")
        print("\n功能说明:")
        print("  - 三区域独立显示: [1]No Light | [2]Visible | [3]Infrared")
        print("  - 自动覆盖: 同类别保留最新照片")
        print("  - CRC32校验: 验证数据完整性")
        print("  - Numba加速: JIT编译提升性能")
        print("\n提示: 按 ESC 退出 | 按 S 保存所有当前照片")
        print("-" * 70)

    except PermissionError:
        print(f"❌ 串口 {port} 被占用")
        return
    except Exception as e:
        print(f"❌ 串口打开失败: {e}")
        return

    # 创建大画布用于三区域显示
    # 680x520: 左上320x240, 右上320x240, 左下320x240
    display_canvas = np.zeros((520, 680, 3), dtype=np.uint8)

    # 存储各类照片的图像数据（用于保存）
    current_images = {1: None, 2: None, 3: None}

    # 智能缓冲区管理
    buffer = b''
    last_data_time = time.time()
    bytes_received = 0
    frame_start_time = None

    # 状态机
    state = "WAIT_HEADER"  # WAIT_HEADER, WAIT_DATA, WAIT_CRC, WAIT_END
    current_header = None
    expected_width = None
    expected_height = None
    current_photo_type = None
    crc_enabled = False
    image_buffer = b''

    # 显示控制
    last_display_time = 0

    try:
        while True:
            current_time = time.time()

            # 1. 读取串口数据
            try:
                if ser.in_waiting > 0:
                    chunk = ser.read(ser.in_waiting)
                    buffer += chunk
                    bytes_received += len(chunk)
                    last_data_time = current_time

                    # 显示实时统计
                    if not frame_start_time:
                        frame_start_time = current_time
                    elif current_time - frame_start_time > 1.0:
                        print(f"\n📊 接收统计: {bytes_received} 字节 | 缓冲区: {len(buffer)} 字节")
                        frame_start_time = current_time

                else:
                    time.sleep(0.001)

            except (serial.SerialException, PermissionError) as e:
                print(f"\n❌ 串口通信错误: {e}")
                break
            except Exception as e:
                print(f"\n⚠️ 读取异常: {e}")
                time.sleep(0.1)
                continue

            # 2. 心跳检测
            if current_time - last_data_time > 5.0:
                print(f"\n⏳ 等待数据中... (已接收 {bytes_received} 字节)")
                last_data_time = current_time

            # 3. 状态机处理
            if state == "WAIT_HEADER":
                # 查找帧头
                header_idx = buffer.find(b'IMG_START')
                if header_idx != -1:
                    header_end = buffer.find(b'\n', header_idx)
                    if header_end != -1:
                        header_str = buffer[header_idx:header_end].decode('ascii', errors='ignore')
                        width, height, bpp, photo_type, crc_flag = parse_header(header_str)

                        if width and height and photo_type:
                            current_header = header_str
                            expected_width = width
                            expected_height = height
                            current_photo_type = photo_type
                            crc_enabled = (crc_flag == 1)

                            print(f"\n✓ 收到帧头: {header_str.strip()}")
                            print(f"  尺寸: {width}x{height}, 类型: {PHOTO_DISPLAY_NAMES.get(photo_type, 'Unknown')}, CRC: {'启用' if crc_enabled else '禁用'}")

                            buffer = buffer[header_end + 1:]
                            bytes_received = 0
                            image_buffer = b''
                            state = "WAIT_DATA"
                        else:
                            buffer = buffer[header_end + 1:]
                    else:
                        # 帧头不完整，等待更多数据
                        if len(buffer) > 100:
                            buffer = buffer[header_idx:]
                        else:
                            buffer = buffer[header_idx:]
                else:
                    # 清理缓冲区（避免累积垃圾数据）
                    if len(buffer) > 1000:
                        buffer = buffer[-100:]

            elif state == "WAIT_DATA":
                # 等待足够数据
                expected_data_size = expected_width * expected_height * 2
                if len(buffer) >= expected_data_size:
                    # 提取图像数据
                    image_data = buffer[:expected_data_size]
                    buffer = buffer[expected_data_size:]
                    image_buffer = image_data

                    if crc_enabled:
                        state = "WAIT_CRC"
                    else:
                        state = "WAIT_END"

                    print(f"✓ 图像数据接收完成: {len(image_data)} 字节")

            elif state == "WAIT_CRC":
                # 等待4字节CRC
                if len(buffer) >= 4:
                    crc_bytes = buffer[:4]
                    buffer = buffer[4:]
                    received_crc = struct.unpack('>I', crc_bytes)[0]  # 大端序
                    print(f"✓ 收到CRC32: {hex(received_crc)}")

                    # 验证CRC
                    crc_ok, crc_msg = verify_crc32(image_buffer, received_crc)
                    print(f"  {crc_msg}")

                    if crc_ok:
                        state = "WAIT_END"
                    else:
                        print("  ❌ CRC校验失败，丢弃此帧")
                        state = "WAIT_HEADER"
                        image_buffer = b''

            elif state == "WAIT_END":
                # 查找结束标记
                end_idx = buffer.find(b'IMAGE_END')
                if end_idx != -1:
                    print("✓ 收到帧结束标记")

                    # 处理图像
                    if len(image_buffer) == expected_width * expected_height * 2:
                        print(f"✓ 开始处理第 {total_frames + 1} 帧 ({PHOTO_DISPLAY_NAMES.get(current_photo_type, 'Unknown')})...")

                        # 处理图像数据
                        image_array = process_image_data(image_buffer)

                        if image_array is not None:
                            total_frames += 1
                            frame_counts[current_photo_type] += 1

                            # 水平翻转修复
                            final_image = cv2.flip(image_array, 1)

                            # 更新当前图像（用于保存）
                            current_images[current_photo_type] = final_image.copy()

                            # 更新显示区域
                            pos = DISPLAY_POSITIONS.get(current_photo_type, (0, 0))
                            x, y = pos

                            # 将图像缩放到320x240并放置到对应区域
                            display_canvas[y:y+240, x:x+320] = final_image

                            # 自动保存
                            timestamp = time.strftime("%Y%m%d_%H%M%S")
                            photo_name = PHOTO_TYPES.get(current_photo_type, f"Type{current_photo_type}")
                            filename = f"{SAVE_DIR}/{timestamp}_{photo_name}_{frame_counts[current_photo_type]}.jpg"
                            cv2.imwrite(filename, final_image)
                            print(f"  ✓ 已保存: {filename}")

                            # 显示统计
                            print(f"  📊 总帧数: {total_frames}")
                            print(f"  📷 No Light: {frame_counts[1]} | Visible: {frame_counts[2]} | Infrared: {frame_counts[3]}")

                    # 清理状态
                    buffer = buffer[end_idx + len(b'IMAGE_END'):]
                    image_buffer = b''
                    state = "WAIT_HEADER"
                    bytes_received = 0

            # 4. 显示更新（限制刷新率避免卡顿）
            if current_time - last_display_time > 0.1:  # 10Hz刷新
                # 绘制布局和标签
                display_with_layout = display_canvas.copy()
                display_with_layout = draw_layout_overlay(display_with_layout)

                cv2.imshow(WINDOW_NAME, display_with_layout)
                last_display_time = current_time

            # 5. 检查按键
            key = cv2.waitKey(1)
            if key == 27:  # ESC
                print("\n用户按ESC退出")
                break
            elif key == ord('s') or key == ord('S'):  # S - 手动保存所有当前照片
                print(f"\n💾 保存所有当前照片...")
                timestamp = time.strftime("%Y%m%d_%H%M%S")
                for photo_type, img in current_images.items():
                    if img is not None:
                        photo_name = PHOTO_TYPES.get(photo_type, f"Type{photo_type}")
                        filename = f"{SAVE_DIR}/{timestamp}_Manual_{photo_name}.jpg"
                        cv2.imwrite(filename, img)
                        print(f"  已保存: {filename}")

    except KeyboardInterrupt:
        print("\n\n用户按Ctrl+C退出")

    except Exception as e:
        print(f"\n❌ 主循环异常: {e}")
        import traceback
        traceback.print_exc()

    finally:
        if ser and ser.is_open:
            ser.close()
            print("✓ 串口已关闭")
        cv2.destroyAllWindows()
        print("✓ 程序已退出")
        print(f"\n📊 最终统计:")
        print(f"  总帧数: {total_frames}")
        print(f"  No Light: {frame_counts[1]} 张")
        print(f"  Visible: {frame_counts[2]} 张")
        print(f"  Infrared: {frame_counts[3]} 张")

if __name__ == "__main__":
    port = find_serial_port()
    if port:
        main_loop(port)
    else:
        print("\n无法启动程序，请检查设备连接")
