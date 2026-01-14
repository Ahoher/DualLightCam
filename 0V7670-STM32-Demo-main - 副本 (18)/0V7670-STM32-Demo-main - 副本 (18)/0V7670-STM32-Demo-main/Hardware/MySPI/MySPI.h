#ifndef __MYSPI_H
#define __MYSPI_H

#include "stm32f10x.h"

void MySPI_Init(void);
void MySPI_SetSpeed(uint8_t prescaler);
uint8_t MySPI_ReadWriteByte(uint8_t TxData);

// 兼容原有接口
void MySPI_Start(void);
void MySPI_Stop(void);
uint8_t MySPI_SwapByte(uint8_t ByteSend);

#endif
