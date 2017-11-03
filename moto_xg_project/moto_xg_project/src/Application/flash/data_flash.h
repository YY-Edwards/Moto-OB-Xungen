/*******************************************************************************
*
*                         C  H E A D E R  F I L E
*
*            COPYRIGHT 2008 MOTOROLA, INC. ALL RIGHTS RESERVED.
*                    MOTOROLA CONFIDENTIAL PROPRIETARY
*
********************************************************************************
*
* FILE NAME : data_flash.h
*
*-------------------------------- PURPOSE --------------------------------------
*
* Public interface for data flash driver.
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
#ifndef DATA_FLASH_H_
#define DATA_FLASH_H_

#include "spi.h"
#include "gpio.h"
//#include "pm.h"
#include "FreeRTOS.h"
#include "task.h"
//#include "EVK1101/evk1101.h"
#include "xcmp.h"
#include "../Log/log.h"
#include "string.h"


//#include "utility.h"

#define TRUE 1
#define FALSE 0

#define FATAL_ERROR_DMA_TX_OVERFLOW     0x01
#define FATAL_ERROR_DATA_FLASH_SPI_INIT 0x02
#define FATAL_ERROR_DATA_FLASH_READ_ID  0x03
#define FATAL_ERROR_ACC_INIT            0x04

#define DF_SPI_FIRST_NPCS 0
#define DF_SPI_MASTER_SPEED 24000000
#define DF_SPI_BITS 8
#define DF_SPI_PCS_0 0

#define DF_TEST_SUCCESS 0
#define DF_TEST_FAIL 1

#define     READ_ARRAY						0x03
#define     BLOCK_ERASE_4KB					0x20
#define     BLOCK_ERASE_32KB				0x52
#define     BLOCK_ERASE_64KB				0xD8
#define     CHIP_ERASE						0x60
#define     BYTE_PROGRAM					0x02
#define     PAGE_PROGRAM					0x02
#define     WRITE_ENABLE                    0x06
#define     WRITE_DISABLE                   0x04
#define     PROGRAM_ERASE_SUSPEND           0xB0
#define     PROGRAM_ERASE_RESUME			0xD0
#define     READ_STATUS_REG					0x05
#define     WRITE_STATUS_REG_BYTE_1         0x01
#define     WRITE_STATUS_REG_BYTE_2         0x31
#define     READ_M_D_ID                     0x9F

#define     STATUS_BUSY                     0x01
#define     STATUS_WRITE_NOT_ENABLED        0x02
#define     STATUS_SECTOR_PROTECTED         0x04
#define     STATUS_WRITE_PROTECTED          0x10
#define     STATUS_ERASE_PROG_ERROR         0x20

#define     UNPROTECT_ALL_SECTORS           0x00

#define     DF_MAX_ADDR                     0x7FFFFF    /* 8MB */
#define     DF_PAGE_SIZE                    0x100       /* 256B */
#define     DF_DATA_SPACE_SIZE              0x200       /* 512 */

typedef enum
{
  DF_ERROR = -1,
  DF_OK = 0,
  DF_INVALID_PARAM = 1,
  DF_DEVICE_BUSY = 2,
  DF_WRITE_DISABLED = 3,
  DF_ERASE_FAIL = 4,
  DF_ERASE_COMPLETED = 5,
  DF_WRITE_FAIL = 6,
  DF_WRITE_COMPLETED = 7
} df_status_t;

typedef enum
{
	DF_BLOCK_4KB = 1,
	DF_BLOCK_32KB = 2,
	DF_BLOCK_64KB = 3,
	DF_BLOCK_ALL = 4,
} df_block_size_t;

#define spi_write_dummy()                   spi_write(spi, 0xFF)
#define spi_write_byte(x)                   spi_write(spi, (U16)x)
#define spi_read_byte(x)                    spi_read(spi, (U16*)x)


//#define BEYOND_PAGE_BOUNDARY(addr, size)    ((addr + size) > (addr | 0x0000FF) ? TRUE : FALSE)
#define BEYOND_PAGE_BOUNDARY(addr, size)    ((addr + size) > ((addr & 0x7FFF00)+DF_PAGE_SIZE) ? TRUE : FALSE)//huayi
//#define BYTES_TO_NEXT_PAGE_BOUNDARY(addr)   (DF_PAGE_SIZE - (addr & 0x7FFF00))
#define BYTES_TO_NEXT_PAGE_BOUNDARY(addr)   (DF_PAGE_SIZE - (addr & 0xFF))//huayi

void data_flash_init(void);
U16 send_flash_command(U16 command, U32 address, U8 *data_ptr, U16 length);
df_status_t data_flash_erase_block(U32 address, df_block_size_t block_size);
df_status_t data_flash_write(U8 *data_ptr, U32 address, U16 data_length);
df_status_t data_flash_write_block(U8 *data_ptr, U32 address, U16 data_length);
df_status_t data_flash_write_page(U8 *data_ptr, U32 address, U16 length);
df_status_t data_flash_read_block(U32 address, U16 length, U8 *data_ptr);
uint8_t serial_flash_test();

#endif

