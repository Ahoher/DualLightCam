#include "stm32f10x.h"
#include "integer.h"

extern uint8_t SD_TYPE;

#define SD_CS_GPIO_Port GPIOA
#define SD_CS_Pin GPIO_Pin_4

//SD card types
#define ERR     	0x00
#define MMC				0x01
#define V1				0x02
#define V2				0x04
#define V2HC			0x06

#define DUMMY_BYTE				 0xFF 
#define MSD_BLOCKSIZE			 512


//CMD definitions
#define CMD0    0       //Card reset
#define CMD1    1
#define CMD8    8       //CMD8, SEND_IF_COND
#define CMD9    9       //CMD9, read CSD data
#define CMD10   10      //CMD10, read CID data
#define CMD12   12      //CMD12, stop data transfer
#define CMD16   16      //CMD16, set SectorSize, should return 0x00
#define CMD17   17      //CMD17, read sector
#define CMD18   18      //CMD18, read Multi sector
#define CMD23   23      //CMD23, set N blocks to erase before multi-sector write
#define CMD24   24      //CMD24, write sector
#define CMD25   25      //CMD25, write Multi sector
#define CMD41   41      //CMD41, should return 0x00
#define CMD55   55      //CMD55, should return 0x01
#define CMD58   58      //CMD58, read OCR information
#define CMD59   59      //CMD59, enable/disable CRC, should return 0x00

//Data write response meanings
#define MSD_DATA_OK                0x05
#define MSD_DATA_CRC_ERROR         0x0B
#define MSD_DATA_WRITE_ERROR       0x0D
#define MSD_DATA_OTHER_ERROR       0xFF
//SD card response markers
#define MSD_RESPONSE_NO_ERROR      0x00
#define MSD_IN_IDLE_STATE          0x01
#define MSD_ERASE_RESET            0x02
#define MSD_ILLEGAL_COMMAND        0x04
#define MSD_COM_CRC_ERROR          0x08
#define MSD_ERASE_SEQUENCE_ERROR   0x10
#define MSD_ADDRESS_ERROR          0x20
#define MSD_PARAMETER_ERROR        0x40
#define MSD_RESPONSE_FAILURE       0xFF


enum _CD_HOLD
{
	HOLD = 0,
	RELEASE = 1,
};

typedef struct               /* Card Specific Data */
{
  uint8_t  CSDStruct;            /* CSD structure */
  uint8_t  SysSpecVersion;       /* System specification version */
  uint8_t  Reserved1;            /* Reserved */
  uint8_t  TAAC;                 /* Data read access-time 1 */
  uint8_t  NSAC;                 /* Data read access-time 2 in CLK cycles */
  uint8_t  MaxBusClkFrec;        /* Max. bus clock frequency */
  uint16_t CardComdClasses;      /* Card command classes */
  uint8_t  RdBlockLen;           /* Max. read data block length */
  uint8_t  PartBlockRead;        /* Partial blocks for read allowed */
  uint8_t  WrBlockMisalign;      /* Write block misalignment */
  uint8_t  RdBlockMisalign;      /* Read block misalignment */
  uint8_t  DSRImpl;              /* DSR implemented */
  uint8_t  Reserved2;            /* Reserved */
  uint32_t DeviceSize;           /* Device Size */
  uint8_t  MaxRdCurrentVDDMin;   /* Max. read current @ VDD min */
  uint8_t  MaxRdCurrentVDDMax;   /* Max. read current @ VDD max */
  uint8_t  MaxWrCurrentVDDMin;   /* Max. write current @ VDD min */
  uint8_t  MaxWrCurrentVDDMax;   /* Max. write current @ VDD max */
  uint8_t  DeviceSizeMul;        /* Device size multiplier */
  uint8_t  EraseGrSize;          /* Erase group size */
  uint8_t  EraseGrMul;           /* Erase group size multiplier */
  uint8_t  WrProtectGrSize;      /* Write protect group size */
  uint8_t  WrProtectGrEnable;    /* Write protect group enable */
  uint8_t  ManDeflECC;           /* Manufacturer default ECC */
  uint8_t  WrSpeedFact;          /* Write speed factor */
  uint8_t  MaxWrBlockLen;        /* Max. write data block length */
  uint8_t  WriteBlockPaPartial;  /* Partial blocks for write allowed */
  uint8_t  Reserved3;            /* Reserded */
  uint8_t  ContentProtectAppli;  /* Content protection application */
  uint8_t  FileFormatGrouop;     /* File format group */
  uint8_t  CopyFlag;             /* Copy flag (OTP) */
  uint8_t  PermWrProtect;        /* Permanent write protection */
  uint8_t  TempWrProtect;        /* Temporary write protection */
  uint8_t  FileFormat;           /* File Format */
  uint8_t  ECC;                  /* ECC code */
  uint8_t  CSD_CRC;              /* CSD CRC */
  uint8_t  Reserved4;            /* always 1*/
}
MSD_CSD;

typedef struct				 /*Card Identification Data*/
{
  uint8_t  ManufacturerID;       /* ManufacturerID */
  uint16_t OEM_AppliID;          /* OEM/Application ID */
  uint32_t ProdName1;            /* Product Name part1 */
  uint8_t  ProdName2;            /* Product Name part2*/
  uint8_t  ProdRev;              /* Product Revision */
  uint32_t ProdSN;               /* Product Serial Number */
  uint8_t  Reserved1;            /* Reserved1 */
  uint16_t ManufactDate;         /* Manufacturing Date */
  uint8_t  CID_CRC;              /* CID CRC */
  uint8_t  Reserved2;            /* always 1 */
}
MSD_CID;

typedef struct
{
  MSD_CSD CSD;
  MSD_CID CID;
  uint32_t Capacity;              /* Card Capacity */
  uint32_t BlockSize;             /* Card Block Size */
  uint16_t RCA;
  uint8_t CardType;
  uint32_t SpaceTotal;            /* Total space size in file system */
  uint32_t SpaceFree;      	     /* Free space size in file system */
}
MSD_CARDINFO, *PMSD_CARDINFO;

extern MSD_CARDINFO SD0_CardInfo;


uint8_t		 	SD_init(void);
void 				SD_CS(uint8_t p);
uint32_t  	SD_GetSectorCount(void);
uint8_t 		SD_GETCID (uint8_t *cid_data);
uint8_t 		SD_GETCSD(uint8_t *csd_data);
int 				MSD0_GetCardInfo(PMSD_CARDINFO SD0_CardInfo);
uint8_t			SD_ReceiveData(uint8_t *data, uint16_t len);
uint8_t 		SD_SendBlock(uint8_t*buf,uint8_t cmd);
uint8_t 		SD_ReadDisk(uint8_t*buf,uint32_t sector,uint8_t cnt);
uint8_t 		SD_WriteDisk(uint8_t*buf,uint32_t sector,uint8_t cnt);


void SPI_setspeed(uint8_t speed);
uint8_t spi_readwrite(uint8_t Txdata);

// FATFS required function
DWORD get_fattime(void);

