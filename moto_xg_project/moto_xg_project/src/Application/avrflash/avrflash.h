/*
 * avrflash.h
 *
 * Created: 2018/12/3 16:40:52
 *  Author: Edwards
 */ 


#ifndef AVRFLASH_H_
#define AVRFLASH_H_
#include "flashc.h"

#define MAX_PAYLOAD_LEN 150
#define MAX_PROGRAM_DATA_BYTE_LEN 128


#pragma  pack(1)

typedef enum
{
	ENTER_BOOT_REQ_OPCODE =0x01,
	ENTER_BOOT_RLY_OPCODE =0x02,
	READ_INFO_REQ_OPCODE = 0X03 ,
	READ_INFO_RLY_OPCODE =0x04,
	WRITE_ID_REQ_OPCODE= 0x05,
	WRITE_ID_RLY_OPCODE =0x06,
	PROGRAM_FLASH_REQ_OPCODE=0x07,
	PROGRAM_FLASH_RLY_OPCODE =0x08,
	EXIT_BOOT_REQ_OPCODE=0x09,
	EXIT_BOOT_RLY_OPCODE =0x0A
	
}df_opcode_t;

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


typedef union {
	df_enter_boot_reply_t	df_enter_boot_reply;
	df_read_info_reply_t	df_read_info_reply;
	df_write_id_request_t	df_write_id_request;
	df_write_id_reply_t		df_write_id_reply;
	df_program_request_t	df_program_request;
	df_program_reply_t		df_program_reply;
	df_exit_boot_reply_t	df_exit_boot_reply;
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
void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes);
void avr_flash_test(void);
void parse_flash_protocol(flash_proto_t * );


#endif /* AVRFLASH_H_ */