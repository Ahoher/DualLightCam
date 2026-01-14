#!/usr/bin/env python3
"""
串口通信测试 - 仅测试串口连接
"""

import serial
import serial.tools.list_ports
import time

def find_serial_port():
    """查找串口"""
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("未找到串口")
        return None

    for port in ports:
        if "CH340" in port.description or "USB" in port.description:
            return port.device
    return ports[0].device if ports else None

def test_serial():
    """测试串口通信"""
    port = find_serial_port()
    if not port:
        print("找不到串口设备")
        return

    print(f"尝试连接: {port}")

    try:
        # 尝试不同波特率
        for baud in [115200, 921600]:
            print(f"\n测试波特率: {baud}")
            try:
                ser = serial.Serial(port, baud, timeout=2)
                ser.reset_input_buffer()

                print("等待数据...")
                start = time.time()
                data_received = False

                while time.time() - start < 5:
                    if ser.in_waiting > 0:
                        data = ser.read(ser.in_waiting)
                        print(f"收到: {data.decode('utf-8', errors='ignore')}")
                        data_received = True
                        break
                    time.sleep(0.1)

                ser.close()

                if data_received:
                    print(f"✓ 波特率 {baud} 工作正常！")
                    return
                else:
                    print(f"✗ 波特率 {baud} 无数据")

            except Exception as e:
                print(f"✗ 错误: {e}")

        print("\n建议:")
        print("1. 检查STM32是否已烧录程序")
        print("2. 检查串口线连接 (TX-RX, RX-TX)")
        print("3. 检查波特率设置")
        print("4. 尝试按复位按钮")

    except Exception as e:
        print(f"严重错误: {e}")

if __name__ == "__main__":
    print("=" * 50)
    print("串口通信测试")
    print("=" * 50)
    test_serial()
