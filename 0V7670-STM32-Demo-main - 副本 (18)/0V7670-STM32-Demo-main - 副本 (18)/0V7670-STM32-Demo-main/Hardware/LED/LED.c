#include "stm32f10x.h"                  // Device header
/**
  * 函    数：LED初始化
  * 参    数：无
  * 返 回 值：无
  */
void LED_Init(void)
{
    /*开启时钟*/
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);  //开启GPIOA和GPIOB的时钟
    
    /* 注意：PA15、PB3、PB4与JTAG/SWD调试接口复用，需要先释放这些引脚 */
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);  //禁用JTAG，保留SWD调试功能
    
    /*GPIO初始化*/
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  //推挽输出模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    
    // 初始化PA15
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 初始化PB3和PB4
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    
    /*设置GPIO初始化后的默认电平*/
    GPIO_SetBits(GPIOA, GPIO_Pin_15);  //设置PA15引脚为高电平
    GPIO_SetBits(GPIOB, GPIO_Pin_3 | GPIO_Pin_4);  //设置PB3和PB4引脚为高电平
}


