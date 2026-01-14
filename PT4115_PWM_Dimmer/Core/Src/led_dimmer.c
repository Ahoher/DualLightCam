/* USER CODE BEGIN 0 */
/* LED PWM 调光驱动 - PT4115 DIM 引脚控制 */

#include "tim.h"
#include "led_dimmer.h"

/* 私有变量 */
static uint16_t ir_duty = 0;             // 红外光亮度 (TIM1_CH1 - PA8)
static uint16_t vis_duty = 0;            // 可见光亮度 (TIM1_CH2 - PA9)
static uint8_t light_mode = 0;           // 0:红外光, 1:可见光
static uint8_t strobe_mode = 0;          // 0:正常, 1:频闪
static uint8_t strobe_freq = 5;          // 频闪频率 (Hz)
static uint32_t strobe_timer = 0;        // 频闪定时器
static uint8_t strobe_state = 0;         // 0:灭, 1:亮

static uint8_t last_mode_state = 1;      // Mode 按键上次状态
static uint8_t last_up_state = 1;        // Up 按键上次状态
static uint8_t last_down_state = 1;      // Down 按键上次状态
static uint32_t mode_press_time = 0;     // Mode 按键按下时间

/* 外部信号检测相关 */
static uint8_t exti_active = 0;          // 外部信号激活标志（禁用按键）

/* 宏定义 */
#define DUTY_STEP 50                     // 每次按键亮度变化值
#define DUTY_MIN 0                       // 最小亮度
#define DUTY_MAX 1000                    // 最大亮度
#define FREQ_STEP 1                      // 每次按键频率变化值
#define FREQ_MIN 1                       // 最小频率
#define FREQ_MAX 20                      // 最大频率
#define LONG_PRESS_TIME 500              // 长按阈值(ms)

/**
  * @brief  LED 调光初始化
  * @note   在 main() 中 MX_TIM1_Init() 之后调用
  * @retval None
  */
void LED_Init(void)
{
    // 确保 PWM 输出关闭（初始占空比 0）
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);  // 红外光 PA8
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);  // 可见光 PA9

    // 启动 PWM 输出（两个通道）
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);

    // 初始化变量
    ir_duty = 0;
    vis_duty = 0;
    light_mode = 0;  // 默认红外光模式
}

/**
  * @brief  设置 LED 亮度（立即生效）
  * @param  duty: 占空比值 (0~999)
  * @note   duty < 30 时直接关闭输出
  * @retval 0: 成功, 1: 参数错误
  */
uint8_t LED_SetBrightness(uint16_t duty)
{
    // 参数范围检查
    if (duty > 999) {
        return 1;  // 参数错误
    }

    // 根据当前模式设置对应的 PWM 通道
    if (light_mode == 0) {
        // 红外光模式 - PA8 (TIM1_CH1)
        if (duty < 30) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        } else {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, duty);
        }
        ir_duty = duty;
    } else {
        // 可见光模式 - PA9 (TIM1_CH2)
        if (duty < 30) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        } else {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, duty);
        }
        vis_duty = duty;
    }

    return 0;  // 成功
}

/**
  * @brief  立即关闭 LED
  * @retval None
  */
void LED_TurnOff(void)
{
    if (light_mode == 0) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        ir_duty = 0;
    } else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        vis_duty = 0;
    }
}

/**
  * @brief  立即全亮 LED
  * @retval None
  */
void LED_TurnOn(void)
{
    if (light_mode == 0) {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 999);
        ir_duty = 999;
    } else {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 999);
        vis_duty = 999;
    }
}

/**
  * @brief  按键扫描（主循环调用）
  * @note   检测按键按下并调用相应处理函数
  * @retval None
  */
void LED_KeyScan(void)
{
    // === 外部信号检测（优先级最高）===
    uint8_t pa1_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1);
    uint8_t pa2_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);

    // PA1=1, PA2=1 → 都关闭
    if (pa1_state == 1 && pa2_state == 1) {
        // 关闭两个通道
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        exti_active = 0;  // 允许按键
    }
    // PA1=1, PA2=0 → 红外光
    else if (pa1_state == 1 && pa2_state == 0) {
        LED_EXTI_IR_Pulse();
        exti_active = 1;  // 禁用按键
    }
    // PA1=0, PA2=1 → 可见光
    else if (pa1_state == 0 && pa2_state == 1) {
        LED_EXTI_Vis_Pulse();
        exti_active = 1;  // 禁用按键
    }
    // PA1=0, PA2=0 → 保持当前状态，允许按键
    else {
        exti_active = 0;
    }

    // === 按键处理（仅在外部信号为 11 或 00 时）===
    if (!exti_active) {
        uint8_t mode_state = HAL_GPIO_ReadPin(Key_Mode_GPIO_Port, Key_Mode_Pin);
        uint8_t up_state = HAL_GPIO_ReadPin(Key_Up_GPIO_Port, Key_Up_Pin);
        uint8_t down_state = HAL_GPIO_ReadPin(Key_Down_GPIO_Port, Key_Down_Pin);

        // Mode 按键处理（支持长按/短按）
        if (last_mode_state == 1 && mode_state == 0) {
            // 按键按下，记录时间
            mode_press_time = HAL_GetTick();
        } else if (last_mode_state == 0 && mode_state == 1) {
            // 按键释放，判断长按/短按
            uint32_t press_duration = HAL_GetTick() - mode_press_time;

            if (press_duration >= LONG_PRESS_TIME) {
                // 长按：切换频闪/正常模式
                LED_ToggleStrobeMode();
            } else {
                // 短按：切换红外/可见光（仅在正常模式）
                if (!strobe_mode) {
                    LED_KeyMode_Handler();
                }
            }
        }
        last_mode_state = mode_state;

        // Up 按键（下降沿检测）
        if (last_up_state == 1 && up_state == 0) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(Key_Up_GPIO_Port, Key_Up_Pin) == 0) {
                LED_KeyUp_Handler();
            }
        }
        last_up_state = up_state;

        // Down 按键（下降沿检测）
        if (last_down_state == 1 && down_state == 0) {
            HAL_Delay(20);
            if (HAL_GPIO_ReadPin(Key_Down_GPIO_Port, Key_Down_Pin) == 0) {
                LED_KeyDown_Handler();
            }
        }
        last_down_state = down_state;
    }

    // 频闪处理（在主循环中调用）
    if (strobe_mode) {
        LED_StrobeProcess();
    }
}

/**
  * @brief  Mode 按键处理（切换红外/可见光模式）
  * @note   0:红外光(PA8), 1:可见光(PA9)
  * @retval None
  */
void LED_KeyMode_Handler(void)
{
    // 先完全关闭两个通道，确保干净切换
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

    // 等待确保 PWM 周期完成
    HAL_Delay(2);

    // 切换模式
    light_mode = !light_mode;

    // 只打开当前模式的通道
    if (light_mode == 0) {
        // 红外光模式 - PA8
        if (ir_duty >= 30) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ir_duty);
        }
    } else {
        // 可见光模式 - PA9
        if (vis_duty >= 30) {
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, vis_duty);
        }
    }
}

/**
  * @brief  Up 按键处理（频闪模式调频率，正常模式调亮度）
  * @retval None
  */
void LED_KeyUp_Handler(void)
{
    if (strobe_mode) {
        // 频闪模式：调整频率
        if (strobe_freq + FREQ_STEP <= FREQ_MAX) {
            strobe_freq += FREQ_STEP;
        } else {
            strobe_freq = FREQ_MAX;
        }
    } else {
        // 正常模式：调整亮度
        if (light_mode == 0) {
            // 红外光模式 - 只操作 CH1
            if (ir_duty + DUTY_STEP <= DUTY_MAX) {
                ir_duty += DUTY_STEP;
            } else {
                ir_duty = DUTY_MAX;
            }

            // 确保 CH2 为 0
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

            // 更新 CH1
            if (ir_duty < 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ir_duty);
            }
        } else {
            // 可见光模式 - 只操作 CH2
            if (vis_duty + DUTY_STEP <= DUTY_MAX) {
                vis_duty += DUTY_STEP;
            } else {
                vis_duty = DUTY_MAX;
            }

            // 确保 CH1 为 0
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

            // 更新 CH2
            if (vis_duty < 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, vis_duty);
            }
        }
    }
}

/**
  * @brief  Down 按键处理（频闪模式调频率，正常模式调亮度）
  * @retval None
  */
void LED_KeyDown_Handler(void)
{
    if (strobe_mode) {
        // 频闪模式：调整频率
        if (strobe_freq > FREQ_MIN + FREQ_STEP) {
            strobe_freq -= FREQ_STEP;
        } else {
            strobe_freq = FREQ_MIN;
        }
    } else {
        // 正常模式：调整亮度
        if (light_mode == 0) {
            // 红外光模式 - 只操作 CH1
            if (ir_duty >= DUTY_STEP + 30) {
                ir_duty -= DUTY_STEP;
            } else {
                ir_duty = 0;
            }

            // 确保 CH2 为 0
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

            // 更新 CH1
            if (ir_duty < 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ir_duty);
            }
        } else {
            // 可见光模式 - 只操作 CH2
            if (vis_duty >= DUTY_STEP + 30) {
                vis_duty -= DUTY_STEP;
            } else {
                vis_duty = 0;
            }

            // 确保 CH1 为 0
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

            // 更新 CH2
            if (vis_duty < 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, vis_duty);
            }
        }
    }
}

/**
  * @brief  切换频闪模式
  * @note   长按 Mode 按键调用
  * @retval None
  */
void LED_ToggleStrobeMode(void)
{
    strobe_mode = !strobe_mode;

    if (strobe_mode) {
        // 进入频闪模式：关闭当前输出，准备频闪
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        strobe_state = 0;
        strobe_timer = HAL_GetTick();
    } else {
        // 退出频闪模式：恢复当前模式的亮度
        if (light_mode == 0) {
            // 恢复红外光
            if (ir_duty >= 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ir_duty);
            }
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        } else {
            // 恢复可见光
            if (vis_duty >= 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, vis_duty);
            }
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        }
    }
}

/**
  * @brief  频闪处理函数
  * @note   在 LED_KeyScan() 中自动调用
  * @retval None
  */
void LED_StrobeProcess(void)
{
    uint32_t current_time = HAL_GetTick();
    uint32_t strobe_period = 1000 / strobe_freq;  // 频闪周期(ms)
    uint32_t half_period = strobe_period / 2;      // 半周期(ms)

    // 检查是否到达切换时间
    if (current_time - strobe_timer >= half_period) {
        strobe_timer = current_time;
        strobe_state = !strobe_state;

        // 根据当前模式和频闪状态更新 PWM
        if (light_mode == 0) {
            // 红外光模式 - PA8
            if (strobe_state && ir_duty >= 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, ir_duty);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
            }
            // 确保 PA9 关闭
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
        } else {
            // 可见光模式 - PA9
            if (strobe_state && vis_duty >= 30) {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, vis_duty);
            } else {
                __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);
            }
            // 确保 PA8 关闭
            __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
        }
    }
}

/**
  * @brief  外部信号红外光补光处理
  * @note   PA1=1, PA2=0 时调用，50% PWM 补光
  * @retval None
  */
void LED_EXTI_IR_Pulse(void)
{
    // 关闭频闪模式（如果正在频闪）
    if (strobe_mode) {
        strobe_mode = 0;
    }

    // 关闭可见光，确保只有红外光
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 0);

    // 红外光 50% 补光 (500/1000)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 1000);
}

/**
  * @brief  外部信号可见光补光处理
  * @note   PA1=0, PA2=1 时调用，50% PWM 补光
  * @retval None
  */
void LED_EXTI_Vis_Pulse(void)
{
    // 关闭频闪模式（如果正在频闪）
    if (strobe_mode) {
        strobe_mode = 0;
    }

    // 关闭红外光，确保只有可见光
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);

    // 可见光 50% 补光 (500/1000)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, 500);
}

/* USER CODE END 0 */
