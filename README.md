# DualLightCam - 双光源摄像头项目

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/Ahoher/DualLightCam/blob/main/LICENSE)
[![STM32](https://img.shields.io/badge/STM32-F103C8T6-green.svg)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8t6.html)
[![OV7670](https://img.shields.io/badge/Camera-OV7670-orange.svg)](https://www.ovt.com/sensors/OV7670)

这是一个基于 STM32 的嵌入式视觉项目，包含两个核心子项目：**OV7670 摄像头图像采集系统** 和 **PT4115 双通道 PWM LED 调光系统**。项目支持双光源补光（可见光+红外光）拍照，并可将图像保存到 SD 卡或传输到 PC 显示。

---

## 📋 项目概览

### **项目特点**
- ✅ **双光源补光**：可见光 + 红外光独立控制
- ✅ **多模式拍照**：不补光、可见光补光、红外光补光
- ✅ **双存储方式**：SD 卡本地存储 + PC 实时显示
- ✅ **图像传输协议**：带 CRC32 校验的可靠传输
- ✅ **极简设计**：主循环仅一行代码驱动所有功能
- ✅ **完整文档**：详细的技术文档和使用说明

---

## 🏗️ 项目结构

```
DualLightCam/
├── 0V7670-STM32-Demo-main/     # OV7670 摄像头项目
│   ├── User/                   # 主程序和摄像头驱动
│   ├── Hardware/               # 硬件驱动（OV7670, SD, SPI, UART等）
│   ├── PC_Visualizer/          # PC 端图像显示程序
│   ├── FATFS/                  # FATFS 文件系统
│   ├── Liberary/               # STM32 标准库
│   └── project.uvprojx         # Keil 工程文件
│
└── PT4115_PWM_Dimmer/          # PT4115 PWM 调光项目
    ├── Core/                   # 核心代码
    │   ├── Src/
    │   │   ├── main.c          # 极简主循环
    │   │   └── led_dimmer.c    # 调光核心逻辑
    │   └── Inc/
    │       └── led_dimmer.h    # 调光驱动头文件
    ├── Drivers/                # STM32 HAL 驱动
    └── README.md               # 详细文档
```

---

## 📦 子项目 1：OV7670 摄像头系统

### **项目概述**
基于 STM32F103C8T6 的 OV7670 摄像头图像采集系统，支持 320×240 分辨率 RGB565 图像采集，可通过 SD 卡存储或串口传输到 PC 显示。

### **硬件配置**
| 模块 | 型号/参数 | 说明 |
|------|-----------|------|
| MCU | STM32F103C8T6 | 72MHz, 64KB Flash, 20KB RAM |
| 摄像头 | OV7670 | VGA 分辨率，支持 RGB565 |
| 存储 | MicroSD 卡 | FATFS 文件系统 |
| 补光 | 可见光 + 红外光 | 独立控制 |
| 通信 | UART (921600) | 高速图像传输 |

### **引脚分配**
| 引脚 | 功能 | 连接 |
|------|------|------|
| PA8 | SPI1_SCK | OV7670 SIOC |
| PA9 | SPI1_MOSI | OV7670 SIOD |
| PA10 | GPIO_OUT | OV7670 RESET |
| PA11 | GPIO_OUT | OV7670 PWDN |
| PA12 | GPIO_OUT | OV7670 VSYNC |
| PB12 | GPIO_OUT | FIFO_WEN |
| PB13 | GPIO_OUT | FIFO_RRST |
| PB14 | GPIO_OUT | FIFO_RCLK |
| PB15 | GPIO_OUT | FIFO_WRST |
| PA4 | SPI1_SS | SD 卡 CS |
| PA5 | SPI1_SCK | SD 卡 SCK |
| PA6 | SPI1_MISO | SD 卡 MISO |
| PA7 | SPI1_MOSI | SD 卡 MOSI |
| PA15 | GPIO_OUT | 补光 LED (通用) |
| PB3 | GPIO_OUT | 可见光补光 |
| PB4 | GPIO_OUT | 红外光补光 |
| PB5 | GPIO_OUT | 拍照按键 |
| PB6 | GPIO_OUT | 模式切换按键 |
| PB7 | GPIO_OUT | 补光切换按键 |
| PB8 | GPIO_OUT | SD 卡检测 |
| PB9 | GPIO_OUT | 状态 LED |

### **功能特性**

#### **1. 三种拍照模式**
```c
// 按键1：不补光拍照
Camera_SaveToSD(1);    // 保存到SD卡
Capture_Photo(1);      // 发送到PC

// 按键2：可见光补光拍照
Camera_SaveToSD(2);    // 保存到SD卡
Capture_Photo(2);      // 发送到PC

// 按键3：红外光补光拍照
Camera_SaveToSD(3);    // 保存到SD卡
Capture_Photo(3);      // 发送到PC
```

#### **2. SD 卡存储**
- **文件格式**：`IMG_XXX.DAT` (XXX = 001, 002, ...)
- **数据结构**：
  ```
  协议头: "IMG_START,320,240,16,type,1\r\n"  (26字节)
  图像数据: 320×240×2 = 153,600 字节
  CRC32: 4 字节
  帧尾: "\r\nIMAGE_END\r\n"  (14字节)
  总计: 153,644 字节/张
  ```
- **CRC 校验**：确保数据完整性，兼容 Python `zlib.crc32`

#### **3. PC 实时显示**
- **协议格式**：与 SD 卡存储相同
- **传输速度**：约 77KB/s (921600 波特率)
- **每帧时间**：约 1.7 秒
- **自动保存**：接收完成后自动保存为 JPG

### **PC 端使用**

#### **启动程序**
```bash
cd PC_Visualizer
python camera_viewer_fixed_display.py
```

#### **操作说明**
- **ESC** - 退出程序
- 程序自动接收图像并显示
- 自动保存到 `captures/` 目录

#### **已修复问题**
1. ✅ 数据接收不完整（0/240 → 240/240）
2. ✅ 花白彩条（PC 端滤波修复）
3. ✅ 图像左右反转（水平翻转修复）
4. ✅ 显示卡顿（单线程稳定运行）

---

## 💡 子项目 2：PT4115 PWM 调光系统

### **项目概述**
基于 STM32 的双通道 PWM LED 调光控制系统，用于控制 PT4115 LED 驱动芯片。支持红外光/可见光独立控制、亮度调节和频闪功能。

### **硬件配置**
| 模块 | 型号/参数 | 说明 |
|------|-----------|------|
| MCU | STM32F103C8T6 | 72MHz |
| PWM 定时器 | TIM1 | 高级定时器 |
| PWM 频率 | 1 kHz | 分辨率 0.1% |
| 驱动芯片 | PT4115 | LED 恒流驱动 |
| 按键 | 3 个 | Mode, Up, Down |

### **引脚分配**
| 引脚 | 功能 | 连接 |
|------|------|------|
| PA8 | TIM1_CH1 | PT4115 DIM (红外光) |
| PA9 | TIM1_CH2 | PT4115 DIM (可见光) |
| PB7 | GPIO_IN | Mode 按键 |
| PB8 | GPIO_IN | Up 按键 |
| PB9 | GPIO_IN | Down 按键 |

### **功能特性**

#### **1. 按键功能**
| 按键 | 操作 | 功能 |
|------|------|------|
| **Mode** | 短按 (<500ms) | 切换红外/可见光通道 |
| **Mode** | 长按 (≥500ms) | 进入/退出频闪模式 |
| **Up** | 按下 | 频闪模式：频率+1Hz<br>正常模式：亮度+50 |
| **Down** | 按下 | 频闪模式：频率-1Hz<br>正常模式：亮度-50 |

#### **2. 操作示例**
```bash
# 场景1：红外光调光
上电 → 按 Up 3 次 → 亮度: 0 → 50 → 100 → 150
      → 按 Mode (短按) → 切换到可见光

# 场景2：红外光频闪
上电 → 按 Up 3 次 → 亮度 150
      → 长按 Mode → 进入频闪 (5Hz)
      → 按 Up 2 次 → 频率 7Hz
      → 长按 Mode → 退出频闪
```

#### **3. 核心算法**
```c
// 极简主循环
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM1_Init();

    LED_Init();

    while (1)
    {
        LED_KeyScan();  // 一行代码驱动所有功能
    }
}

// 非阻塞频闪算法
void LED_StrobeProcess(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t period = 1000 / strobe_freq;
    uint32_t half = period / 2;

    if (current_time - strobe_timer >= half)
    {
        strobe_timer = current_time;
        strobe_state = !strobe_state;
        PWM = strobe_state ? duty : 0;
    }
}
```

---

## 🔧 开发环境

### **STM32 开发**
- **IDE**: Keil MDK-ARM v5
- **编译器**: ARMCC/ARMCLANG
- **库**: STM32F1xx HAL 库 / 标准外设库
- **调试**: ST-Link / DAP-Link

### **PC 端开发**
- **语言**: Python 3.8+
- **依赖库**:
  ```bash
  pip install pyserial opencv-python numpy pillow
  ```

---

## 🚀 快速开始

### **步骤 1：硬件连接**
1. 连接 STM32 开发板到 PC (USB-TTL)
2. 插入 MicroSD 卡（格式化为 FAT32）
3. 连接 OV7670 摄像头模块
4. 连接 PT4115 LED 驱动电路（如需要）

### **步骤 2：烧录固件**
```bash
# 使用 Keil 打开工程
# 0V7670-STM32-Demo-main/project.uvprojx
# 或
# PT4115_PWM_Dimmer/MDK-ARM/project.uvprojx

# 编译并下载到 STM32
```

### **步骤 3：PC 端准备**
```bash
# 安装依赖
pip install pyserial opencv-python numpy pillow

# 启动 PC 端程序
cd PC_Visualizer
python camera_viewer_fixed_display.py
```

### **步骤 4：开始使用**
- **OV7670 项目**：按下按键 1/2/3 拍照
- **PT4115 项目**：使用 Mode/Up/Down 按键调光

---

## 📊 性能指标

### **OV7670 摄像头系统**
| 指标 | 数值 | 说明 |
|------|------|------|
| 分辨率 | 320×240 | RGB565 格式 |
| 传输速度 | 77 KB/s | 921600 波特率 |
| 每帧时间 | 1.7 秒 | 含 CRC 校验 |
| SD 卡存储 | 153,644 字节/张 | 含协议头和 CRC |
| 内存占用 | 640 字节 | 行缓冲区 |

### **PT4115 调光系统**
| 指标 | 数值 | 说明 |
|------|------|------|
| PWM 频率 | 1 kHz | 分辨率 0.1% |
| 亮度范围 | 0~999 | 0.1% 步进 |
| 频闪频率 | 1~20 Hz | 1Hz 步进 |
| CPU 占用 | < 11% | 非阻塞设计 |
| 内存占用 | 19 字节 | 静态变量 |

---

## 🔍 技术亮点

### **1. 可靠的数据传输**
```python
# PC 端 CRC 校验
def verify_crc(data, received_crc):
    calculated = zlib.crc32(data) & 0xFFFFFFFF
    return calculated == received_crc
```

### **2. 非阻塞设计**
```c
// 不使用 HAL_Delay()
uint32_t current = HAL_GetTick();
if (current - last_time >= timeout) {
    last_time = current;
    // 执行任务
}
```

### **3. 双通道隔离**
```c
// 切换时完全隔离
if (mode == 0) {
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);  // 关闭另一通道
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
}
```

---

## 🐛 故障排查

### **OV7670 常见问题**
1. **图像花白**：检查 FIFO 读取时序，增加 `delay_us(1)`
2. **SD 卡初始化失败**：检查 SPI 连接，确认卡已格式化
3. **PC 无图像**：检查波特率设置（921600），确认串口正确

### **PT4115 常见问题**
1. **LED 不亮**：检查 PWM 输出（PA8/PA9），确认 PT4115 电源
2. **双通道干扰**：代码已优化隔离，检查硬件连接
3. **按键无响应**：检查 GPIO 配置，确认按键电平

---

## 📄 许可证

本项目基于 MIT 许可证开源，可自由使用和修改。

---

## 📞 联系与贡献

如有问题或建议，欢迎提交 Issue 或 Pull Request。

**作者**: Ahoher
**邮箱**: [GitHub](https://github.com/Ahoher)
**版本**: v1.0
**最后更新**: 2026-01-14

---

## 🎓 学习资源

- [OV7670 数据手册](https://www.ovt.com/sensors/OV7670)
- [STM32F103C8T6 参考手册](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8t6.html)
- [PT4115 LED 驱动芯片手册](https://www.diodes.com/products/power-management/led-drivers/pt4115/)
- [FATFS 文件系统](http://elm-chan.org/fsw/ff/00index_e.html)

---

**Happy Coding! 🚀**
