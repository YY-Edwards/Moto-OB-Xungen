﻿/*
 * rc522.h
 *
 * Created: 2017/8/10 星期四 下午 16:22:32
 *  Author: Edwards
 */ 


#ifndef RC522_H_
#define RC522_H_
#include "spi.h"
#include "string.h"
#include "gpio.h"
#include "timer.h"
#include "data_flash.h"


#define FATAL_ERROR_DMA_TX_OVERFLOW     0x01
#define FATAL_ERROR_RC522_SPI_INIT 0x02
#define FATAL_ERROR_RC522_READ_ID  0x03
#define FATAL_ERROR_ACC_INIT            0x04

#define DF_SPI_FIRST_NPCS 0
#define DF_SPI_MASTER_SPEED 24000000
#define DF_SPI_BITS 8
#define DF_SPI_PCS_0 0

#define DF_TEST_SUCCESS 0
#define DF_TEST_FAIL 1

#define ENTER_POWERDOWN 1
#define WAKEUP_RC522	0
//
//typedef enum
//{
	//DF_ERROR = -1,
	//DF_OK = 0,
	//DF_INVALID_PARAM = 1,
	//DF_DEVICE_BUSY = 2,
	//DF_WRITE_DISABLED = 3,
	//DF_ERASE_FAIL = 4,
	//DF_ERASE_COMPLETED = 5,
	//DF_WRITE_FAIL = 6,
	//DF_WRITE_COMPLETED = 7
//} df_status_t;



/////////////////////////////////////////////////////////////////////
//MF522命令字
/////////////////////////////////////////////////////////////////////
#define PCD_IDLE              0x00               //取消当前命令
#define PCD_AUTHENT           0x0E               //验证密钥
#define PCD_RECEIVE           0x08               //接收数据
#define PCD_TRANSMIT          0x04               //发送数据
#define PCD_TRANSCEIVE        0x0C               //发送并接收数据
#define PCD_RESETPHASE        0x0F               //复位
#define PCD_CALCCRC           0x03               //CRC计算
#define PCD_POWERDOWN         0x10               //软掉电

/////////////////////////////////////////////////////////////////////
//Mifare_One卡片命令字
/////////////////////////////////////////////////////////////////////
#define PICC_REQIDL           0x26               //寻天线区内未进入休眠状态
#define PICC_REQALL           0x52               //寻天线区内全部卡
#define PICC_ANTICOLL1        0x93               //防冲撞
#define PICC_ANTICOLL2        0x95               //防冲撞
#define PICC_AUTHENT1A        0x60               //验证A密钥
#define PICC_AUTHENT1B        0x61               //验证B密钥
#define PICC_READ             0x30               //读块
#define PICC_WRITE            0xA0               //写块
#define PICC_DECREMENT        0xC0               //扣款
#define PICC_INCREMENT        0xC1               //充值
#define PICC_RESTORE          0xC2               //调块数据到缓冲区
#define PICC_TRANSFER         0xB0               //保存缓冲区中数据
#define PICC_HALT             0x50               //休眠

/////////////////////////////////////////////////////////////////////
//MF522 FIFO长度定义
/////////////////////////////////////////////////////////////////////
#define DEF_FIFO_LENGTH       64                 //FIFO size=64byte
#define MAXRLEN  18

/////////////////////////////////////////////////////////////////////
//MF522寄存器定义
/////////////////////////////////////////////////////////////////////
// PAGE 0
#define     RFU00                 0x00
#define     CommandReg            0x01
#define     ComIEnReg             0x02
#define     DivlEnReg             0x03
#define     ComIrqReg             0x04
#define     DivIrqReg             0x05
#define     ErrorReg              0x06
#define     Status1Reg            0x07
#define     Status2Reg            0x08
#define     FIFODataReg           0x09
#define     FIFOLevelReg          0x0A
#define     WaterLevelReg         0x0B
#define     ControlReg            0x0C
#define     BitFramingReg         0x0D
#define     CollReg               0x0E
#define     RFU0F                 0x0F
// PAGE 1
#define     RFU10                 0x10
#define     ModeReg               0x11
#define     TxModeReg             0x12
#define     RxModeReg             0x13
#define     TxControlReg          0x14
#define     TxAutoReg             0x15
#define     TxSelReg              0x16
#define     RxSelReg              0x17
#define     RxThresholdReg        0x18
#define     DemodReg              0x19
#define     RFU1A                 0x1A
#define     RFU1B                 0x1B
#define     MifareReg             0x1C
#define     RFU1D                 0x1D
#define     RFU1E                 0x1E
#define     SerialSpeedReg        0x1F
// PAGE 2
#define     RFU20                 0x20
#define     CRCResultRegM         0x21
#define     CRCResultRegL         0x22
#define     RFU23                 0x23
#define     ModWidthReg           0x24
#define     RFU25                 0x25
#define     RFCfgReg              0x26
#define     GsNReg                0x27
#define     CWGsCfgReg            0x28
#define     ModGsCfgReg           0x29
#define     TModeReg              0x2A
#define     TPrescalerReg         0x2B
#define     TReloadRegH           0x2C
#define     TReloadRegL           0x2D
#define     TCounterValueRegH     0x2E
#define     TCounterValueRegL     0x2F
// PAGE 3
#define     RFU30                 0x30
#define     TestSel1Reg           0x31
#define     TestSel2Reg           0x32
#define     TestPinEnReg          0x33
#define     TestPinValueReg       0x34
#define     TestBusReg            0x35
#define     AutoTestReg           0x36
#define     VersionReg            0x37
#define     AnalogTestReg         0x38
#define     TestDAC1Reg           0x39
#define     TestDAC2Reg           0x3A
#define     TestADCReg            0x3B
#define     RFU3C                 0x3C
#define     RFU3D                 0x3D
#define     RFU3E                 0x3E
#define     RFU3F		  		  0x3F

/////////////////////////////////////////////////////////////////////
//和MF522通讯时返回的错误代码
/////////////////////////////////////////////////////////////////////
#define 	MI_OK                 0
#define 	MI_NOTAGERR           (1)
#define 	MI_ERR                (2)

#define	SHAQU1	0X01
#define	KUAI4	0X04
#define	KUAI7	0X07
#define	REGCARD	0xa1
#define	CONSUME	0xa2
#define READCARD	0xa3
#define ADDMONEY	0xa4

#define SET_SPI_CS   gpio_set_gpio_pin(AVR32_SPI_NPCS_0_1_PIN)
#define CLR_SPI_CS   gpio_clr_gpio_pin(AVR32_SPI_NPCS_0_1_PIN)

#define SET_RC522RST  gpio_set_gpio_pin(AVR32_PIN_PA25)//GPIOF->BSRR=0X02
#define CLR_RC522RST  gpio_clr_gpio_pin(AVR32_PIN_PA25)//GPIOF->BRR=0X02

//void InitRc522(void);

U8 RC522_WriteByte(U8 Data);
U8 RC522_ReadByte(void);

void Powerdown_RC522(U8 act);
void Wait_Wakeup_RC522(void);
void ClearBitMask(U8   reg,U8   mask);
void WriteRawRC(U8   Address, U8   value);
void SetBitMask(U8   reg,U8   mask);
char PcdComMF522(	U8  Command,
					U8  *pIn ,
					U8  InLenByte,
					U8  *pOut ,
					U8  *pOutLenBit);
					
void CalulateCRC(U8 *pIn ,U8   len,U8 *pOut );
U8 ReadRawRC(U8   Address);
char PcdReset(void);
char PcdRequest(unsigned char req_code,unsigned char *pTagType);
void PcdAntennaOn(void);
void PcdAntennaOff(void);
char M500PcdConfigISOType(unsigned char type);
char PcdAnticoll(unsigned char *pSnr);
char PcdSelect(unsigned char *pSnr);
char PcdAuthState(unsigned char auth_mode,unsigned char addr,unsigned char *pKey,unsigned char *pSnr);
char PcdWrite(unsigned char addr,unsigned char *pData);
char PcdRead(unsigned char addr,unsigned char *pData);
char PcdHalt(void);
void Reset_RC522(void);
void RC522_SPI_SetSpeed(U16 SPI_BaudRatePrescaler);
void RC522_SPI_SetSpeedLow(void);
void RC522_SPI_SetSpeedHi(void);



//#define spi_write_dummy()                   spi_write(spi, 0xFF)
//#define spi_write_byte(x)                   spi_write(spi, (U16)x)
//#define spi_read_byte(x)                    spi_read(spi, (U16*)x)


void rc522_init(void);



#endif /* RC522_H_ */