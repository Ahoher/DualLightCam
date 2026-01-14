#include "stm32f10x.h"                  // Device header
#include "delay.h"
/**
  * 函    数：按键初始化
  * 参    数：无
  * 返 回 值：无
  */
void Key_Init(void)
{
    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOB, ENABLE);  // 开启GPIOA和GPIOC的时钟
    
    /* GPIO初始化 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    /* 初始化PC14和PC15 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14 | GPIO_Pin_15;
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    /* 初始化PA8 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    /* 初始化PB6、B7 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

/**
  * 函    数：按键获取键码
  * 参    数：无
  * 返 回 值：按下按键的键码值，范围：0~5，返回0代表没有按键按下
  * 注意事项：此函数是阻塞式操作，当按键按住不放时，函数会卡住，直到按键松手
  */
uint8_t Key_GetNum(void)
{
    uint8_t KeyNum = 0;  // 定义变量，默认键码值为0
    
    // 检测PA8按键 (键码1)
    if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == 0)  // 读PA8输入寄存器的状态
    {
        delay_ms(20);  // 延时消抖
        while (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_8) == 0);  // 等待按键松手
        delay_ms(20);  // 延时消抖
        KeyNum = 1;  // 置键码为1
    }
    
    // 检测PC14按键 (键码2)
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) == 0)  // 读PC14输入寄存器的状态
    {
        delay_ms(20);  // 延时消抖
        while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) == 0);  // 等待按键松手
        delay_ms(20);  // 延时消抖
        KeyNum = 2;  // 置键码为2
    }
    
    // 检测PC15按键 (键码3)
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15) == 0)  // 读PC15输入寄存器的状态
    {
        delay_ms(20);  // 延时消抖
        while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15) == 0);  // 等待按键松手
        delay_ms(20);  // 延时消抖
        KeyNum = 3;  // 置键码为3
    }

    // 检测PB6按键 (键码4)
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6) == 0)  // 读PB6输入寄存器的状态
    {
        delay_ms(20);  // 延时消抖
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_6) == 0);  // 等待按键松手
        delay_ms(20);  // 延时消抖
        KeyNum = 4;  // 置键码为4
    }

    // 检测PB7按键 (键码5)清空
    if (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == 0)  // 读PB7输入寄存器的状态
    {
        delay_ms(20);  // 延时消抖
        while (GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == 0);  // 等待按键松手
        delay_ms(20);  // 延时消抖
        KeyNum = 5;  // 置键码为5
    }


    return KeyNum;  // 返回键码值，如果没有按键按下，所有if都不成立，则键码为默认值0
}
