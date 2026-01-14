#include "SDdriver.h"
#include "ff.h"
#include "stm32f10x_spi.h"
#include "Delay.h"

uint8_t DFF=0xFF;
uint8_t test;
uint8_t SD_TYPE=0x00;

MSD_CARDINFO SD0_CardInfo;

//////////////////////////////////////////////////////////////
// Chip Select
//////////////////////////////////////////////////////////////
void SD_CS(uint8_t p){
	if(p==0){
		GPIO_SetBits(SD_CS_GPIO_Port, SD_CS_Pin);  // CS = HIGH (Release)
		}else{
		GPIO_ResetBits(SD_CS_GPIO_Port, SD_CS_Pin); // CS = LOW (Select)
		}
}
///////////////////////////////////////////////////////////////
// Send command, release after sending
//////////////////////////////////////////////////////////////
int SD_sendcmd(uint8_t cmd,uint32_t arg,uint8_t crc){
	uint8_t r1;
  uint16_t retry;
  uint16_t timeout;

  SD_CS(0);
	delay_ms(20);
  SD_CS(1);

  // Wait for bus idle, with timeout
  timeout = 0;
	do{
		retry=spi_readwrite(DFF);
		timeout++;
		if(timeout > 1000) return 0xFF;  // Timeout return
	}while(retry!=0xFF);

  spi_readwrite(cmd | 0x40);
  spi_readwrite(arg >> 24);
  spi_readwrite(arg >> 16);
  spi_readwrite(arg >> 8);
  spi_readwrite(arg);
  spi_readwrite(crc);
  if(cmd==CMD12)spi_readwrite(DFF);

  // Wait for response with timeout
  timeout = 0;
  do
	{
		r1=spi_readwrite(0xFF);
		timeout++;
		if(timeout > 1000) return 0xFF;  // Timeout return
	}while(r1&0X80);

	return r1;
}


/////////////////////////////////////////////////////////////
// SD card initialization
////////////////////////////////////////////////////////////
uint8_t SD_init(void)
{
	uint8_t r1;
	uint8_t buff[6] = {0};
	uint16_t retry;
	uint8_t i;

//	MX_SPI3_Init();
	SPI_setspeed(0x00);

	// CS high, send at least 74 clocks(10 bytes = 80 clocks)
	SD_CS(0);  // CS = HIGH (Release)
	delay_ms(10);
	for(retry=0;retry<20;retry++){  // Increased to 20 bytes
			spi_readwrite(DFF);
	}

	// SD card enter IDLE state
	retry = 0;
	do{
		r1 = SD_sendcmd(CMD0 ,0, 0x95);
		retry++;
		if(retry > 200) {
			SD_CS(0);
			return 1;  // Init timeout
		}
	}while(r1!=0x01);

	// Check SD card type
	SD_TYPE=0;
	r1 = SD_sendcmd(CMD8, 0x1AA, 0x87);
	if(r1==0x01){
		for(i=0;i<4;i++)buff[i]=spi_readwrite(DFF);	//Get trailing return value of R7 resp
		if(buff[2]==0X01&&buff[3]==0XAA)//Card supports 2.7~3.6V
		{
			retry=0XFFFE;
			do
			{
				SD_sendcmd(CMD55,0,0X01);	//Send CMD55
				r1=SD_sendcmd(CMD41,0x40000000,0X01);//Send CMD41
			}while(r1&&retry--);
			if(retry&&SD_sendcmd(CMD58,0,0X01)==0)//Identify SD2.0 card version
			{
				for(i=0;i<4;i++)buff[i]=spi_readwrite(0XFF);//Get OCR value
				if(buff[0]&0x40){
					SD_TYPE=V2HC;
				}else {
					SD_TYPE=V2;
				}
			}
		}else{
			SD_sendcmd(CMD55,0,0X01);			//Send CMD55
			r1=SD_sendcmd(CMD41,0,0X01);	//Send CMD41
			if(r1<=1)
			{
				SD_TYPE=V1;
				retry=0XFFFE;
				do //Wait for exit IDLE mode
				{
					SD_sendcmd(CMD55,0,0X01);	//Send CMD55
					r1=SD_sendcmd(CMD41,0,0X01);//Send CMD41
				}while(r1&&retry--);
			}else//MMC card does not support CMD55+CMD41
			{
				SD_TYPE=MMC;//MMC V3
				retry=0XFFFE;
				do //Wait for exit IDLE mode
				{
					r1=SD_sendcmd(CMD1,0,0X01);//Send CMD1
				}while(r1&&retry--);
			}
			if(retry==0||SD_sendcmd(CMD16,512,0X01)!=0)SD_TYPE=ERR;//Bad card
		}
	}
	SD_CS(0);
	SPI_setspeed(0x08);
	if(SD_TYPE)return 0;
	else return 1;
}


//Read specified length data
uint8_t SD_ReceiveData(uint8_t *data, uint16_t len)
{
   uint8_t r1;
   uint16_t timeout = 0;
   SD_CS(1);
   do
   {
      r1 = spi_readwrite(0xFF);
      delay_ms(10);
      timeout++;
      if(timeout > 200) {
        SD_CS(0);
        return 1;  // Timeout return error
      }
	 }while(r1 != 0xFE);
  while(len--)
  {
   *data = spi_readwrite(0xFF);
   data++;
  }
  spi_readwrite(0xFF);
  spi_readwrite(0xFF);
  return 0;
}
//Write a data packet to SD card (512 bytes)
uint8_t SD_SendBlock(uint8_t*buf,uint8_t cmd)
{
	uint16_t t;
	uint8_t r1;
	uint16_t timeout = 0;
	do{
		r1=spi_readwrite(0xFF);
		timeout++;
		if(timeout > 1000) return 1;  // Timeout
	}while(r1!=0xFF);

	spi_readwrite(cmd);
	if(cmd!=0XFD)//Not end command
	{
		for(t=0;t<512;t++)spi_readwrite(buf[t]);//Increase speed, reduce function parameter time
	    spi_readwrite(0xFF);//Ignore crc
	    spi_readwrite(0xFF);
		t=spi_readwrite(0xFF);//Receive response
		if((t&0x1F)!=0x05)return 2;//Response error
	}
    return 0;//Write successful
}

//Get CID information
uint8_t SD_GETCID (uint8_t *cid_data)
{
		uint8_t r1;
	  r1=SD_sendcmd(CMD10,0,0x01); //Read CID register
		if(r1==0x00){
			r1=SD_ReceiveData(cid_data,16);
		}
		SD_CS(0);
		if(r1)return 1;
		else return 0;
}
//Get CSD information
uint8_t SD_GETCSD(uint8_t *csd_data){
		uint8_t r1;
    r1=SD_sendcmd(CMD9,0,0x01);//Send CMD9 command, read CSD register
    if(r1==0)
	{
    r1=SD_ReceiveData(csd_data, 16);//Receive 16 bytes data
    }
	SD_CS(0);//Deselect
	if(r1)return 1;
	else return 0;
}
//Get total sectors of SD card
uint32_t SD_GetSectorCount(void)
{
    uint8_t csd[16];
    uint32_t Capacity;
    uint8_t n;
		uint16_t csize;
	//Get CSD information, return 0 if error
    if(SD_GETCSD(csd)!=0) return 0;
    //If SDHC card, calculate as below
    if((csd[0]&0xC0)==0x40)	 //V2.00 card
    {
		csize = csd[9] + ((uint16_t)csd[8] << 8) + 1;
		Capacity = (uint32_t)csize << 10;//Get sector count
    }else//V1.XX card
    {
		n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
		csize = (csd[8] >> 6) + ((uint16_t)csd[7] << 2) + ((uint16_t)(csd[6] & 3) << 10) + 1;
		Capacity= (uint32_t)csize << (n - 9);//Get sector count
    }
    return Capacity;
}
int MSD0_GetCardInfo(PMSD_CARDINFO SD0_CardInfo)
{
  uint8_t r1;
  uint8_t CSD_Tab[16];
  uint8_t CID_Tab[16];

  /* Send CMD9, Read CSD */
  r1 = SD_sendcmd(CMD9, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CSD_Tab, 16))
  {
	return 1;
  }

  /* Send CMD10, Read CID */
  r1 = SD_sendcmd(CMD10, 0, 0xFF);
  if(r1 != 0x00)
  {
    return r1;
  }

  if(SD_ReceiveData(CID_Tab, 16))
  {
	return 2;
  }

  /* Byte 0 */
  SD0_CardInfo->CSD.CSDStruct = (CSD_Tab[0] & 0xC0) >> 6;
  SD0_CardInfo->CSD.SysSpecVersion = (CSD_Tab[0] & 0x3C) >> 2;
  SD0_CardInfo->CSD.Reserved1 = CSD_Tab[0] & 0x03;
  /* Byte 1 */
  SD0_CardInfo->CSD.TAAC = CSD_Tab[1] ;
  /* Byte 2 */
  SD0_CardInfo->CSD.NSAC = CSD_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CSD.MaxBusClkFrec = CSD_Tab[3];
  /* Byte 4 */
  SD0_CardInfo->CSD.CardComdClasses = CSD_Tab[4] << 4;
  /* Byte 5 */
  SD0_CardInfo->CSD.CardComdClasses |= (CSD_Tab[5] & 0xF0) >> 4;
  SD0_CardInfo->CSD.RdBlockLen = CSD_Tab[5] & 0x0F;
  /* Byte 6 */
  SD0_CardInfo->CSD.PartBlockRead = (CSD_Tab[6] & 0x80) >> 7;
  SD0_CardInfo->CSD.WrBlockMisalign = (CSD_Tab[6] & 0x40) >> 6;
  SD0_CardInfo->CSD.RdBlockMisalign = (CSD_Tab[6] & 0x20) >> 5;
  SD0_CardInfo->CSD.DSRImpl = (CSD_Tab[6] & 0x10) >> 4;
  SD0_CardInfo->CSD.Reserved2 = 0; /* Reserved */
  SD0_CardInfo->CSD.DeviceSize = (CSD_Tab[6] & 0x03) << 10;
  /* Byte 7 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[7]) << 2;
  /* Byte 8 */
  SD0_CardInfo->CSD.DeviceSize |= (CSD_Tab[8] & 0xC0) >> 6;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMin = (CSD_Tab[8] & 0x38) >> 3;
  SD0_CardInfo->CSD.MaxRdCurrentVDDMax = (CSD_Tab[8] & 0x07);
  /* Byte 9 */
  SD0_CardInfo->CSD.MaxWrCurrentVDDMin = (CSD_Tab[9] & 0xE0) >> 5;
  SD0_CardInfo->CSD.MaxWrCurrentVDDMax = (CSD_Tab[9] & 0x1C) >> 2;
  SD0_CardInfo->CSD.DeviceSizeMul = (CSD_Tab[9] & 0x03) << 1;
  /* Byte 10 */
  SD0_CardInfo->CSD.DeviceSizeMul |= (CSD_Tab[10] & 0x80) >> 7;
  SD0_CardInfo->CSD.EraseGrSize = (CSD_Tab[10] & 0x7C) >> 2;
  SD0_CardInfo->CSD.EraseGrMul = (CSD_Tab[10] & 0x03) << 3;
  /* Byte 11 */
  SD0_CardInfo->CSD.EraseGrMul |= (CSD_Tab[11] & 0xE0) >> 5;
  SD0_CardInfo->CSD.WrProtectGrSize = (CSD_Tab[11] & 0x1F);
  /* Byte 12 */
  SD0_CardInfo->CSD.WrProtectGrEnable = (CSD_Tab[12] & 0x80) >> 7;
  SD0_CardInfo->CSD.ManDeflECC = (CSD_Tab[12] & 0x60) >> 5;
  SD0_CardInfo->CSD.WrSpeedFact = (CSD_Tab[12] & 0x1C) >> 2;
  SD0_CardInfo->CSD.MaxWrBlockLen = (CSD_Tab[12] & 0x03) << 2;
  /* Byte 13 */
  SD0_CardInfo->CSD.MaxWrBlockLen |= (CSD_Tab[13] & 0xc0) >> 6;
  SD0_CardInfo->CSD.WriteBlockPaPartial = (CSD_Tab[13] & 0x20) >> 5;
  SD0_CardInfo->CSD.Reserved3 = 0;
  SD0_CardInfo->CSD.ContentProtectAppli = (CSD_Tab[13] & 0x01);
  /* Byte 14 */
  SD0_CardInfo->CSD.FileFormatGrouop = (CSD_Tab[14] & 0x80) >> 7;
  SD0_CardInfo->CSD.CopyFlag = (CSD_Tab[14] & 0x40) >> 6;
  SD0_CardInfo->CSD.PermWrProtect = (CSD_Tab[14] & 0x20) >> 5;
  SD0_CardInfo->CSD.TempWrProtect = (CSD_Tab[14] & 0x10) >> 4;
  SD0_CardInfo->CSD.FileFormat = (CSD_Tab[14] & 0x0C) >> 2;
  SD0_CardInfo->CSD.ECC = (CSD_Tab[14] & 0x03);
  /* Byte 15 */
  SD0_CardInfo->CSD.CSD_CRC = (CSD_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CSD.Reserved4 = 1;

  if(SD0_CardInfo->CardType == V2HC)
  {
	 /* Byte 7 */
	 SD0_CardInfo->CSD.DeviceSize = (uint16_t)(CSD_Tab[8]) *256;
	 /* Byte 8 */
	 SD0_CardInfo->CSD.DeviceSize += CSD_Tab[9] ;
  }

  SD0_CardInfo->Capacity = SD0_CardInfo->CSD.DeviceSize * MSD_BLOCKSIZE * 1024;
  SD0_CardInfo->BlockSize = MSD_BLOCKSIZE;

  /* Byte 0 */
  SD0_CardInfo->CID.ManufacturerID = CID_Tab[0];
  /* Byte 1 */
  SD0_CardInfo->CID.OEM_AppliID = CID_Tab[1] << 8;
  /* Byte 2 */
  SD0_CardInfo->CID.OEM_AppliID |= CID_Tab[2];
  /* Byte 3 */
  SD0_CardInfo->CID.ProdName1 = CID_Tab[3] << 24;
  /* Byte 4 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[4] << 16;
  /* Byte 5 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[5] << 8;
  /* Byte 6 */
  SD0_CardInfo->CID.ProdName1 |= CID_Tab[6];
  /* Byte 7 */
  SD0_CardInfo->CID.ProdName2 = CID_Tab[7];
  /* Byte 8 */
  SD0_CardInfo->CID.ProdRev = CID_Tab[8];
  /* Byte 9 */
  SD0_CardInfo->CID.ProdSN = CID_Tab[9] << 24;
  /* Byte 10 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[10] << 16;
  /* Byte 11 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[11] << 8;
  /* Byte 12 */
  SD0_CardInfo->CID.ProdSN |= CID_Tab[12];
  /* Byte 13 */
  SD0_CardInfo->CID.Reserved1 |= (CID_Tab[13] & 0xF0) >> 4;
  /* Byte 14 */
  SD0_CardInfo->CID.ManufactDate = (CID_Tab[13] & 0x0F) << 8;
  /* Byte 15 */
  SD0_CardInfo->CID.ManufactDate |= CID_Tab[14];
  /* Byte 16 */
  SD0_CardInfo->CID.CID_CRC = (CID_Tab[15] & 0xFE) >> 1;
  SD0_CardInfo->CID.Reserved2 = 1;

  return 0;
}


//Write SD card
//buf:data buffer
//sector:start sector
//cnt:sector count
//return value:0,ok;other,failed.
uint8_t SD_WriteDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
	uint8_t r1;
	if(SD_TYPE!=V2HC)sector *= 512;//Convert to byte address
	if(cnt==1)
	{
		r1=SD_sendcmd(CMD24,sector,0X01);//Write command
		if(r1==0)//Command sent successfully
		{
			r1=SD_SendBlock(buf,0xFE);//Write 512 bytes
		}
	}else
	{
		if(SD_TYPE!=MMC)
		{
			SD_sendcmd(CMD55,0,0X01);
			SD_sendcmd(CMD23,cnt,0X01);//Send command
		}
 		r1=SD_sendcmd(CMD25,sector,0X01);//Continuous write command
		if(r1==0)
		{
			do
			{
				r1=SD_SendBlock(buf,0xFC);//Receive 512 bytes
				buf+=512;
			}while(--cnt && r1==0);
			r1=SD_SendBlock(0,0xFD);//Receive 512 bytes
		}
	}
	SD_CS(0);//Deselect
	return r1;//
}
//Read SD card
//buf:data buffer
//sector:sector
//cnt:sector count
//return value:0,ok;other,failed.
uint8_t SD_ReadDisk(uint8_t*buf,uint32_t sector,uint8_t cnt)
{
	uint8_t r1;
	if(SD_TYPE!=V2HC)sector <<= 9;//Convert to byte address
	if(cnt==1)
	{
		r1=SD_sendcmd(CMD17,sector,0X01);//Read command
		if(r1==0)//Command sent successfully
		{
			r1=SD_ReceiveData(buf,512);//Receive 512 bytes
		}
	}else
	{
		r1=SD_sendcmd(CMD18,sector,0X01);//Continuous read command
		do
		{
			r1=SD_ReceiveData(buf,512);//Receive 512 bytes
			buf+=512;
		}while(--cnt && r1==0);
		SD_sendcmd(CMD12,0,0X01);	//Send stop command
	}
	SD_CS(0);//Deselect
	return r1;//
}


/**
  * @brief  SPI read/write one byte (Standard Library version)
  * @param  Txdata: Data to send
  * @retval Received data
  */
uint8_t spi_readwrite(uint8_t Txdata)
{
	// Wait for transmit buffer empty
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);

	// Send data
	SPI_I2S_SendData(SPI1, Txdata);

	// Wait for receive buffer not empty
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

	// Return received data
	return SPI_I2S_ReceiveData(SPI1);
}
/**
  * @brief  SPI baud rate setting (Standard Library version)
  * @param  speed: SPI_BaudRatePrescaler_2/4/8/16/32/64/128/256
  * @retval None
  */
void SPI_setspeed(uint8_t speed)
{
	// Disable SPI
	SPI_Cmd(SPI1, DISABLE);

	// Modify baud rate prescaler
	SPI1->CR1 &= 0xFFC7;  // Clear BR[2:0] bits (bits 3-5)
	SPI1->CR1 |= speed;   // Set new divider value

	// Re-enable SPI
	SPI_Cmd(SPI1, ENABLE);
}


///////////////////////////END//////////////////////////////////////

/**
  * @brief  Get current time for FATFS file system
  * @retval Return timestamp (bit31:25 year(0-127), bit24:21 month(1-12), bit20:16 day(1-31),
  *                     bit15:11 hour(0-23), bit10:5 minute(0-59), bit4:0 second(0-29)*2)
  */
DWORD get_fattime(void)
{
    // Return fixed timestamp (2024-01-01 12:00:00)
    // Year: 2024-1980 = 44 (bit31:25)
    // Month: 1 (bit24:21)
    // Day: 1 (bit20:16)
    // Hour: 12 (bit15:11)
    // Minute: 0 (bit10:5)
    // Second: 0 (bit4:0)
    return ((DWORD)(2024 - 1980) << 25) |
           ((DWORD)1 << 21) |
           ((DWORD)1 << 16) |
           ((DWORD)12 << 11) |
           ((DWORD)0 << 5) |
           ((DWORD)0);
}

