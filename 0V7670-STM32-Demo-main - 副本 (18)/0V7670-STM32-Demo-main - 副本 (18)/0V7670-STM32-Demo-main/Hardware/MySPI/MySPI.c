#include "stm32f10x.h"                  // Device header

/**
  * 函    数：SPI初始化（标准库版本）
  * 参    数：无
  * 返 回 值：无
  */
void MySPI_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;
	SPI_InitTypeDef SPI_InitStruct;

	/* 1. 使能时钟 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOA, ENABLE);

	/* 2. 配置GPIO */
	// PA5 - SPI1_SCK (推挽复用输出)
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PA7 - SPI1_MOSI (推挽复用输出)
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PA6 - SPI1_MISO (浮空输入)
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStruct);

	// PA4 - SPI1_NSS (软件控制，推挽输出)
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
	GPIO_SetBits(GPIOA, GPIO_Pin_4);  // 默认高电平

	/* 3. 配置SPI参数 */
	SPI_InitStruct.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
	SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;      // 时钟极性：低电平
	SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;    // 时钟相位：第一个边沿
	SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;       // 软件NSS控制
	SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;  // 18MHz
	SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitStruct.SPI_CRCPolynomial = 7;

	SPI_Init(SPI1, &SPI_InitStruct);

	/* 4. 使能SPI */
	SPI_Cmd(SPI1, ENABLE);
}

/**
  * 函    数：SPI1速度设置
  * 参    数：prescaler: SPI_BaudRatePrescaler_2/4/8/16/32/64/128/256
  * 返 回 值：无
  */
void MySPI_SetSpeed(uint8_t prescaler)
{
	SPI1->CR1 &= 0xFFC7;  // 清除BR[2:0]位
	SPI1->CR1 |= prescaler;  // 设置新的分频值
}

/**
  * 函    数：SPI读写一个字节
  * 参    数：TxData 要发送的数据
  * 返 回 值：接收到的数据
  */
uint8_t MySPI_ReadWriteByte(uint8_t TxData)
{
	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);  // 等待发送缓冲区空
	SPI_I2S_SendData(SPI1, TxData);  // 发送数据

	while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);  // 等待接收完成
	return SPI_I2S_ReceiveData(SPI1);  // 返回接收到的数据
}

/**
  * 函    数：SPI起始（兼容原有接口）
  * 参    数：无
  * 返 回 值：无
  */
void MySPI_Start(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);  // 拉低SS，开始时序
}

/**
  * 函    数：SPI终止（兼容原有接口）
  * 参    数：无
  * 返 回 值：无
  */
void MySPI_Stop(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_4);  // 拉高SS，终止时序
}

/**
  * 函    数：SPI交换传输一个字节（兼容原有接口）
  * 参    数：ByteSend 要发送的一个字节
  * 返 回 值：接收的一个字节
  */
uint8_t MySPI_SwapByte(uint8_t ByteSend)
{
	return MySPI_ReadWriteByte(ByteSend);
}
