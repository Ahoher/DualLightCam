/* USER CODE BEGIN Header */
/**
  * @file    led_dimmer.h
  * @brief   LED PWM 调光驱动头文件
  * @note    适用于 PT4115 DIM 引脚控制
  */
/* USER CODE END Header */

#ifndef __LED_DIMMER_H__
#define __LED_DIMMER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* 函数原型 ------------------------------------------------------------------*/
void LED_Init(void);
uint8_t LED_SetBrightness(uint16_t duty);
void LED_TurnOff(void);
void LED_TurnOn(void);

/* 按键控制功能 */
void LED_KeyScan(void);           // 按键扫描（主循环调用）
void LED_KeyMode_Handler(void);   // Mode 按键处理（短按）
void LED_KeyUp_Handler(void);     // Up 按键处理
void LED_KeyDown_Handler(void);   // Down 按键处理

/* 频闪功能 */
void LED_ToggleStrobeMode(void);  // 切换频闪模式（长按）
void LED_StrobeProcess(void);     // 频闪处理

/* 外部信号功能 */
void LED_EXTI_IR_Pulse(void);     // 外部信号红外光补光
void LED_EXTI_Vis_Pulse(void);    // 外部信号可见光补光

#ifdef __cplusplus
}
#endif

#endif /* __LED_DIMMER_H__ */
