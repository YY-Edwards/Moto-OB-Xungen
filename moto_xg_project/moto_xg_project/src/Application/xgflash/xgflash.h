/*
 * xgflash.h
 *
 * Created: 2017/11/8 16:49:25
 *  Author: Edwards
 */ 


#ifndef XGFLASH_H_
#define XGFLASH_H_
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "compiler.h"
#include "data_flash.h"
//#include "flashc.h"
#include "string.h"
#include "log.h"
#include "RFID.h"

/**

label:			"PATROL"(6bytes)

voice_numbers:	0x xxxx(2bytes)

index_number(2bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + address(4bytes) + length(2bytes);
...
...
index_number(2bytes) + address(4bytes) + length(2bytes);


**/


#define TRUE 1
#define FALSE 0

#define PageSize 256

//#define FLASH_PAGE_SIZE 80 //Kbyte

#define XG_MESSAGE_LISTINFO_START_ADD		0x000000
#define XG_MESSAGE_LISTINFO_BOUNDARY_ADD	0x020000//128k=2*64k

#define LABEL_ADDRESS						XG_MESSAGE_LISTINFO_START_ADD
#define LABEL_LENGTH						0x06//6bytes:"PATROL"

#define MESSAGE_NUMBERS_ADD					(XG_MESSAGE_LISTINFO_START_ADD + LABEL_LENGTH)
#define MESSAGE_NUMBERS_LENGTH				0x02//2bytes:0x xxxx  

#define XG_MESSAGE_INFO_HEADER_START_ADD	(XG_MESSAGE_LISTINFO_START_ADD + LABEL_LENGTH + MESSAGE_NUMBERS_LENGTH)
//#define VOICE_INFO_LENGTH	0x08//8bytes:list_number(2bytes) + address(4bytes) + length(2bytes)
#define XG_MESSAGE_INFO_HEADER_LENGTH		0x08

#define XG_MESSAGE_DATA_START_ADD			XG_MESSAGE_LISTINFO_BOUNDARY_ADD + 10//0x02000A
#define XG_MESSAGE_DATA_BOUNDARY_ADD		DF_MAX_ADDR//0x7FFFFF    /* 8MB */

#define MAX_MESSAGE_STORE 120

//volatile Message_Protocol_t message_store[MAX_MESSAGE_STORE];

extern volatile xQueueHandle message_storage_queue ;
extern void * get_idle_store(xQueueHandle store);
extern void set_idle_store(xQueueHandle store, void * ptr);


#define get_message_store()			get_idle_store(message_storage_queue)
#define set_message_store(ptr)		set_idle_store(message_storage_queue, ptr)

#pragma pack(1)
typedef struct
{
	unsigned short	numb;
	unsigned int	address;
	unsigned short	offset;

}MessageList_Info_t;//8bytes
#pragma pack()


//! Structure type containing variables to store in NVRAM using a specific
//! memory map.
typedef const struct {
	uint8_t  var8;
	uint16_t var16;
	uint8_t  var8_3[3];
	uint32_t var32;
} nvram_data_t;


typedef enum
{
	XG_ERROR = -1,
	XG_OK = 0,
	XG_INVALID_PARAM = 1,
	XG_FLASH_FULL = 2,
	XG_OUT_BOUNDARY = 3,
	XG_ERASE_FAIL = 4,
	XG_ERASE_COMPLETED = 5,
	XG_FLASH_READ_FAIL = 6,
	XG_FLASH_WRITE_FAIL = 7,
	
} xgflash_status_t;

void create_xg_flash_test_task(void);
void flash_rw_example(const char *caption, nvram_data_t *nvram_data);
void xg_flashc_init(void);

static xgflash_status_t xgflash_list_info_init(void);
U16 xgflash_get_message_count(void);
xgflash_status_t xgflash_message_save(U8 *data_ptr, U16 data_len, U8 data_end_flag);
xgflash_status_t xgflash_get_message_data(U32 message_index, void *buff_ptr, bool erase);//read+erase
xgflash_status_t xgflash_erase_info_region(void);


#endif /* XGFLASH_H_ */