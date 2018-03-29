/*******************************************************************************
*
*                         C  S O U R C E  F I L E
*
*            COPYRIGHT 2008 MOTOROLA, INC. ALL RIGHTS RESERVED.
*                    MOTOROLA CONFIDENTIAL PROPRIETARY
*
********************************************************************************
*
* FILE NAME : data_flash.c
*
*-------------------------------- PURPOSE --------------------------------------
*
* The purpose of this file is to provide data flash driver.
*
* W25Q64(AT25DF641) is a 64MBIT(8MB) SPI serial flash, with address range from
* 0x000000 to 0x7FFFFF.
*
*--------------------------- DEPENDENCY COMMENTS -------------------------------
*
*
*-------------------------- PROJECT SPECIFIC DATA ------------------------------
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-09   a21961    none        Initial draft
*
*******************************************************************************/
#include "data_flash.h"

volatile avr32_spi_t *spi;
static Bool data_flash_check_device_id(void);
static void test_data_flash(Bool bReadOnlyTest);
void create_data_flash_test_task(void);
void runDataFlashTest( void *pvParameters );
U8 data_flash_failure = 0;

U8 FLASH_BUF[4096];




void W25Q64_SPI_SetSpeed(U16 SPI_BaudRatePrescaler)
{
	spi->csr1 = (spi->csr1 & (U16)0x00FF) |SPI_BaudRatePrescaler;

	spi_enable(spi); /*!< W25Q64_SPI enable */
	
	
}
void W25Q64_SPI_SetSpeedLow(void)
{
	W25Q64_SPI_SetSpeed(0x0200);//baudDiv=4
	
}
void W25Q64_SPI_SetSpeedHi(void)
{
	
	W25Q64_SPI_SetSpeed(0x0100);//baudDiv=1
	
}


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_init
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by xg_flashc_init() to initialize data flash driver.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Following things are done in this funciton:
* 1. SPI GPIO configuration.
* 2. SPI configuration.
* 3. Check Manufacturer ID and device ID.
* 4. Unprotect all sect.
* 5. Create Data Flash Test task.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
void data_flash_init(void)
{
//	U16 status = 0xff;

	static const gpio_map_t DF_SPI_GPIO_MAP =
	{
			{AVR32_SPI_SCK_0_1_PIN,        AVR32_SPI_SCK_0_1_FUNCTION   },  // SPI Clock. PA17 as SCK (func C)
			{AVR32_SPI_MISO_0_2_PIN,       AVR32_SPI_MISO_0_2_FUNCTION  },  // MISO. PA28 as MISO (func C)
			{AVR32_SPI_MOSI_0_2_PIN,       AVR32_SPI_MOSI_0_2_FUNCTION  },  // MOSI. PA29 as MOSI (func C)
			{AVR32_SPI_NPCS_1_1_PIN,       AVR32_SPI_NPCS_1_1_FUNCTION  },  // Chip Select NPCS. PA23 as NPCS[1] (func B)
	};

	// SPI options.
	spi_options_t spiOptions =
	{
		.reg          = DF_SPI_SECOND_NPCS,   // PCS1
		.baudrate     = DF_SPI_MASTER_SPEED, // 24MHz
		.bits         = DF_SPI_BITS,         // 8 bit per transfer
		.spck_delay   = 0,
		.trans_delay  = 0,
		.stay_act     = 1,
		.spi_mode     = 1,//0,0 针对不同的存储芯片，注意模式
		.modfdis      = 1
	};

	// Assign I/Os to SPI.
	gpio_enable_module(DF_SPI_GPIO_MAP, sizeof(DF_SPI_GPIO_MAP) / sizeof(DF_SPI_GPIO_MAP[0]));

	spi = &AVR32_SPI;

	// Initialize as master.
	spi_initMaster(spi, &spiOptions);

	// Set selection mode: variable_ps, pcs_decode, delay.
	spi_selectionMode(spi, 0, 0, 0);

	// Enable SPI.
	spi_enable(spi);

	W25Q64_SPI_SetSpeedLow();

	// Initialize data flash with SPI clock PBA.
	if (spi_setupChipReg(spi, &spiOptions, 2*12000000) != SPI_OK)
	{
		data_flash_failure = FATAL_ERROR_DATA_FLASH_SPI_INIT;
		return;
	}
	 
	if (data_flash_check_device_id() != TRUE)//check W25Q64 ID
	{
		data_flash_failure = FATAL_ERROR_DATA_FLASH_READ_ID;
		return;
	}


	//W25Q64_SPI_SetSpeedHi();
	// Set STATUS reg to unprotect all sect
	//send_flash_command(WRITE_ENABLE, 0, NULL, 0);
	//send_flash_command(WRITE_STATUS_REG_BYTE_1, 0, NULL, 0);
	//status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
	
	return;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_check_device_id
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by data_flash_init() to check manufacturer/device id.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* For W25Q64, manufacturer id is 0xEF and device_id is 0x16.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
static Bool data_flash_check_device_id(void)
{
	U16 manufacturer_device_id[2] = {0x5A5A, 0x5A5A};

	/* DF memory check. */
	/* Select the DF memory to check. */
	spi_selectChip(spi, DF_SPI_PCS_1);

	/* Send the Manufacturer/Device ID Read command. */
	spi_write(spi, READ_M_D_ID);	
	spi_write_zero();
	spi_write_zero();
	spi_write_zero();

	/* Send 2 dummy byte to read the status register. */
	
	spi_write_dummy();
	spi_read(spi, &manufacturer_device_id[0]);
	
	spi_write_dummy();
	spi_read(spi, &manufacturer_device_id[1]);

	/* Unselect the checked DF memory. */
	spi_unselectChip(spi, DF_SPI_PCS_1);

	/* Unexpected device ID. */
    if ((manufacturer_device_id[0] != 0xEF) || (manufacturer_device_id[1] != 0x16))
    {
    	return FALSE;
    }

    return TRUE;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: send_flash_command
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is provide as external interface of data flash driver.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* This function is the "raw" driver, i.e, without consideration for page wrap,
* etc. Please refer to W25Q64(AT25DF641) datasheet for detail.
*
* Caller of this function must follow below rules for input param:
*
* COMMAND                    ADDRESS    DATA_PTR     LENGTH    Return value
* -------------------------------------------------------------------------
* READ_STATUS_REG            0          NULL         0         status
* WRITE_ENABLE               0          NULL         0         Not Care
* WRITE_DISABLE              0          NULL         0         Not Care
* BLOCK_ERASE_4KB            address    NULL         0         Not Care
* BYTE_PROGRAM               address    data_ptr     length    Not Care
* READ_ARRAY                 address    data_ptr     length    Not Care
* WRITE_STATUS_REG_BYTE_1    0          NULL         0         Not Care
* -------------------------------------------------------------------------
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
U16 send_flash_command(U16 command, U32 address, U8 *data_ptr, U16 length)
{
	U16 status = 1;
	U16 i = 0;

    /* in order to call spi_write(U16* data), change 3-bytes address to 3-U16 address */
	U16 addr[3];
	addr[2] = (address & 0x00ff0000) >> 16;
	addr[1] = (address & 0x0000ff00) >> 8;
	addr[0] = (address & 0x000000ff);

	U16 data_u16;

	spi_selectChip(spi, DF_SPI_PCS_1);

    switch (command)
	{
		case READ_STATUS_REG:
			spi_write_byte(command);
			spi_write_dummy();
			spi_read_byte(&status);
			break;
		case WRITE_ENABLE:
		case WRITE_DISABLE:
			spi_write_byte(command);
			break;
		case BLOCK_ERASE_4KB:
		case BLOCK_ERASE_32KB:
		case BLOCK_ERASE_64KB:
			spi_write_byte(command);
			spi_write_byte(addr[2]);
			spi_write_byte(addr[1]);
			spi_write_byte(addr[0]);
			break;
		case CHIP_ERASE:
			spi_write_byte(command);
			break;
		case BYTE_PROGRAM: /* PAGE_PROGRAM use the same value */
			spi_write_byte(command);
			spi_write_byte(addr[2]);
			spi_write_byte(addr[1]);
			spi_write_byte(addr[0]);
			for (i = 0; i < length; i++)
			{
				data_u16 = (*data_ptr)&0x00ff;//modified by huayi
				data_ptr++;
				spi_write_byte(data_u16);
			}
			break;
		case READ_ARRAY:
			spi_write_byte(command);
			spi_write_byte(addr[2]);
			spi_write_byte(addr[1]);
			spi_write_byte(addr[0]);
			for (i = 0; i < length; i++)
			{
				spi_write_dummy();
				spi_read_byte(&data_u16);
				*data_ptr = (U8)data_u16;
				data_ptr++;
			}
			break;
		case WRITE_STATUS_REG_BYTE_1:
			spi_write_byte(command);
			spi_write_byte(UNPROTECT_ALL_SECTORS);
			spi_write_byte(UNPROTECT_ALL_SECTORS);
			break;
		default:
			break;
	}

	spi_unselectChip(spi, DF_SPI_PCS_1);

	return status;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_test
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is to test data flash erase/write/read functionality.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Depends on input parameter value, this function performs 2 kinds of tests:
* 1. Write/Read 0x5555.
* 2. Write/Read 0xAAAA.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 27-Feb-10   mdpq74    none        Initial draft
*
*******************************************************************************/
static int data_flash_test(U8 value)
{
	/* write 0x5555 or 0xAAAA to address 0x00001002 then read it back */
	U8 data_write_in[2] = {value, value};
	U8 data_read_out[2] = {0, 0};
	U32 test_address = 0x00001002;
	U16 status = 1;
	U16 count = 0;

	/* test both write and read */
	send_flash_command(READ_ARRAY, test_address, data_read_out, 2);
	data_read_out[0] = 0;
	data_read_out[1] = 0;

	status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
	status = 0;

	send_flash_command(WRITE_ENABLE, 0, NULL, 0);

	send_flash_command(BLOCK_ERASE_4KB, test_address, NULL, 0);

	do {
		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		count++;
	} while((status & STATUS_BUSY) != 0);

	send_flash_command(WRITE_ENABLE, 0, NULL, 0);

	send_flash_command(BYTE_PROGRAM, test_address, data_write_in, 2);

	count = 0;
	do {
		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		count++;
	} while((status & STATUS_BUSY) != 0);

	send_flash_command(WRITE_DISABLE, 0, NULL, 0);

	send_flash_command(READ_ARRAY, test_address, data_read_out, 2);

	status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);

	if((data_read_out[0]==value)&&(data_read_out[1]==value))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: test_data_flash
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is to test data flash erase/write/read functionality.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Depends on input param bReadOnlyTest, this function performs 2 kinds of tests:
* 1. Read/Write test.
* 2. Read only test.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
void test_data_flash(Bool bReadOnlyTest)
{
	/* write 0x5A5A to address 0x00000000 then read it back */
	U8 data_write_in[2] = {0x5A, 0x72};
	U8 data_read_out[2] = {0, 0};
	//U32 test_address = 0x00001002;
	U32 test_address = 0x00000000;
	U16 status = 1;
	U16 count = 0;

	if (bReadOnlyTest == FALSE)
	{
		/* test both write and read */
		send_flash_command(READ_ARRAY, test_address, data_read_out, 2);
		data_read_out[0] = 0;
		data_read_out[1] = 0;

		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		status = 0;

		send_flash_command(WRITE_ENABLE, 0, NULL, 0);

		send_flash_command(BLOCK_ERASE_4KB, test_address, NULL, 0);

		do {
			status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
			count++;
		} while((status & STATUS_BUSY) != 0);
		
		//send_flash_command(WRITE_DISABLE, 0, NULL, 0);
		//read again
		send_flash_command(READ_ARRAY, test_address, data_read_out, 2);
		if((data_read_out[0] !=0xFF) | ((data_read_out[1] !=0xFF)) )
		{
			mylog("\n----flash erase fail----\n");			
		}
		
		send_flash_command(WRITE_ENABLE, 0, NULL, 0);

		send_flash_command(BYTE_PROGRAM, test_address, data_write_in, 2);

		count = 0;
		do {
			status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
			count++;
		} while((status & STATUS_BUSY) != 0);

		send_flash_command(WRITE_DISABLE, 0, NULL, 0);

		send_flash_command(READ_ARRAY, test_address, data_read_out, 2);

		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		
		while(1)
		{
			 /* read only test */
			 data_read_out[0] = 0x0;
			 data_read_out[1] = 0x0;
			 send_flash_command(READ_ARRAY, test_address, data_read_out, 2);
		}


	    if ((data_read_out[0] == 0x5A) || (data_read_out[1] == 0x5A))
	    {
	    	mylog("\n----Flash r/w test pass----\n");
			//print_text_time("Flash r/w test pass", 4);
	    }
	    else
	    {
			mylog("\n----Flash r/w test fail----\n");
	    	//print_text_time("Flash r/w test fail", 4);
	    }

	}
	else
	{
        /* read only test */
		data_read_out[0] = 0x0;
		data_read_out[1] = 0x0;
		send_flash_command(READ_ARRAY, test_address, data_read_out, 2);

	    if ((data_read_out[0] != 0x5A) || (data_read_out[1] != 0x5A))
	    {
	    	//print_text_time("Flash test fail", 4);
			mylog("\n----Flash read test fail----\n");
		}
	    else
	    {
	    	mylog("\n----Flash read test pass----\n");
			//print_text_time("Flash test pass", 4);
	    }
	}

	return;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: create_data_flash_test_task
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by data_flash_init() to create task for data flash.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
void create_data_flash_test_task(void)
{
	xTaskCreate(
			runDataFlashTest
			,  (const signed portCHAR *)"DFT"
			,  configMINIMAL_STACK_SIZE
			,  NULL
			,  tskFLASH_PRIORITY
			,  NULL );
}


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: runDataFlashTest
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is the main function for data flash test task.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
* Upon powerup, will do read/write test.
* Then do a read only test every 15000 tick.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 10-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
void runDataFlashTest( void *pvParameters )
{
	Bool firstTest = TRUE;
	static  portTickType xLastWakeTime;
	const portTickType xFrequency = 4000;//2s,定时问题已经修正。2s x  2000hz = 4000
	
	 xLastWakeTime = xTaskGetTickCount();
	 
	while(1)
	{
		vTaskDelayUntil( &xLastWakeTime, xFrequency / portTICK_RATE_MS  );//精确的以1000ms为周期执行。

		if(1)
		{
			if (firstTest)
			{
				// write test for the first time
				test_data_flash(FALSE);
				firstTest = FALSE;
			}
			else
			{
				test_data_flash(TRUE);
			}
		}
	}
}


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: serial_flash_test
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by factory_test_hardware()in bootloader.c
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Following things are done in this funciton:
* 1. write 0xAA to flash address 0x00001002 and read it back , compare the send
*    and receive data. if same , goto 2. else goto 3.
* 2. write 0x55 to flash address 0x00001002 and read it back , compare the send
*    and receive data. if same, return success . else goto 3.
* 3. return failed.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 27-Feb-09   a21961&mdpq74    none        Initial draft
*
*******************************************************************************/

/* for test the serial flash , first write 0xAA to one location of serial flash
   then ,read it back and compare whether same . if same , then write 0X55 to the
   same location and read back to compare . if not same , then write Factory Test
   Reply byte 2 as fail. otherwise , write it as success. */

uint8_t serial_flash_test(void)
{
	if (data_flash_failure == 0)
	{
		if(data_flash_test(0xAA))
		{
			if(data_flash_test(0x55))
			{
				return (uint8_t)DF_TEST_SUCCESS;
			}else
			{
				return (uint8_t)DF_TEST_FAIL;
			}
		}else
		{
			return (uint8_t)DF_TEST_FAIL;
		}
	}
	else
	{
		return (uint8_t)DF_TEST_FAIL;
	}
}
/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_erase_block
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is provide as external interface to erase data flash by block.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
* Caller must provide address within destination block and the block size.
* The block size can be either 4, 32 or 64K bytes.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 25-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
df_status_t data_flash_erase_block(U32 address, df_block_size_t block_size)
{
	U16 status = 1;
	U16 erase_commond;
	df_status_t return_code = DF_ERASE_FAIL;
	U16 count = 0; /* to monitor erase time consumption */

	/* check input parameter */
	if (address >= DF_MAX_ADDR)
	{
		return DF_INVALID_PARAM;
	}

	/* determine erase command to be issued */
	if (block_size == DF_BLOCK_4KB)
		erase_commond = BLOCK_ERASE_4KB;
	else if (block_size == DF_BLOCK_32KB)
		erase_commond = BLOCK_ERASE_32KB;
	else if(block_size == DF_BLOCK_64KB)
		erase_commond = BLOCK_ERASE_64KB;
	else/*(block_size == DF_BLOCK_ALL)*/
		erase_commond = CHIP_ERASE;

	while((status = send_flash_command(READ_STATUS_REG, 0, 0, 0)) == STATUS_BUSY);
	//status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
	//if ((status & STATUS_BUSY) != 0)
	//{
		//return DF_DEVICE_BUSY;
	//}

	/* send WRITE_ENABLE command */
	send_flash_command(WRITE_ENABLE, 0, NULL, 0);

	/* send BLOCK_ERASE command */
	send_flash_command(erase_commond, address, NULL, 0);
	do {
		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		count++;
	} while((status & STATUS_BUSY) != 0);

	/*  Check Status Register */
	if ((status & STATUS_ERASE_PROG_ERROR) != 0)
	{
		return_code = DF_ERASE_FAIL;
	}
#if 0
	else if (((status & STATUS_WRITE_NOT_ENABLED) != 0) ||
			 ((status & STATUS_SECTOR_PROTECTED)  != 0) ||
			 ((status & STATUS_WRITE_PROTECTED)   != 0))
	{
		return_code = DF_WRITE_DISABLED;
	}
#endif
	else
	{
		/*  Erase Successful.  */
		return_code = DF_ERASE_COMPLETED;
	}

	/* send WRITE_DISABLE command */
	send_flash_command(WRITE_DISABLE, 0, NULL, 0);

	return return_code;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_write
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is provide as external interface to write data flash with
* specified source data, destination address and data length.(and erase function)
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* To keep write operation more controllable, a 4KB limitation is applied on input
* parameter length.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 25-Fri-17   Edwards    none        Initial draft
*
*******************************************************************************/
df_status_t data_flash_write(U8 *data_ptr, U32 address, U16 data_length)
{
	U32 secpos;
	U16 secoff;
	U16 secremain;
	U16 i;
	df_status_t return_code = DF_OK;

	secpos	=	address/4096;//扇区地址 0~2047 for AT25DF641 
	secoff	=	address%4096;//在扇区内的偏移
	secremain	=	4096-secoff;//扇区剩余空间大小
	if(data_length <= secremain)secremain = data_length;//不大于4096个字节
	while(1)
	{
		data_flash_read_block(secpos*4096, 4096, FLASH_BUF);//读出整个扇区的内容
		for(i=0; i<secremain; i++)//校验数据
		{
			if(FLASH_BUF[secoff+i]!=0XFF)break;//需要擦除
		}
		if(i < secremain)//需要擦除
		{
			return_code = data_flash_erase_block(secpos*4096, DF_BLOCK_4KB);//擦除这个扇区
			for(i=0; i<secremain; i++)	   //复制
			{
				FLASH_BUF[i+secoff]=data_ptr[i];
			}
			return_code = data_flash_write_block(FLASH_BUF, secpos*4096, 4096);//写入整个扇区

		}
		else 
		{
			return_code = data_flash_write_block(data_ptr, address, secremain);//写已经擦除了的,直接写入扇区剩余区间.
		}
		if(data_length==secremain)break;//写入结束了
		else//写入未结束
		{
			secpos++;//扇区地址增1
			secoff=0;//偏移位置为0

			data_ptr+=secremain;  //指针偏移
			address+=secremain;//写地址偏移
			data_length-=secremain;				//字节数递减
			if(data_length>4096)secremain=4096;	//下一个扇区还是写不完
			else secremain=data_length;			//下一个扇区可以写完了
		}
	}
	
	return return_code;
	
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_write_block
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is provide as external interface to write data flash with
* specified source data, destination address and data length.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* This capsulation of data flash write operation is to deal with page wrap.
* The byte program will wrap over, once the 256 bytes page boundary reached.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 25-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
df_status_t data_flash_write_block(U8 *data_ptr, U32 address, U16 data_length)
{
	df_status_t return_code = DF_WRITE_COMPLETED;
	U32 write_addr = address;
	U16 bytes_remained = data_length;

	while (bytes_remained >= 1 && return_code == DF_WRITE_COMPLETED)
	{
		if ((write_addr & 0xFF) == 0)
		{
			/* address 256 bytes aligned */

			if (bytes_remained <= DF_PAGE_SIZE)
			{
				/* bytes remained less than one page */
				return_code = data_flash_write_page(data_ptr, write_addr, bytes_remained);
				bytes_remained = 0;	/* end while loop */
			}
			else /* (bytes_remained > DF_PAGE_SIZE) */
			{
				/* bytes remained more than one page */
				return_code = data_flash_write_page(data_ptr, write_addr, DF_PAGE_SIZE);
				bytes_remained -= DF_PAGE_SIZE;
				data_ptr += DF_PAGE_SIZE;
				write_addr += DF_PAGE_SIZE;
			}
		}
		else
		{
			/*(write_addr & 0xFF != 0), address not 256 bytes aligned */

			if (BEYOND_PAGE_BOUNDARY(write_addr, bytes_remained) == TRUE)
			{
				/* bytes remained exceed current page, only write data within current page */
				return_code = data_flash_write_page(data_ptr, write_addr, BYTES_TO_NEXT_PAGE_BOUNDARY(write_addr));
				bytes_remained -= BYTES_TO_NEXT_PAGE_BOUNDARY(write_addr);
				data_ptr += BYTES_TO_NEXT_PAGE_BOUNDARY(write_addr);
				write_addr += BYTES_TO_NEXT_PAGE_BOUNDARY(write_addr); /* now became 256 bytes aligned */
			}
			else
			{
				/* bytes remained not exceed current page */
				return_code = data_flash_write_page(data_ptr, write_addr, bytes_remained);
				bytes_remained = 0; /* end while loop */
			}
		}
	}	/* end of while */

	return return_code;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_write_page
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by data_flash_write_block() to program bytes within
* one 256 bytes page.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Caller of this function must guarantee data not exceed page boundary.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 25-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
df_status_t data_flash_write_page(U8 *data_ptr, U32 address, U16 length)
{
	U16 status = 1;
	df_status_t return_code = DF_WRITE_FAIL;
	U16 count = 0; /* to monitor write time consumption */

	/* check input parameter */
	if (data_ptr == NULL || address >= DF_MAX_ADDR || length > DF_PAGE_SIZE)
	{
		return DF_INVALID_PARAM;
	}

	while((status = send_flash_command(READ_STATUS_REG, 0, 0, 0)) == STATUS_BUSY);
	//if ((status & STATUS_BUSY) != 0)
	//{
		//return DF_DEVICE_BUSY;
	//}

	/* send WRITE_ENABLE command */
	send_flash_command(WRITE_ENABLE, 0, NULL, 0);

	/* send PAGE_PROGRAM command */
	send_flash_command(PAGE_PROGRAM, address, data_ptr, length);
	do {
		status = send_flash_command(READ_STATUS_REG, 0, NULL, 0);
		count++;
	} while((status & STATUS_BUSY) != 0);

	/*  Check Status Register */
	if ((status & STATUS_ERASE_PROG_ERROR) != 0)
	{
		return_code = DF_WRITE_FAIL;
	}
#if 0//huayi
	else if (((status & STATUS_WRITE_NOT_ENABLED) != 0) ||
			 ((status & STATUS_SECTOR_PROTECTED)  != 0) ||
			 ((status & STATUS_WRITE_PROTECTED)   != 0))
	{
		return_code = DF_WRITE_DISABLED;
	}
#endif
	else
	{
		/*  Erase Successful.  */
		return_code = DF_WRITE_COMPLETED;
	}

	/* send WRITE_DISABLE command */
	send_flash_command(WRITE_DISABLE, 0, NULL, 0);

	return return_code;
}

/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2009 MOTOROLA, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: data_flash_read_block
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is provide as external interface to read data flash with
* specified address, data length and buffer pointer.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* To keep read operation more controllable, a 4KB limitation is applied on input
* parameter length.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 25-Feb-10   a21961    none        Initial draft
*
*******************************************************************************/
df_status_t data_flash_read_block(U32 address, U16 length, U8 *data_ptr)
{
	/* check input parameter */
	if (data_ptr == NULL || address >= DF_MAX_ADDR || length > 0x1000)
	{
		return DF_INVALID_PARAM;
	}

	send_flash_command(READ_ARRAY, address, data_ptr, length);

	return DF_OK;
}
