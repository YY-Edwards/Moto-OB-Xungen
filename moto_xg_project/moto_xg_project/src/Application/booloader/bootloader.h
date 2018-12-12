/*
 * bootloader.h
 *
 * Created: 2018/12/3 16:40:52
 *  Author: Edwards
 */ 


#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
#include "flashc.h"

#define ERASE_FLASH_TIMER 2
#define MAX_PAYLOAD_LEN 150
#define MAX_PROGRAM_DATA_BYTE_LEN 128

#define FLASH_PAGE_SIZE					512 //bytes
#define BOOT_FLASH_BEGIN 				(0x80000000)
//#define BOOT_FLASH_END 				(0x80080000)

#define USERPAGE_START_ADD				(0x80800000)//user page
#define USER_PAGE_SIZE					(0x00000200)//user page size:512bytes

#define BOOT_INFO_START_ADD    			(USERPAGE_START_ADD+0x00000004)//0x80800004
#define BOOT_ID_OFFSET					(0x00000004)
#define BOOT_ID_SIZE					(0x00000004)
#define BOOT_INFO_SIZE    				(0x00000010)//16bytes

#define FIRMWARE_INFO_START_ADD    		(BOOT_INFO_START_ADD+BOOT_INFO_SIZE)//0x80800014

#define FIRMWARE_VALID_FLAG_OFFSET		(0x00000001)
#define FIRMWARE_ID_OFFSET				(0x0000000B)
#define FIRMWARE_ID_SIZE				(0x00000004)
#define FIRMWARE_VALID_FLAG_START_ADD	(FIRMWARE_INFO_START_ADD + FIRMWARE_VALID_FLAG_OFFSET)//0x80800015

#define FIRMWARE_INFO_SIZE    			(0x00000010)//16bytes

#define BOOT_UNINIT 					(0xffffffff)
#define BOOT_LOADER_BEGIN				(0x80000000)   
#define BOOT_LOADER_SIZE				(0x00010000)   //64K ?
#define MAX_FIRMWARE_BYTE_ZISE			(0x00020000)   //128K ?

#define MIN_BOOT_3_PARTY  				(0x80010000)   //3 party


#pragma  pack(1)

typedef struct
{
	uint8_t version[4];
	uint8_t id[4];
	uint8_t reserved[8];
	
}df_boot_info_t;


typedef enum
{
	MOTO_PATROL =0x01,
	MOTO_RECORD =0x02,
	MOTO_CSBK = 0X03 
	
}df_firmware_type_t;

typedef struct
{
	uint8_t type;//MOTO_PATROL
	uint8_t isValid;//0:invalid;1:valid
	uint8_t reserved[2];
	uint8_t addr[4];//BOOT_3_PARTY
	uint8_t version[4];//firmware_version
	uint8_t id[4];
	
} df_firmware_info_t;//16bytes,



typedef enum
{
	REPLYMASk = 0x80,
	ENTER_BOOT_REQ_OPCODE =0x01,
	ENTER_BOOT_RLY_OPCODE =REPLYMASk | ENTER_BOOT_REQ_OPCODE,
	READ_INFO_REQ_OPCODE = 0X02 ,
	READ_INFO_RLY_OPCODE =REPLYMASk | READ_INFO_REQ_OPCODE,
	WRITE_ID_REQ_OPCODE= 0x03,
	WRITE_ID_RLY_OPCODE =REPLYMASk | WRITE_ID_REQ_OPCODE,
	PROGRAM_FLASH_REQ_OPCODE=0x04,
	PROGRAM_FLASH_RLY_OPCODE =REPLYMASk | PROGRAM_FLASH_REQ_OPCODE,
	EXIT_BOOT_REQ_OPCODE=0x05,
	EXIT_BOOT_RLY_OPCODE =REPLYMASk | EXIT_BOOT_REQ_OPCODE,
	ERASE_REQ_OPCODE=0x06,
	ERASE_RLY_OPCODE =REPLYMASk | ERASE_REQ_OPCODE,
	CHECK_MEMORY_REQ_OPCODE=0x07,
	CHECK_MEMORY_RLY_OPCODE =REPLYMASk | CHECK_MEMORY_REQ_OPCODE
	
	
}df_opcode_t;

typedef enum
{
	DF_SUCCESS =0x00,
	DF_FAILURE = 0x01, 
	DF_WAIT_TO_RESET =0x02,
	DF_ERASING = 0x03
	
}df_operation_status_t;

typedef enum
{
	APP_TYPE_BOOTLOADER =0x00,
	APP_TYPE_FIRMWARE =0x01,
	
}df_app_type_t;


typedef struct 
{
	uint8_t result;
} df_enter_boot_reply_t;

typedef struct
{
	uint8_t result;
	uint8_t info[32];
} df_read_info_reply_t;


typedef struct
{
	uint8_t condition;
	uint8_t id[4];
} df_write_id_request_t;

typedef struct
{
	uint8_t result;
} df_write_id_reply_t;


typedef struct
{
	uint32_t addr;
	uint8_t data[MAX_PROGRAM_DATA_BYTE_LEN];
} df_program_request_t;

typedef struct
{
	uint8_t result;
	uint32_t addr;
} df_program_reply_t;

typedef struct
{
	uint8_t result;
} df_exit_boot_reply_t;

typedef struct
{
	uint8_t result;
} df_erase_reply_t;


typedef struct
{
	uint32_t fileSize;//bytes
	uint32_t programStartAddr;
} df_check_flash_memory_request_t;

typedef struct
{
	uint8_t result;
} df_check_flash_memory_reply_t;


typedef union {
	df_enter_boot_reply_t	df_enter_boot_reply;
	df_read_info_reply_t	df_read_info_reply;
	df_write_id_request_t	df_write_id_request;
	df_write_id_reply_t		df_write_id_reply;
	df_program_request_t	df_program_request;
	df_program_reply_t		df_program_reply;
	df_exit_boot_reply_t	df_exit_boot_reply;
	df_erase_reply_t		df_erase_reply;
	df_check_flash_memory_request_t		df_check_flash_memory_request;
	df_check_flash_memory_reply_t		df_check_flash_memory_reply;
	uint8_t					u8[MAX_PAYLOAD_LEN];
} flash_proto_payload_t;

typedef struct
{
	uint8_t opcode;
	uint8_t checkSum;
	uint8_t payload_len;
	flash_proto_payload_t proto_payload;
	
} flash_proto_t;


#pragma  pack()

//will jump to 3-party if needed
//int flash_read_app_start();
bool avrflash_check_3_party_is_valid(void);
//void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes);
void avr_flash_test(void);
void bootloader_init(void);
void parse_flash_protocol(flash_proto_t * , U8 rx_sessionID);


#endif /* BOOTLOADER_H_ */