/*
 * xgflash.h
 *
 * Created: 2017/11/8 16:49:25
 *  Author: Edwards
 */ 


#ifndef XGFLASH_H_
#define XGFLASH_H_

#include "compiler.h"
#include "xgrtc.h"

/**

label:			"XUNGENG"(8bytes)

voice_numbers:	0x xxxx(2bytes)

index_number(2bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + address(4bytes) + length(2bytes);
...
...
index_number(2bytes) + address(4bytes) + length(2bytes);


**/


//#define PROGRAM_START_ADDRESS         (AVR32_FLASH_ADDRESS + PROGRAM_START_OFFSET)
//#define PROGRAM_START_OFFSET          0x00014020

#define FLASH_PAGE_SIZE 80 //Kbyte

#define XG_MESSAGE_LISTINFO_START_ADD		(AVR32_FLASH_ADDRESS + 0x00000020)
#define XG_MESSAGE_LISTINFO_BOUNDARY_ADD	0x8000501A

#define LABEL_ADDRESS						XG_MESSAGE_LISTINFO_START_ADD
#define LABEL_LENGTH						0x08//8bytes:"XUNGENG"

#define MESSAGE_NUMBERS_ADD					(XG_MESSAGE_LISTINFO_START_ADD + LABEL_LENGTH)
#define MESSAGE_NUMBERS_LENGTH				0x02//2bytes:0x xxxx  

#define XG_MESSAGE_INFO_HEADER_START_ADD	(XG_MESSAGE_LISTINFO_START_ADD + LABEL_LENGTH + MESSAGE_NUMBERS_LENGTH)
//#define VOICE_INFO_LENGTH	0x08//8bytes:list_number(2bytes) + address(4bytes) + length(2bytes)
#define XG_MESSAGE_INFO_HEADER_LENGTH		0x08

#define XG_MESSAGE_DATA_START_ADD			0x80005020
#define XG_MESSAGE_DATA_BOUNDARY_ADD		0x8001401A


#pragma pack(1)
typedef struct
{
	unsigned short	numb;
	unsigned int	address;
	unsigned short	offset;

}MessageList_Info_t;//8bytes
#pragma pack()

#pragma pack(1)
typedef struct
{
	unsigned char	RFID_ID[4];
	DateTime_t		XG_Time;

}MessageData_t;//10bytes
#pragma pack()



//! Structure type containing variables to store in NVRAM using a specific
//! memory map.
typedef const struct {
	uint8_t  var8;
	uint16_t var16;
	uint8_t  var8_3[3];
	uint32_t var32;
} nvram_data_t;


void flash_rw_example(const char *caption, nvram_data_t *nvram_data);
void xg_flashc_init(void);

#endif /* XGFLASH_H_ */