#include "stm32f10x.h"                  // Device header
#include "delay.h"
#include "USART.h"
#include "sys.h"                         // For RCC_Configuration

// SD卡相关头文件
#include "MySPI.h"
#include "SDdriver.h"
#include "ff.h"
#include "diskio.h"

// OV7670相关头文件
#include "OV7670.h"
#include "exti.h"
#include "timer.h"
#include "Key.h"
#include "LED.h"

#include <stdio.h>
#include <string.h>

// 函数别名定义 - 用于SD卡测试
#define UART_Init           Serial_Init
#define UART_SendString     Serial_SendString
#define UART_SendNumber     Serial_SendNumber
#define UART_SendArray      Serial_SendArray
#define Delay_ms            delay_ms
#define Delay_us            delay_us

// 全局变量 - SD卡测试
uint8_t test_data = 0;
FATFS fs;           // File system object
FRESULT res;        // Function result
FIL fil;            // File object
UINT bw;            // Bytes written
UINT br;            // Bytes read

// 全局变量 - OV7670拍照
uint8_t g_image_line_buffer[640];  // 320像素 × 2字节 = 640字节
uint8_t KeyNum;		//定义用于接收按键键码的变量

// ==================== 阶段1&2新增：SD卡照片存储全局变量 ====================

// 照片文件名管理
char photo_filename[32];           // 当前照片文件名，格式：IMG_XXX.DAT
uint16_t photo_counter = 0;        // 照片计数器，用于生成唯一文件名

// SD卡读写缓冲区（复用g_image_line_buffer，无需额外内存）

// ==================== SD卡测试函数区域 ====================

/* Test SPI Initialization */
void Test_SPI1_Init(void)
{
	// Initialize SPI1
	MySPI_Init();

	// Check if SPI1 is configured correctly
	uint16_t cr1_value = SPI1->CR1;

	// Verify key bits
	if ((cr1_value & SPI_CR1_MSTR) &&          // Master mode
	    (cr1_value & SPI_CR1_SPE) &&           // SPI enabled
	    ((cr1_value & SPI_CR1_BR) >> 3) == 1)  // Baud rate = 4 prescaler
	{
		// SPI initialization successful
		UART_SendString("✓ SPI1 Init OK\r\n");

		// Use oscilloscope to measure PA5(SCK), send test byte
		SPI1->DR = 0xAA;
		while(SPI1->SR & SPI_SR_BSY);  // Wait for transfer complete

		UART_SendString("✓ SPI1 Sent 0xAA OK\r\n");
	}
	else
	{
		UART_SendString("✗ SPI1 Init Failed\r\n");
	}
}

/* SPI Full Test */
void SPI_Full_Test(void)
{
	UART_SendString("\r\n=== SPI Full Test ===\r\n");

	// 1. Test SPI ReadWrite
	UART_SendString("1. Testing SPI ReadWriteByte...\r\n");
	uint8_t tx_data = 0x55;
	uint8_t rx_data = MySPI_ReadWriteByte(tx_data);

	UART_SendString("   TX: 0x");
	UART_SendNumber(tx_data, 2);
	UART_SendString(" | RX: 0x");
	UART_SendNumber(rx_data, 2);
	UART_SendString(" (Note: RX may be 0x00 without slave device)\r\n");

	// 2. Test SPI speed setting
	UART_SendString("2. Testing SPI SetSpeed...\r\n");
	MySPI_SetSpeed(0x10);  // 9MHz
	UART_SendString("   Speed set to 8 prescaler\r\n");

	MySPI_SetSpeed(0x08);  // 18MHz
	UART_SendString("   Speed set to 4 prescaler\r\n");

	// 3. Test Start/Stop signal
	UART_SendString("3. Testing SPI Start/Stop...\r\n");
	MySPI_Start();
	UART_SendString("   SPI Start (SS=Low)\r\n");
	Delay_ms(1);
	MySPI_Stop();
	UART_SendString("   SPI Stop (SS=High)\r\n");

	// 4. Test GPIO pin status
	UART_SendString("4. GPIO Pin Status:\r\n");
	UART_SendString("   PA4(SS): 0x");
	UART_SendNumber(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_4), 1);
	UART_SendString("\r\n");

	UART_SendString("   PA5(SCK): 0x");
	UART_SendNumber(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_5), 1);
	UART_SendString("\r\n");

	UART_SendString("   PA6(MISO): 0x");
	UART_SendNumber(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6), 1);
	UART_SendString("\r\n");

	UART_SendString("   PA7(MOSI): 0x");
	UART_SendNumber(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_7), 1);
	UART_SendString("\r\n");

	UART_SendString("\r\n=== SPI Test Complete ===\r\n");
}

/* Test SD card driver SPI read/write functions */
void Test_SD_SPI_ReadWrite(void)
{
	UART_SendString("\r\n=== SD SPI ReadWrite Test ===\r\n");

	// Test 1: Send 0x55, receive data
	UART_SendString("1. Testing spi_readwrite(0x55)...\r\n");
	uint8_t tx_data = 0x55;
	uint8_t rx_data = spi_readwrite(tx_data);

	UART_SendString("   TX: 0x");
	UART_SendNumber(tx_data, 2);
	UART_SendString(" | RX: 0x");
	UART_SendNumber(rx_data, 2);
	UART_SendString("\r\n");

	// Test 2: Send 0xAA, receive data
	UART_SendString("2. Testing spi_readwrite(0xAA)...\r\n");
	tx_data = 0xAA;
	rx_data = spi_readwrite(tx_data);

	UART_SendString("   TX: 0x");
	UART_SendNumber(tx_data, 2);
	UART_SendString(" | RX: 0x");
	UART_SendNumber(rx_data, 2);
	UART_SendString("\r\n");

	// Test 3: Continuous readwrite 10 times
	UART_SendString("3. Testing continuous readwrite (10 times)...\r\n");
	for(int i = 0; i < 10; i++) {
		tx_data = i;
		rx_data = spi_readwrite(tx_data);
		UART_SendString("   i=");
		UART_SendNumber(i, 1);
		UART_SendString(" TX:0x");
		UART_SendNumber(tx_data, 2);
		UART_SendString(" RX:0x");
		UART_SendNumber(rx_data, 2);
		UART_SendString("\r\n");
	}

	UART_SendString("\r\n=== SD SPI ReadWrite Test Complete ===\r\n");
}

/* Test SD card driver SPI speed setting function */
void Test_SD_SPI_SetSpeed(void)
{
	UART_SendString("\r\n=== SD SPI SetSpeed Test ===\r\n");

	// Test setting different speeds
	UART_SendString("1. Setting speed to 256 prescaler...\r\n");
	SPI_setspeed(0x38);
	UART_SendString("   CR1 = 0x");
	UART_SendNumber(SPI1->CR1, 4);
	UART_SendString("\r\n");

	UART_SendString("2. Setting speed to 4 prescaler...\r\n");
	SPI_setspeed(0x08);
	UART_SendString("   CR1 = 0x");
	UART_SendNumber(SPI1->CR1, 4);
	UART_SendString("\r\n");

	UART_SendString("\r\n=== SD SPI SetSpeed Test Complete ===\r\n");
}

/* Test SD card CS control */
void Test_SD_CS(void)
{
	UART_SendString("\r\n=== SD CS Control Test ===\r\n");

	// Test CS high (release)
	UART_SendString("1. SD_CS(0) - CS = HIGH (Release)...\r\n");
	SD_CS(0);
	Delay_ms(1);
	UART_SendString("   PA4 state: 0x");
	UART_SendNumber(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_4), 1);
	UART_SendString("\r\n");

	// Test CS low (select)
	UART_SendString("2. SD_CS(1) - CS = LOW (Select)...\r\n");
	SD_CS(1);
	Delay_ms(1);
	UART_SendString("   PA4 state: 0x");
	UART_SendNumber(GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_4), 1);
	UART_SendString("\r\n");

	// Restore CS high
	SD_CS(0);

	UART_SendString("\r\n=== SD CS Control Test Complete ===\r\n");
}

/* Test SD card initialization */
void Test_SD_Init(void)
{
	UART_SendString("\r\n=== SD Card Initialization Test ===\r\n");

	// Initialize SPI
	MySPI_Init();
	UART_SendString("SPI1 initialized\r\n");

	// Initialize SD card
	UART_SendString("Initializing SD card...\r\n");
	uint8_t sd_result = SD_init();

	if(sd_result == 0)
	{
		UART_SendString("✓ SD Card Initialized Successfully!\r\n");
		UART_SendString("  SD Type: ");
		switch(SD_TYPE)
		{
			case MMC: UART_SendString("MMC\r\n"); break;
			case V1:  UART_SendString("SD V1.x\r\n"); break;
			case V2:  UART_SendString("SD V2.0\r\n"); break;
			case V2HC:UART_SendString("SDHC\r\n"); break;
			default:  UART_SendString("Unknown\r\n"); break;
		}

		// Get SD card capacity
		uint32_t sector_count = SD_GetSectorCount();
		UART_SendString("  Sector Count: ");
		UART_SendNumber(sector_count, 10);
		UART_SendString("\r\n");
		UART_SendString("  Capacity: ");
		UART_SendNumber((sector_count * 512) / 1024 / 1024, 10);
		UART_SendString(" MB\r\n");
	}
	else
	{
		UART_SendString("✗ SD Card Initialization Failed!\r\n");
	}

	UART_SendString("\r\n=== SD Init Test Complete ===\r\n");
}

/* Test FATFS file system */
void Test_FATFS(void)
{
	UART_SendString("\r\n=== FATFS Test ===\r\n");

	// Mount filesystem
	UART_SendString("1. Mounting filesystem...\r\n");
	res = f_mount(&fs, "", 1);
	if(res == FR_OK)
	{
		UART_SendString("   ✓ Filesystem mounted\r\n");
	}
	else
	{
		UART_SendString("   ✗ Mount failed, error: ");
		UART_SendNumber(res, 10);
		UART_SendString("\r\n");
		return;
	}

	// Create test file
	UART_SendString("2. Creating test file (test.txt)...\r\n");
	res = f_open(&fil, "test.txt", FA_CREATE_ALWAYS | FA_WRITE);
	if(res == FR_OK)
	{
		UART_SendString("   ✓ File created\r\n");
		const char* write_data = "Hello from STM32 Standard Library!\r\n";
		res = f_write(&fil, write_data, strlen(write_data), &bw);
		if(res == FR_OK)
		{
			UART_SendString("   ✓ Data written: ");
			UART_SendNumber(bw, 10);
			UART_SendString(" bytes\r\n");
		}
		f_close(&fil);
	}
	else
	{
		UART_SendString("   ✗ Create failed, error: ");
		UART_SendNumber(res, 10);
		UART_SendString("\r\n");
	}

	// Read test file
	UART_SendString("3. Reading test file...\r\n");
	res = f_open(&fil, "test.txt", FA_READ);
	if(res == FR_OK)
	{
		char read_buf[64];
		res = f_read(&fil, read_buf, sizeof(read_buf)-1, &br);
		if(res == FR_OK)
		{
			read_buf[br] = '\0';
			UART_SendString("   ✓ Read ");
			UART_SendNumber(br, 10);
			UART_SendString(" bytes: ");
			UART_SendString(read_buf);
		}
		f_close(&fil);
	}
	else
	{
		UART_SendString("   ✗ Read failed, error: ");
		UART_SendNumber(res, 10);
		UART_SendString("\r\n");
	}

	// Test file append
	UART_SendString("4. Appending to file...\r\n");
	res = f_open(&fil, "test.txt", FA_OPEN_ALWAYS | FA_WRITE);
	if(res == FR_OK)
	{
		const char* append_data = "This is appended data.\r\n";
		res = f_write(&fil, append_data, strlen(append_data), &bw);
		if(res == FR_OK)
		{
			UART_SendString("   ✓ Appended ");
			UART_SendNumber(bw, 10);
			UART_SendString(" bytes\r\n");
		}
		f_close(&fil);
	}

	// Get file info
	UART_SendString("5. File info:\r\n");
	FILINFO fno;
	res = f_stat("test.txt", &fno);
	if(res == FR_OK)
	{
		UART_SendString("   Size: ");
		UART_SendNumber(fno.fsize, 10);
		UART_SendString(" bytes\r\n");
	}

	// List root directory
	UART_SendString("6. Root directory listing:\r\n");
	DIR dir;
	res = f_opendir(&dir, "");
	if(res == FR_OK)
	{
		while(1)
		{
			res = f_readdir(&dir, &fno);
			if(res != FR_OK || fno.fname[0] == 0) break;
			if(fno.fattrib & AM_DIR)
			{
				UART_SendString("   [DIR]  ");
			}
			else
			{
				UART_SendString("   [FILE] ");
			}
			UART_SendString(fno.fname);
			UART_SendString(" (");
			UART_SendNumber(fno.fsize, 10);
			UART_SendString(" bytes)\r\n");
		}
		f_closedir(&dir);
	}

	UART_SendString("\r\n=== FATFS Test Complete ===\r\n");
}

// ==================== OV7670拍照函数区域 ====================

// 软件CRC32计算 - 兼容Python zlib.crc32
uint32_t crc32_software(uint8_t *data, uint32_t length)
{
	uint32_t crc = 0xFFFFFFFF;
	uint32_t i, j;

	for(i = 0; i < length; i++)
	{
		crc ^= data[i];
		for(j = 0; j < 8; j++)
		{
			if(crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
	}

	return ~crc;
}

// 发送图像协议头
// photo_type: 1=不补光, 2=可见光, 3=红外光
void Send_Image_Header(uint8_t photo_type)
{
	// 发送同步头和图像信息
	// 协议格式: IMG_START,width,height,bpp,type,crc\r\n
	// width=320, height=240, bpp=16 (RGB565), type=照片类型, crc=1 (CRC32使能)

	char header[50];
	sprintf(header, "IMG_START,320,240,16,%d,1\r\n", photo_type);
	Serial_SendString(header);

	// 短暂延迟确保接收端准备好
	delay_ms(5);
}

// 发送图像到PC - 增强版（带CRC校验）
// photo_type: 1=不补光, 2=可见光, 3=红外光
void Camera_SendToPC(uint8_t photo_type)
{
	uint32_t i, j;
	uint16_t color;
	uint16_t colorL, colorH;
	uint32_t crc_value = 0;

	if(OV7670_STA == 2)
	{
		// 发送图像数据头（带照片类型标识）
		Send_Image_Header(photo_type);

		// 复位FIFO读指针
		FIFO_RRST = 0;
		delay_us(1);
		FIFO_RCLK = 0;
		delay_us(1);
		FIFO_RCLK = 1;
		delay_us(1);
		FIFO_RCLK = 0;
		delay_us(1);
		FIFO_RRST = 1;
		delay_us(1);
		FIFO_RCLK = 1;
		delay_us(1);

		// 初始化CRC
		crc_value = 0xFFFFFFFF;

		// 逐行读取并发送，同时计算CRC
		for(i = 0; i < 240; i++)  // LCD_HIGH = 240
		{
			delay_us(2);

			for(j = 0; j < 320; j++)  // LCD_WIDTH = 320
			{
				FIFO_RCLK = 0;
				delay_us(1);
				colorH = OV7670_RedData();

				FIFO_RCLK = 1;
				delay_us(1);
				FIFO_RCLK = 0;
				delay_us(1);
				colorL = OV7670_RedData();

				colorL >>= 8;
				color = colorH | colorL;
				FIFO_RCLK = 1;

				g_image_line_buffer[j * 2] = (color >> 8) & 0xFF;
				g_image_line_buffer[j * 2 + 1] = color & 0xFF;
			}

			// 发送一行数据
			Serial_SendArray(g_image_line_buffer, 640);

			// 计算CRC（对这一行数据）
			for(j = 0; j < 640; j++)
			{
				crc_value ^= g_image_line_buffer[j];
				for(uint8_t k = 0; k < 8; k++)
				{
					if(crc_value & 1)
						crc_value = (crc_value >> 1) ^ 0xEDB88320;
					else
						crc_value >>= 1;
				}
			}
		}

		// 完成CRC计算
		crc_value = ~crc_value;

		// 发送CRC32值（4字节）
		uint8_t crc_bytes[4];
		crc_bytes[0] = (crc_value >> 24) & 0xFF;
		crc_bytes[1] = (crc_value >> 16) & 0xFF;
		crc_bytes[2] = (crc_value >> 8) & 0xFF;
		crc_bytes[3] = crc_value & 0xFF;
		Serial_SendArray(crc_bytes, 4);

		Serial_SendString("\r\nIMAGE_END\r\n");

		OV7670_STA = 0;
		delay_ms(50);
	}
}

// 拍照函数 - 根据补光模式拍照并发送到PC
// light_mode: 1=不补光, 2=可见光补光, 3=红外光补光
void Capture_Photo(uint8_t light_mode)
{
	uint8_t light_on = 0;

	// 根据模式设置补光
	switch(light_mode)
	{
		case 1:	// 不补光
			GPIO_SetBits(GPIOA, GPIO_Pin_15);  // PA15=高，关闭补光
			GPIO_SetBits(GPIOB, GPIO_Pin_3);   // PB3=高，关闭可见光
			GPIO_SetBits(GPIOB, GPIO_Pin_4);   // PB4=高，关闭红外
			Serial_SendString("Mode: No Light\r\n");
			break;

		case 2:	// 可见光补光
			GPIO_SetBits(GPIOA, GPIO_Pin_15);  // PA15=高，关闭补光
			GPIO_ResetBits(GPIOB, GPIO_Pin_3); // PB3=低，开启可见光
			GPIO_SetBits(GPIOB, GPIO_Pin_4);   // PB4=高，关闭红外
			Serial_SendString("Mode: Visible Light\r\n");
			light_on = 1;
			break;

		case 3:	// 红外光补光
			GPIO_SetBits(GPIOA, GPIO_Pin_15);  // PA15=高，关闭补光
			GPIO_SetBits(GPIOB, GPIO_Pin_3);   // PB3=高，关闭可见光
			GPIO_ResetBits(GPIOB, GPIO_Pin_4); // PB4=低，开启红外
			Serial_SendString("Mode: Infrared Light\r\n");
			light_on = 1;
			break;
	}

	// 等待补光稳定（如果需要补光）
	if(light_on)
	{
		delay_ms(200);
	}

	// 通过串口显示拍照状态
	Serial_SendString("Capturing...\r\n");

	// 舍弃当前FIFO中的图像，等待下一张
	if(OV7670_STA == 2)
	{
		// 当前有图像，清零等待下一张
		OV7670_STA = 0;
	}

	// 等待新的下一帧图像
	while(OV7670_STA != 2)
	{
		delay_ms(10);
	}

	// 发送到PC（传入照片类型）
	Camera_SendToPC(light_mode);

	// 关闭所有补光
//	GPIO_SetBits(GPIOA, GPIO_Pin_15);  // PA15=高
//	GPIO_SetBits(GPIOB, GPIO_Pin_3);   // PB3=高
//	GPIO_SetBits(GPIOB, GPIO_Pin_4);   // PB4=高

	// 通过串口显示完成状态
	Serial_SendString("Capture Complete!\r\n");

	// 短暂显示结果
	delay_ms(1000);
}

// ==================== 阶段1&2新增：SD卡照片存储函数 ====================

/*
 * 生成照片文件名
 * 格式：IMG_XXX.DAT (XXX = 001, 002, 003...)
 * 输入：photo_type (1=不补光, 2=可见光, 3=红外光)
 * 输出：filename (生成的文件名字符串)
 */
void Generate_PhotoFilename(char* filename, uint8_t photo_type)
{
	photo_counter++;  // 计数器递增，确保文件名唯一
	sprintf(filename, "IMG_%03d.DAT", photo_type * 100 + photo_counter);
}

/*
 * 创建照片文件并写入协议头
 * 输入：photo_type (1=不补光, 2=可见光, 3=红外光)
 * 返回：FR_OK 表示成功，其他表示失败
 */
FRESULT Create_PhotoFile(uint8_t photo_type)
{
	FRESULT res;
	char header[50];

	// 生成唯一文件名
	Generate_PhotoFilename(photo_filename, photo_type);

	// 创建/覆盖文件
	res = f_open(&fil, photo_filename, FA_CREATE_ALWAYS | FA_WRITE);
	if(res != FR_OK)
	{
		UART_SendString("✗ File create failed: ");
		UART_SendString(photo_filename);
		UART_SendString(" (error: ");
		UART_SendNumber(res, 10);
		UART_SendString(")\r\n");
		return res;
	}

	// 写入协议头：IMG_START,320,240,16,type,1\r\n
	// 长度：26字节 (24字符 + \r\n)
	sprintf(header, "IMG_START,320,240,16,%d,1\r\n", photo_type);
	res = f_write(&fil, header, strlen(header), &bw);
	if(res != FR_OK)
	{
		f_close(&fil);
		return res;
	}

	UART_SendString("✓ SD File Created: ");
	UART_SendString(photo_filename);
	UART_SendString(" (Header written)\r\n");

	return FR_OK;
}

/*
 * 写入一行图像数据到SD卡
 * 输入：line_data (图像数据指针), length (数据长度)
 * 返回：FR_OK 表示成功
 */
FRESULT Write_ImageLineToSD(uint8_t* line_data, uint16_t length)
{
	return f_write(&fil, line_data, length, &bw);
}

/*
 * 写入CRC32和帧尾到SD卡，并关闭文件
 * 输入：crc_value (CRC32校验值)
 * 返回：FR_OK 表示成功
 */
FRESULT Write_ImageFooterToSD(uint32_t crc_value)
{
	uint8_t crc_bytes[4];
	const char* frame_end = "\r\nIMAGE_END\r\n";

	// 写入CRC32（4字节）
	crc_bytes[0] = (crc_value >> 24) & 0xFF;
	crc_bytes[1] = (crc_value >> 16) & 0xFF;
	crc_bytes[2] = (crc_value >> 8) & 0xFF;
	crc_bytes[3] = crc_value & 0xFF;
	f_write(&fil, crc_bytes, 4, &bw);

	// 写入帧尾
	f_write(&fil, frame_end, strlen(frame_end), &bw);

	// 关闭文件
	f_close(&fil);

	UART_SendString("✓ CRC and Frame End written, file closed\r\n");

	return FR_OK;
}

/*
 * 从SD卡读取文件并发送到PC
 * 输入：filename (要读取的文件名)
 * 返回：FR_OK 表示成功
 */
FRESULT ReadAndSendToPC(char* filename)
{
	FRESULT res;
	uint8_t read_buf[640];
	UINT br;

	// 打开文件
	res = f_open(&fil, filename, FA_READ);
	if(res != FR_OK)
	{
		UART_SendString("✗ File open failed: ");
		UART_SendString(filename);
		UART_SendString(" (error: ");
		UART_SendNumber(res, 10);
		UART_SendString(")\r\n");
		return res;
	}

	UART_SendString("✓ Reading from SD: ");
	UART_SendString(filename);
	UART_SendString("\r\n");

	// 循环读取并发送（每次最多640字节）
	while(1)
	{
		res = f_read(&fil, read_buf, sizeof(read_buf), &br);
		if(res != FR_OK)
		{
			UART_SendString("✗ Read error\r\n");
			f_close(&fil);
			return res;
		}

		if(br == 0) break;  // 文件结束

		// 发送到PC
		Serial_SendArray(read_buf, br);
	}

	f_close(&fil);
	UART_SendString("✓ File read complete, sent to PC\r\n");

	return FR_OK;
}

/*
 * 拍照并保存到SD卡（核心函数）
 * 输入：photo_type (1=不补光, 2=可见光, 3=红外光)
 * 功能：读取摄像头数据，计算CRC，保存到SD卡
 */
void Camera_SaveToSD(uint8_t photo_type)
{
	uint32_t i, j;
	uint16_t color;
	uint16_t colorL, colorH;
	uint32_t crc_value = 0;
	FRESULT res;

	if(OV7670_STA == 2)
	{
		// 第1步：创建文件并写入协议头
		UART_SendString("\r\n[SD] Creating file...\r\n");
		res = Create_PhotoFile(photo_type);
		if(res != FR_OK)
		{
			UART_SendString("✗ File creation failed!\r\n");
			return;
		}

		// 第2步：复位FIFO读指针（同原Camera_SendToPC）
		FIFO_RRST = 0;
		delay_us(1);
		FIFO_RCLK = 0;
		delay_us(1);
		FIFO_RCLK = 1;
		delay_us(1);
		FIFO_RCLK = 0;
		delay_us(1);
		FIFO_RRST = 1;
		delay_us(1);
		FIFO_RCLK = 1;
		delay_us(1);

		// 初始化CRC
		crc_value = 0xFFFFFFFF;

		UART_SendString("[SD] Capturing to SD...\r\n");

		// 第3步：逐行读取并写入SD卡，同时计算CRC
		for(i = 0; i < 240; i++)  // 240行
		{
			delay_us(2);

			// 读取一行320像素（640字节）
			for(j = 0; j < 320; j++)
			{
				FIFO_RCLK = 0;
				delay_us(1);
				colorH = OV7670_RedData();

				FIFO_RCLK = 1;
				delay_us(1);
				FIFO_RCLK = 0;
				delay_us(1);
				colorL = OV7670_RedData();

				colorL >>= 8;
				color = colorH | colorL;
				FIFO_RCLK = 1;

				g_image_line_buffer[j * 2] = (color >> 8) & 0xFF;
				g_image_line_buffer[j * 2 + 1] = color & 0xFF;
			}

			// 写入一行数据到SD卡
			res = Write_ImageLineToSD(g_image_line_buffer, 640);
			if(res != FR_OK)
			{
				UART_SendString("✗ Write error at line ");
				UART_SendNumber(i, 10);
				UART_SendString("\r\n");
				f_close(&fil);
				return;
			}

			// 计算CRC（对这一行数据）
			for(j = 0; j < 640; j++)
			{
				crc_value ^= g_image_line_buffer[j];
				for(uint8_t k = 0; k < 8; k++)
				{
					if(crc_value & 1)
						crc_value = (crc_value >> 1) ^ 0xEDB88320;
					else
						crc_value >>= 1;
				}
			}

			// 每50行显示进度
			if(i % 50 == 0 && i > 0)
			{
				UART_SendString("  Progress: ");
				UART_SendNumber(i, 10);
				UART_SendString("/240 lines (");
				UART_SendNumber((i * 640) / 1536, 10);
				UART_SendString("%)\r\n");
			}
		}

		// 第4步：完成CRC计算
		crc_value = ~crc_value;

		// 第5步：写入CRC和帧尾，并关闭文件
		res = Write_ImageFooterToSD(crc_value);
		if(res != FR_OK)
		{
			UART_SendString("✗ Footer write failed!\r\n");
			return;
		}

		OV7670_STA = 0;
		delay_ms(50);

		UART_SendString("\r\n[SD] ✓ Save Complete! File: ");
		UART_SendString(photo_filename);
		UART_SendString("\r\n[SD] Total bytes: ");
		UART_SendNumber(153600 + 4 + 14, 10);  // 图像+CRC+帧尾
		UART_SendString("\r\n");
	}
}

/*
 * 测试函数：验证SD卡照片存储功能
 * 功能：创建测试文件，写入测试数据，读取并发送到PC
 */
void Test_SD_PhotoStorage(void)
{
	FRESULT res;

	UART_SendString("\r\n=== SD卡照片存储测试 (阶段1&2) ===\r\n");

	// 测试1：创建文件并写入协议头
	UART_SendString("测试1：创建照片文件\r\n");
	res = Create_PhotoFile(1);  // 使用类型1测试
	if(res != FR_OK)
	{
		UART_SendString("✗ 测试1失败\r\n");
		return;
	}

	// 测试2：写入模拟图像数据（10行）
	UART_SendString("测试2：写入模拟图像数据\r\n");
	uint8_t test_line[640];
	for(int i = 0; i < 640; i++) test_line[i] = (uint8_t)(i & 0xFF);

	for(int i = 0; i < 10; i++)
	{
		Write_ImageLineToSD(test_line, 640);
	}
	UART_SendString("  ✓ 写入10行测试数据\r\n");

	// 测试3：写入CRC和帧尾
	UART_SendString("测试3：写入CRC和帧尾\r\n");
	uint32_t test_crc = 0x12345678;  // 测试CRC值
	Write_ImageFooterToSD(test_crc);

	// 测试4：读取并发送到PC
	UART_SendString("测试4：读取文件并发送到PC\r\n");
	res = ReadAndSendToPC(photo_filename);
	if(res != FR_OK)
	{
		UART_SendString("✗ 测试4失败\r\n");
		return;
	}

	// 测试5：验证文件大小
	UART_SendString("测试5：验证文件信息\r\n");
	FILINFO fno;
	res = f_stat(photo_filename, &fno);
	if(res == FR_OK)
	{
		UART_SendString("  文件名: ");
		UART_SendString(photo_filename);
		UART_SendString("\r\n  文件大小: ");
		UART_SendNumber(fno.fsize, 10);
		UART_SendString(" bytes\r\n");

		if(fno.fsize == (10 * 640 + 4 + 14))
		{
			UART_SendString("  ✓ 文件大小正确！\r\n");
		}
		else
		{
			UART_SendString("  ✗ 文件大小异常\r\n");
		}
	}

	UART_SendString("\r\n=== SD卡照片存储测试完成 ===\r\n");
}

/*
 * 测试函数：验证完整拍照流程（摄像头→SD卡→PC）
 * 功能：模拟一次完整的拍照并存储流程
 */
void Test_FullCaptureFlow(void)
{
	FRESULT res;

	UART_SendString("\r\n=== 完整拍照流程测试 ===\r\n");

	// 检查摄像头状态
	if(OV7670_STA != 2)
	{
		UART_SendString("⚠ 等待摄像头数据...\r\n");
		while(OV7670_STA != 2)
		{
			delay_ms(10);
		}
	}

	// 步骤1：创建SD卡文件
	UART_SendString("步骤1：创建SD卡文件\r\n");
	res = Create_PhotoFile(1);  // 不补光模式
	if(res != FR_OK)
	{
		UART_SendString("✗ 创建文件失败\r\n");
		return;
	}

	// 步骤2：保存摄像头数据到SD卡
	UART_SendString("步骤2：保存摄像头数据到SD卡\r\n");
	Camera_SaveToSD(1);

	// 步骤3：从SD卡读取并发送到PC
	UART_SendString("步骤3：从SD卡读取并发送到PC\r\n");
	ReadAndSendToPC(photo_filename);

	// 步骤4：列出SD卡根目录
	UART_SendString("步骤4：SD卡根目录列表\r\n");
	DIR dir;
	FILINFO fno;
	res = f_opendir(&dir, "");
	if(res == FR_OK)
	{
		UART_SendString("  [SD卡根目录]\r\n");
		while(1)
		{
			res = f_readdir(&dir, &fno);
			if(res != FR_OK || fno.fname[0] == 0) break;

			if(!(fno.fattrib & AM_DIR))  // 只显示文件
			{
				UART_SendString("  [FILE] ");
				UART_SendString(fno.fname);
				UART_SendString(" (");
				UART_SendNumber(fno.fsize, 10);
				UART_SendString(" bytes)\r\n");
			}
		}
		f_closedir(&dir);
	}

	UART_SendString("\r\n=== 完整流程测试完成 ===\r\n");
}

// ==================== 主函数 ====================

int main(void)
{
	/* 模块初始化 */
	RCC_Configuration();			// 时钟设置
	Serial_Init();					// 串口初始化（921600波特率）

	/* 打印启动信息 */
	Serial_SendString("\r\n=== STM32F103 Combined Project ===\r\n");
	Serial_SendString("Phase 1: SD Card Test\r\n");
	Serial_SendString("Phase 2: OV7670 Camera\r\n");
	Serial_SendString("Baud Rate: 921600\r\n\r\n");

	// ==================== 第一阶段：SD卡测试 ====================

	Serial_SendString("\r\n========== PHASE 1: SD CARD TEST ==========\r\n");

	// 执行SPI测试
	Test_SPI1_Init();
	SPI_Full_Test();

	// 执行SD卡驱动SPI函数测试
	Test_SD_SPI_ReadWrite();
	Test_SD_SPI_SetSpeed();
	Test_SD_CS();

	// SD卡初始化测试
	Test_SD_Init();

	// FATFS文件系统测试
	Test_FATFS();

	// ==================== 阶段1&2新增：SD卡照片存储功能测试 ====================

	Serial_SendString("\r\n========== 阶段1&2新增：SD卡照片存储测试 ==========\r\n");

	// 测试SD卡照片存储基本功能
	Test_SD_PhotoStorage();

	// 等待用户查看测试结果
	delay_ms(2000);

	// ==================== 第二阶段：OV7670拍照 ====================

	Serial_SendString("\r\n========== PHASE 2: OV7670 CAMERA ==========\r\n");

	// OV7670相关初始化
	OV7670_Init();										// 摄像头初始化
	mEXTI_Init();										// 外部中断初始化
	TIMER_Init();										// 定时器初始化
	LED_Init();
	Key_Init();

	Serial_SendString("\r\n=== OV7670 Camera System ===\r\n");
	Serial_SendString("Multi-Type Capture Mode\r\n");
	Serial_SendString("Resolution: 320x240 RGB565\r\n");
	Serial_SendString("Key1: No Light | Key2: Visible | Key3: Infrared\r\n");
	Serial_SendString("Protocol: IMG_START,width,height,bpp,type,crc\r\n\r\n");

	// 等待摄像头稳定
	delay_ms(1000);

	// ==================== 主循环：拍照功能 ====================

	while(1)
	{
		KeyNum = Key_GetNum();		//获取按键键码（阻塞式，等待按键）

		// 根据按键值执行不同的拍照模式
		if(KeyNum == 1)
		{
			// 按键1：不补光拍照,PA8,PA15
			// 保存到SD卡
			Camera_SaveToSD(1);
			// 发送到PC
			Capture_Photo(1);
		}
		else if(KeyNum == 2)
		{
			// 按键2：可见光补光拍照,PC14,PB3
			// 保存到SD卡
			Camera_SaveToSD(2);
			// 发送到PC
			Capture_Photo(2);
		}
		else if(KeyNum == 3)
		{
			// 按键3：红外光补光拍照,PC15,PB4
			// 保存到SD卡
			Camera_SaveToSD(3);
			// 发送到PC
			Capture_Photo(3);
		}
		// KeyNum == 0 时不做任何操作，继续等待按键
	}
}
