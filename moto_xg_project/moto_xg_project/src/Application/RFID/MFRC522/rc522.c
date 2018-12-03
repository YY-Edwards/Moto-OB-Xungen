/*
 * rc522.c
 *
 * Created: 2017/8/10 星期四 下午 16:22:41
 *  Author: Edwards
 */ 
#include "rc522.h"

volatile avr32_spi_t *spi;
#define spi_write_dummy()                   spi_write(spi, 0xFF)
#define spi_write_byte(x)                   spi_write(spi, (U16)x)
#define spi_read_byte(x)                    spi_read(spi, (U16*)x)


U8 rc522_init_failure =0;

U8 RC522_WriteByte(U8 Data)
{
	U8 temp =0;
	
	//spi_selectChip(spi, DF_SPI_PCS_0);
	
	/*!< Send the byte */
	spi_write(spi,  (U16)Data);
	
	/*!< Wait to receive a byte*/

	temp = spi_read(spi, (U16*)&Data);
	
	//spi_unselectChip(spi, DF_SPI_PCS_0);
	
	/*!< Return the byte read from the SPI bus */
	return  temp;

}
U8 RC522_ReadByte(void)
{
	U16 *Data =NULL ;

	//必须这样才能正常读写
	//spi_selectChip(spi, DF_SPI_PCS_0);
	
	/*!< Send the byte */

	spi_write(spi,  0xFF);
	
	/*!< Return the byte read from the SPI bus */

	spi_read(spi, Data);

	/*!< Return the shifted data */
	
	//spi_unselectChip(spi, DF_SPI_PCS_0);
	
	return (U8)(*Data);
	
}



/////////////////////////////////////////////////////////////////////
//功    能：读RC632寄存器
//参数说明：Address[IN]:寄存器地址
//返    回：读出的值
/////////////////////////////////////////////////////////////////////
U8 ReadRawRC(U8   Address)
{
	U8   ucAddr;
	U8   ucResult=0;
	//CLR_SPI_CS;	
	spi_selectChip(spi, DF_SPI_PCS_0);
	 
	ucAddr = ((Address<<1)&0x7E)|0x80;
	
	RC522_WriteByte(ucAddr);
	ucResult = RC522_ReadByte();
	
	spi_unselectChip(spi, DF_SPI_PCS_0);
	//SET_SPI_CS;
	return ucResult;
}

/////////////////////////////////////////////////////////////////////
//功    能：写RC632寄存器
//参数说明：Address[IN]:寄存器地址
//          value[IN]:写入的值
/////////////////////////////////////////////////////////////////////
void WriteRawRC(U8   Address, U8   value)
{
	U8   ucAddr;

	//CLR_SPI_CS;	
	spi_selectChip(spi, DF_SPI_PCS_0);
	
	ucAddr = ((Address<<1)&0x7E);
	RC522_WriteByte(ucAddr);
	RC522_WriteByte(value);
	
	spi_unselectChip(spi, DF_SPI_PCS_0);
	
	//SET_SPI_CS;

}

void RC522_SPI_SetSpeed(U16 SPI_BaudRatePrescaler)
{
	spi->csr0 = (spi->csr0 & (U16)0x00FF) |SPI_BaudRatePrescaler;

	spi_enable(spi); /*!< SD_SPI enable */
		
	
}
void RC522_SPI_SetSpeedLow(void)
{
	RC522_SPI_SetSpeed(0x0A00);//baudDiv=4
	
}
void RC522_SPI_SetSpeedHi(void)
{
	
	RC522_SPI_SetSpeed(0x0100);//baudDiv=1
	
}



void static spi_init(void)
{
//	U16 status = 0xff;

	static const gpio_map_t RC522_SPI_GPIO_MAP =
	{
		{AVR32_SPI_SCK_0_1_PIN,        AVR32_SPI_SCK_0_1_FUNCTION   },  // SPI Clock. PA17 as SCK (func C)
		{AVR32_SPI_MISO_0_2_PIN,       AVR32_SPI_MISO_0_2_FUNCTION  },  // MISO. PA28 as MISO (func C)
		{AVR32_SPI_MOSI_0_2_PIN,       AVR32_SPI_MOSI_0_2_FUNCTION  },  // MOSI. PA29 as MOSI (func C)
		{AVR32_SPI_NPCS_0_1_PIN,       AVR32_SPI_NPCS_0_1_FUNCTION  },  // Chip Select NPCS. PA24 as NPCS[0] (func B)
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = DF_SPI_FIRST_NPCS,   // PCS0
		.baudrate     = DF_SPI_MASTER_SPEED, // 24MHz
		.bits         = DF_SPI_BITS,         // 8 bit per transfer
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 0,
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(RC522_SPI_GPIO_MAP, sizeof(RC522_SPI_GPIO_MAP) / sizeof(RC522_SPI_GPIO_MAP[0]));

	// Configure PA25 as RST pin
	gpio_enable_gpio_pin(AVR32_PIN_PA25);
	gpio_set_gpio_pin(AVR32_PIN_PA25);
	
	//gpio_enable_gpio_pin(AVR32_PIN_PA24);

	spi = &AVR32_SPI;

	// Initialize as master.
	spi_initMaster(spi, &spiOptions);

	// Set selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(spi, 0, 0, 0);

	// Enable SPI.
	spi_enable(spi);

	// Initialize RC522 with SPI clock PBA.
	if (spi_setupChipReg(spi, &spiOptions, 2*12000000) != SPI_OK)
	{
		
		rc522_init_failure = FATAL_ERROR_RC522_SPI_INIT;
		return;
	}
	
	RC522_SPI_SetSpeedLow();
	
	
}
/////////////////////////////////////////////////////////////////////
//功    能：清RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//         mask[IN]:清位值
/////////////////////////////////////////////////////////////////////
void ClearBitMask(U8   reg,U8   mask)
{
	  char   tmp = 0x0;
	  tmp = ReadRawRC(reg);
	  WriteRawRC(reg, tmp & ~mask);  // clear bit mask
	
}


void Reset_RC522(void)
{
	PcdReset();
	PcdAntennaOff();
	delay_ms(2);
	PcdAntennaOn();
}

/////////////////////////////////////////////////////////////////////
//功    能：寻卡
//参数说明: req_code[IN]:寻卡方式
//                0x52 = 寻感应区内所有符合14443A标准的卡
//                0x26 = 寻未进入休眠状态的卡
//          pTagType[OUT]：卡片类型代码
//                0x4400 = Mifare_UltraLight
//                0x0400 = Mifare_One(S50)
//                0x0200 = Mifare_One(S70)
//                0x0800 = Mifare_Pro(X)
//                0x4403 = Mifare_DESFire
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRequest(U8   req_code,U8 *pTagType)
{
	char   status;
	U8   unLen;
	U8   ucComMF522Buf[MAXRLEN];

	ClearBitMask(Status2Reg,0x08);
	WriteRawRC(BitFramingReg,0x07);
	SetBitMask(TxControlReg,0x03);
	
	ucComMF522Buf[0] = req_code;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,1,ucComMF522Buf,&unLen);

	if ((status == MI_OK) && (unLen == 0x10))
	{
		*pTagType     = ucComMF522Buf[0];
		*(pTagType+1) = ucComMF522Buf[1];
	}
	else
	{   status = MI_ERR;   }
	
	return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：防冲撞
//参数说明: pSnr[OUT]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAnticoll(U8 *pSnr)
{
	char   status;
	U8   i,snr_check=0;
	U8   unLen;
	U8   ucComMF522Buf[MAXRLEN];
	

	ClearBitMask(Status2Reg,0x08);
	WriteRawRC(BitFramingReg,0x00);
	ClearBitMask(CollReg,0x80);
	
	ucComMF522Buf[0] = PICC_ANTICOLL1;
	ucComMF522Buf[1] = 0x20;

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,2,ucComMF522Buf,&unLen);

	if (status == MI_OK)
	{
		for (i=0; i<4; i++)
		{
			*(pSnr+i)  = ucComMF522Buf[i];
			snr_check ^= ucComMF522Buf[i];
		}
		if (snr_check != ucComMF522Buf[i])
		{   status = MI_ERR;    }
	}
	
	SetBitMask(CollReg,0x80);
	return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：选定卡片
//参数说明: pSnr[IN]:卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdSelect(U8 *pSnr)
{
	char   status;
	U8   i;
	U8   unLen;
	U8   ucComMF522Buf[MAXRLEN];
	
	ucComMF522Buf[0] = PICC_ANTICOLL1;
	ucComMF522Buf[1] = 0x70;
	ucComMF522Buf[6] = 0;
	for (i=0; i<4; i++)
	{
		ucComMF522Buf[i+2] = *(pSnr+i);
		ucComMF522Buf[6]  ^= *(pSnr+i);
	}
	CalulateCRC(ucComMF522Buf,7,&ucComMF522Buf[7]);
	
	ClearBitMask(Status2Reg,0x08);

	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,9,ucComMF522Buf,&unLen);
	
	if ((status == MI_OK) && (unLen == 0x18))
	{   status = MI_OK;  }
	else
	{   status = MI_ERR;    }

	return status;
}



/////////////////////////////////////////////////////////////////////
//功    能：验证卡片密码
//参数说明: auth_mode[IN]: 密码验证模式
//                 0x60 = 验证A密钥
//                 0x61 = 验证B密钥
//          addr[IN]：块地址
//          pKey[IN]：密码
//          pSnr[IN]：卡片序列号，4字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdAuthState(U8   auth_mode,U8   addr,U8 *pKey,U8 *pSnr)
{
	char   status;
	U8   unLen;
	U8   i,ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = auth_mode;
	ucComMF522Buf[1] = addr;
	//    for (i=0; i<6; i++)
	//    {    ucComMF522Buf[i+2] = *(pKey+i);   }
	//    for (i=0; i<6; i++)
	//    {    ucComMF522Buf[i+8] = *(pSnr+i);   }
	memcpy(&ucComMF522Buf[2], pKey, 6);
	memcpy(&ucComMF522Buf[8], pSnr, 4);
	
	status = PcdComMF522(PCD_AUTHENT,ucComMF522Buf,12,ucComMF522Buf,&unLen);
	if ((status != MI_OK) || (!(ReadRawRC(Status2Reg) & 0x08)))
	{   status = MI_ERR;   }
	
	return status;
}


/////////////////////////////////////////////////////////////////////
//功    能：置RC522寄存器位
//参数说明：reg[IN]:寄存器地址
//          mask[IN]:置位值
/////////////////////////////////////////////////////////////////////
void SetBitMask(U8   reg,U8   mask)
{
	char   tmp = 0x0;
	tmp = ReadRawRC(reg);
	WriteRawRC(reg,tmp | mask);  // set bit mask
}

/////////////////////////////////////////////////////////////////////
//功    能：使RC522进入软掉电模式
//
/////////////////////////////////////////////////////////////////////
void Powerdown_RC522(U8 act)
{
	char   tmp = 0x0;
	char   reg = CommandReg;
	
	tmp = ReadRawRC(reg);
	if(act == ENTER_POWERDOWN)//1
		WriteRawRC(reg,tmp | PCD_POWERDOWN);  // set bit powerdown
	else
	{
		WriteRawRC(reg,tmp | PCD_IDLE);  // wake up rc522
		delay_ns(2);
		Wait_Wakeup_RC522();
	}
}


/////////////////////////////////////////////////////////////////////
//功    能：等待RC522启动
/////////////////////////////////////////////////////////////////////
void Wait_Wakeup_RC522(void)
{
	char   tmp = 0x0;
	char   reg = CommandReg;
	
	do 
	{
		tmp = ReadRawRC(reg);
		//delay_ms(20);
				
	} while ((tmp & 0x10) == 1);
			
	//tmp = ReadRawRC(reg);
	//if((tmp & 0x10) == 0)
	//WriteRawRC(reg,tmp | PCD_POWERDOWN);  // set bit powerdown
	
}

/////////////////////////////////////////////////////////////////////
//功    能：读取M1卡一块数据
//参数说明: addr[IN]：块地址
//          p [OUT]：读出的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdRead(U8   addr,U8 *p )
{
	char   status;
	U8   unLen;
	U8   i,ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_READ;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
	
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);
	if ((status == MI_OK) && (unLen == 0x90))
	//   {   memcpy(p , ucComMF522Buf, 16);   }
	{
		for (i=0; i<16; i++)
		{    *(p +i) = ucComMF522Buf[i];   }
	}
	else
	{   status = MI_ERR;   }
	
	return status;
}

/////////////////////////////////////////////////////////////////////
//功    能：写数据到M1卡一块
//参数说明: addr[IN]：块地址
//          p [IN]：写入的数据，16字节
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdWrite(U8   addr,U8 *p )
{
	char   status;
	U8   unLen;
	U8   i,ucComMF522Buf[MAXRLEN];
	
	ucComMF522Buf[0] = PICC_WRITE;
	ucComMF522Buf[1] = addr;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
	
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
	{   status = MI_ERR;   }
	
	if (status == MI_OK)
	{
		//memcpy(ucComMF522Buf, p , 16);
		for (i=0; i<16; i++)
		{
			ucComMF522Buf[i] = *(p +i);
		}
		CalulateCRC(ucComMF522Buf,16,&ucComMF522Buf[16]);

		status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,18,ucComMF522Buf,&unLen);
		if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
		{   status = MI_ERR;   }
	}
	
	return status;
}



/////////////////////////////////////////////////////////////////////
//功    能：命令卡片进入休眠状态
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdHalt(void)
{
	U8   status;
	U8   unLen;
	U8   ucComMF522Buf[MAXRLEN];

	ucComMF522Buf[0] = PICC_HALT;
	ucComMF522Buf[1] = 0;
	CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
	
	status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

	return MI_OK;
}



/////////////////////////////////////////////////////////////////////
//用MF522计算CRC16函数
/////////////////////////////////////////////////////////////////////
void CalulateCRC(U8 *pIn ,U8   len,U8 *pOut )
{
	U8   i,n;
	ClearBitMask(DivIrqReg,0x04);
	WriteRawRC(CommandReg,PCD_IDLE);
	SetBitMask(FIFOLevelReg,0x80);
	for (i=0; i<len; i++)
	{   WriteRawRC(FIFODataReg, *(pIn +i));   }
	WriteRawRC(CommandReg, PCD_CALCCRC);
	i = 0xFF;
	do
	{
		n = ReadRawRC(DivIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x04));
	pOut [0] = ReadRawRC(CRCResultRegL);
	pOut [1] = ReadRawRC(CRCResultRegM);
}



/////////////////////////////////////////////////////////////////////
//功    能：复位RC522
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
char PcdReset(void)
{

	SET_RC522RST;
	delay_ns(10);

	CLR_RC522RST;
	delay_ns(10);

	SET_RC522RST;
	delay_ns(10);
	WriteRawRC(CommandReg,PCD_RESETPHASE);
	WriteRawRC(CommandReg,PCD_RESETPHASE);
	delay_ns(10);
	
	WriteRawRC(ModeReg,0x3D);            //和Mifare卡通讯，CRC初始值0x6363
	WriteRawRC(TReloadRegL,30);
	WriteRawRC(TReloadRegH,0);
	WriteRawRC(TModeReg,0x8D);
	WriteRawRC(TPrescalerReg,0x3E);
	
	WriteRawRC(TxAutoReg,0x40);//必须要
	
	return MI_OK;
}


//////////////////////////////////////////////////////////////////////
//设置RC632的工作方式
//////////////////////////////////////////////////////////////////////
char M500PcdConfigISOType(U8   type)
{
	if (type == 'A')                     //ISO14443_A
	{
		ClearBitMask(Status2Reg,0x08);
		WriteRawRC(ModeReg,0x3D);//3F
		WriteRawRC(RxSelReg,0x86);//84
		WriteRawRC(RFCfgReg,0x7F);   //4F
		WriteRawRC(TReloadRegL,30);//tmoLength);// TReloadVal = 'h6a =tmoLength(dec)
		WriteRawRC(TReloadRegH,0);
		WriteRawRC(TModeReg,0x8D);
		WriteRawRC(TPrescalerReg,0x3E);
		delay_ns(1000);
		PcdAntennaOn();
	}
	else{ return 1; }
	
	return MI_OK;
}


/////////////////////////////////////////////////////////////////////
//功    能：通过RC522和ISO14443卡通讯
//参数说明：Command[IN]:RC522命令字
//          pIn [IN]:通过RC522发送到卡片的数据
//          InLenByte[IN]:发送数据的字节长度
//          pOut [OUT]:接收到的卡片返回数据
//          *pOutLenBit[OUT]:返回数据的位长度
/////////////////////////////////////////////////////////////////////
char PcdComMF522(U8  Command,
				 U8 *pIn ,
				 U8  InLenByte,
				 U8 *pOut ,
				 U8 *pOutLenBit)
{
	char   status = MI_ERR;
	U8   irqEn   = 0x00;
	U8   waitFor = 0x00;
	U8   lastBits;
	U8   n;
	U16   i;
	
	switch (Command)
	{
		case PCD_AUTHENT:
		irqEn   = 0x12;
		waitFor = 0x10;
		break;
		case PCD_TRANSCEIVE:
		irqEn   = 0x77;
		waitFor = 0x30;
		break;
		default:
		break;
	}
	
	WriteRawRC(ComIEnReg,irqEn|0x80);
	ClearBitMask(ComIrqReg,0x80);	//清所有中断位
	WriteRawRC(CommandReg,PCD_IDLE);
	SetBitMask(FIFOLevelReg,0x80);	 	//清FIFO缓存
	
	for (i=0; i<InLenByte; i++)
	{   
		WriteRawRC(FIFODataReg, pIn [i]);
	}
	WriteRawRC(CommandReg, Command);
	
	if (Command == PCD_TRANSCEIVE)
	{    
		SetBitMask(BitFramingReg,0x80);  
	}	 //开始传送
	
	//i = 600;//根据时钟频率调整，操作M1卡最大等待时间25ms
	i = 2000;
	do
	{
		n = ReadRawRC(ComIrqReg);
		i--;
	}
	while ((i!=0) && !(n&0x01) && !(n&waitFor));
	ClearBitMask(BitFramingReg,0x80);

	if (i!=0)
	{
		if(!(ReadRawRC(ErrorReg)&0x1B))
		{
			status = MI_OK;
			if (n & irqEn & 0x01)
			{   status = MI_NOTAGERR;   }
			if (Command == PCD_TRANSCEIVE)
			{
				n = ReadRawRC(FIFOLevelReg);
				lastBits = ReadRawRC(ControlReg) & 0x07;
				if (lastBits)
				{   *pOutLenBit = (n-1)*8 + lastBits;   }
				else
				{   *pOutLenBit = n*8;   }
				if (n == 0)
				{   n = 1;    }
				if (n > MAXRLEN)
				{   n = MAXRLEN;   }
				for (i=0; i<n; i++)
				{   pOut [i] = ReadRawRC(FIFODataReg);    }
			}
		}
		else
		{   status = MI_ERR;   }
		
	}
	

	SetBitMask(ControlReg,0x80);           // stop timer now
	WriteRawRC(CommandReg,PCD_IDLE);
	return status;
}

/////////////////////////////////////////////////////////////////////
//开启天线
//每次启动或关闭天险发射之间应至少有1ms的间隔
/////////////////////////////////////////////////////////////////////
void PcdAntennaOn(void)
{
	U8   i;
	i = ReadRawRC(TxControlReg);
	if (!(i & 0x03))
	{
		SetBitMask(TxControlReg, 0x03);
	}
}


/////////////////////////////////////////////////////////////////////
//关闭天线
/////////////////////////////////////////////////////////////////////
void PcdAntennaOff(void)
{
	ClearBitMask(TxControlReg, 0x03);
}

/////////////////////////////////////////////////////////////////////
//功    能：扣款和充值
//参数说明: dd_mode[IN]：命令字
//               0xC0 = 扣款
//               0xC1 = 充值
//          addr[IN]：钱包地址
//          pValue[IN]：4字节增(减)值，低位在前
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////                 
/*char PcdValue(U8 dd_mode,U8 addr,U8 *pValue)
{
    char status;
    U8  unLen;
    U8 ucComMF522Buf[MAXRLEN]; 
    //U8 i;
	
    ucComMF522Buf[0] = dd_mode;
    ucComMF522Buf[1] = addr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
        
    if (status == MI_OK)
    {
        memcpy(ucComMF522Buf, pValue, 4);
        //for (i=0; i<16; i++)
        //{    ucComMF522Buf[i] = *(pValue+i);   }
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
        unLen = 0;
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
		if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = PICC_TRANSFER;
        ucComMF522Buf[1] = addr;
        CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]); 
   
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

        if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
        {   status = MI_ERR;   }
    }
    return status;
}*/

/////////////////////////////////////////////////////////////////////
//功    能：备份钱包
//参数说明: sourceaddr[IN]：源地址
//          goaladdr[IN]：目标地址
//返    回: 成功返回MI_OK
/////////////////////////////////////////////////////////////////////
/*char PcdBakValue(U8 sourceaddr, U8 goaladdr)
{
    char status;
    U8  unLen;
    U8 ucComMF522Buf[MAXRLEN]; 

    ucComMF522Buf[0] = PICC_RESTORE;
    ucComMF522Buf[1] = sourceaddr;
    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }
    
    if (status == MI_OK)
    {
        ucComMF522Buf[0] = 0;
        ucComMF522Buf[1] = 0;
        ucComMF522Buf[2] = 0;
        ucComMF522Buf[3] = 0;
        CalulateCRC(ucComMF522Buf,4,&ucComMF522Buf[4]);
 
        status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,6,ucComMF522Buf,&unLen);
		if (status != MI_ERR)
        {    status = MI_OK;    }
    }
    
    if (status != MI_OK)
    {    return MI_ERR;   }
    
    ucComMF522Buf[0] = PICC_TRANSFER;
    ucComMF522Buf[1] = goaladdr;

    CalulateCRC(ucComMF522Buf,2,&ucComMF522Buf[2]);
 
    status = PcdComMF522(PCD_TRANSCEIVE,ucComMF522Buf,4,ucComMF522Buf,&unLen);

    if ((status != MI_OK) || (unLen != 4) || ((ucComMF522Buf[0] & 0x0F) != 0x0A))
    {   status = MI_ERR;   }

    return status;
}*/







//mfrc522 init

void rc522_init()
{
	
	spi_init();
	
	PcdReset();
	
	PcdAntennaOff();
	
	delay_ms(2); 
	
	PcdAntennaOn();
	
	M500PcdConfigISOType( 'A' );
	
	Powerdown_RC522(ENTER_POWERDOWN);
	
	//Powerdown_RC522(WAKEUP_RC522);
	
	//Powerdown_RC522(ENTER_POWERDOWN);
	
	//Powerdown_RC522(WAKEUP_RC522);
	

}