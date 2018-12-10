/*
 * avrflash.c
 *
 * Created: 2018/12/3 16:41:10
 *  Author: Edwards
 */ 
#include "avrflash.h"
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "log.h"
#include "xcmp.h"




#pragma pack(1)
//! Structure type containing variables to store in NVRAM using a specific
//! memory map.
typedef const struct {
	uint8_t  var8;
	uint16_t var16;
	uint8_t  var8_3[3];
	uint32_t var32;
} nvram_data_t;
#pragma pack()


//! NVRAM data structure located in the flash array.
#if defined (__GNUC__)
__attribute__((__section__(".flash_nvram")))
#endif
static nvram_data_t flash_nvram_data
#if defined (__ICCAVR32__)
@ "FLASH_NVRAM"
#endif
;

//! NVRAM data structure located in the User page.
#if defined (__GNUC__)
__attribute__((__section__(".userpage")))
#elif defined(__ICCAVR32__)
__no_init
// This code is added so that this example can be programmed through
// batchisp and a bootloader. Else, IAR will set this variable as
// loadable and batchisp will err because this variable is out of the
// flash memory range (it's in the user page).
// GCC will init this variable at run time not during the programming
// of the application.
#endif
static nvram_data_t user_nvram_data
#if defined (__ICCAVR32__)
@ "USERDATA32_C"
#endif
;



volatile bool is_writing = false;

void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes)
{
	Disable_interrupt_level(1);
	vTaskSuspendAll();
	
	is_writing = true;
	//flashc_memcpy(dst, src, nbytes, TRUE);
	avr_flash_test();
	is_writing = false;
	
	xTaskResumeAll();
	Enable_interrupt_level(1);
	
}



static void flash_rw_example(const char *caption, nvram_data_t *nvram_data)
{
	
	 uint8_t write_data[128];
	for(int i = 0; i < 128; ++i)write_data[i] = i;
	
	uint16_t temp = 0xAB37;
	
	//static char str[80];
	//memset(str, 0x00, sizeof(str));
	
	//print_dbg("\r\nClearing NVRAM variables...");
	//flashc_memset((void *)nvram_data, 0x3B5A2B4A, 32, 256, true);
	
	
	//print_dbg("\r\nNVRAM variables cleared:\r\n");
	//print_nvram_variables(nvram_data);

	//print_dbg("\r\nWriting new values to NVRAM variables...");
	flashc_memcpy((void *)0x80800000,   write_data, 128,   true);
	//flashc_memcpy((void *)&nvram_data->var16,  &write_data, sizeof(nvram_data->var16),  true);
	//flashc_memcpy((void *)&nvram_data->var8_3, &write_data, sizeof(nvram_data->var8_3), true);
	//flashc_memcpy((void *)&nvram_data->var32,  &write_data, sizeof(nvram_data->var32),  true);
	//print_dbg("\r\nNVRAM variables written:\r\n");
	//print_nvram_variables(nvram_data);
	//flashc_memcpy((void *)str,  (void *)&nvram_data->var8, sizeof(nvram_data->var8),  false);
	
		
}

void avr_flash_test()
{
	flash_rw_example("user_page", &user_nvram_data);		
}


uint8_t df_payload_checksunm(void *p, uint8_t len)
{
	unsigned char  sumScratch = 0;

	for(unsigned short i =0; i<len; i++)
	{
		sumScratch += *(unsigned char*)p++;
	}
	  
	 return sumScratch;

}

void parse_flash_protocol(flash_proto_t *p)
{
	uint8_t opcode = p->opcode;
	flash_proto_t tx_buf;

	switch(opcode)
	{
		case ENTER_BOOT_REQ_OPCODE:
		
				log_debug("rx ENTER_BOOT_REQ_OPCODE.");
				tx_buf.opcode = ENTER_BOOT_RLY_OPCODE;
				tx_buf.payload_len =1;
				tx_buf.proto_payload.df_enter_boot_reply.result = 0;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(ENTER_BOOT_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len);
				
			break;
			
		case READ_INFO_REQ_OPCODE:
				log_debug("rx READ_INFO_REQ_OPCODE.");
				tx_buf.opcode = READ_INFO_RLY_OPCODE;
				tx_buf.payload_len =33;
				tx_buf.proto_payload.df_read_info_reply.result = 0;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(READ_INFO_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len);
				
			break;
			
		case WRITE_ID_REQ_OPCODE:
				log_debug("rx WRITE_ID_REQ_OPCODE.");
				tx_buf.opcode = WRITE_ID_RLY_OPCODE;
				tx_buf.payload_len =5;
				tx_buf.proto_payload.df_write_id_reply.result= 0;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(WRITE_ID_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len);
				
			break;
			
		case PROGRAM_FLASH_REQ_OPCODE:
		
				log_debug("rx PROGRAM_FLASH_REQ_OPCODE.");
				tx_buf.opcode = PROGRAM_FLASH_RLY_OPCODE;
				tx_buf.payload_len =5;
				tx_buf.proto_payload.df_program_reply.result = 0;
				tx_buf.proto_payload.df_program_reply.addr = 0xABCD5A5A;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(PROGRAM_FLASH_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len);
				
			break;
			
		case EXIT_BOOT_REQ_OPCODE:
		
				log_debug("rx EXIT_BOOT_REQ_OPCODE.");
				tx_buf.opcode = EXIT_BOOT_RLY_OPCODE;
				tx_buf.payload_len =1;
				tx_buf.proto_payload.df_exit_boot_reply.result = 0;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(EXIT_BOOT_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len);
				
			break;
			
		default:
			log_debug("flash opcode err:[0x%x]", opcode);
			break;
		
	}
	
	
}