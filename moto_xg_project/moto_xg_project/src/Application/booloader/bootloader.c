/*
 * bootloader.c
 *
 * Created: 2018/12/3 16:41:10
 *  Author: Edwards
 */ 
#include "bootloader.h"
#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "log.h"
#include "xcmp.h"
#include "timer.h"
#include "gpio.h"


extern void main();
extern unsigned int flash_size;
volatile bool g_is_inBOOT = false;
volatile bool g_isEraseFlash = false;
volatile bool g_isEraseFlashFinished_ = false;
volatile U8 current_app_type = APP_TYPE_3_PARTY;//默认是3_PARTY
volatile U32 third_partyStartAddr_ = MIN_BOOT_3_PARTY_BEGIN;//默认最小的固件起始地址
volatile U32 third_partyByteSize_ = MAX_3_PARTY_BYTE_SIZE;//默认为最大的固件大小
const U8 third_party_version[4]={0x00, 0x02, 0x00, 0x01};
const U8 boot_version[4]={0x00, 0x01, 0x00, 0x00};
void (*start_program) (void) = (void (*)(void))(BOOT_LOADER_BEGIN);//默认是bootloader	
//The 4-byte OB Firmware Version number uses a reserved byte, a Major Number to track the major changes,
// Minor Number to track minor changes and Product ID Number to differentiate the product line.
/*Product ID Number:
				0x01   Patrol   
				0x02   Record
				0x03	CSBK	
				...		...

*/


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

static void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes , bool erase)
{
	Disable_interrupt_level(1);
	vTaskSuspendAll();
	
	is_writing = true;
	flashc_memcpy(dst, src, nbytes, erase);
	//avr_flash_test();
	is_writing = false;
	
	xTaskResumeAll();
	Enable_interrupt_level(1);
	
}


/*---------------------------------- PURPOSE ------------------------------------
*  Jump to the GOB application start address
*  The start address points to the application on the Generic Option Board
*  that launches upon initialization.
*
*  If runaddr = BOOT_LOADER_BEGIN the boot loader will start.
*  If runaddr >= MIN_BOOT_3_PARTY_BEGIN the 3rd party application will start.
*---------------------------------- SYNOPSIS -----------------------------------
*--------------------------- DETAILED DESCRIPTION ------------------------------*/
#define DETECT_TIME (3)
int flash_read_app_start()
{
	
	volatile unsigned long runaddr;

	volatile unsigned long mainaddr = (unsigned long)main;
	int ret = 0;
	df_third_party_info_t third_party_info;
	memset(&third_party_info, 0x00, sizeof(df_third_party_info_t));
	memcpy((void*)&third_party_info, (void*)THIRD_PARTY_INFO_START_ADD, THIRD_PARTY_INFO_SIZE);	
	runaddr = *(uint32_t *)(third_party_info.addr);//读出APP的地址，并校验是否为预设的起始地址
	start_program = (void *)runaddr;

	if ((runaddr == MIN_BOOT_3_PARTY_BEGIN) &&(mainaddr < MIN_BOOT_3_PARTY_BEGIN))//prevent calling boot loader main again
	{
		
		volatile int forceboot = 0;
		gpio_enable_gpio_pin(AVR32_PIN_PA04);
		while (gpio_get_pin_value(AVR32_PIN_PA04) == 0) //detect the PA04 pin for a while (30ms), if set to low longer than 30ms, stay in boot loader
		{
			wait_10_ms();
			forceboot++;
			if (forceboot >= DETECT_TIME)
				break;
		}

		if ((forceboot < DETECT_TIME) && (start_program != 0xFFFFFFFF)) //only AVR32_PIN_PA04 is high, add by a21617
		{
			ret = 0;
			(*start_program)();
		}
		else
		{
			ret = -1;
		}
	}

	/* when reach here, record current application type */
	if (mainaddr < MIN_BOOT_3_PARTY_BEGIN)
	{
		current_app_type = APP_TYPE_BOOTLOADER;
	}
	else
	{
		current_app_type = APP_TYPE_3_PARTY;
	}

	return ret;
}

bool avrflash_read_info(void *p)
{
	//read  boot_info and app_info from BOOT_INFO_START_ADD
	memcpy((void*)p, (void*)BOOT_INFO_START_ADD, (BOOT_INFO_SIZE+THIRD_PARTY_INFO_SIZE));
	return true;
	
}

bool avrflash_write_id(U8 condition,  void*p)
{
	//write  boot_id or app_id 
	if(condition == APP_TYPE_BOOTLOADER)
	{
		write_flash_in_multitask((BOOT_INFO_START_ADD + BOOT_ID_OFFSET), p, BOOT_ID_SIZE, true);
		//flashc_memcpy((BOOT_INFO_START_ADD + BOOT_ID_OFFSET), p, BOOT_ID_SIZE, true);

	}
	else if(condition == APP_TYPE_3_PARTY)
	{
		write_flash_in_multitask((THIRD_PARTY_INFO_START_ADD + THIRD_PARTY_ID_OFFSET), p, THIRD_PARTY_ID_SIZE, true);
		//flashc_memcpy((THIRD_PARTY_INFO_START_ADD + THIRD_PARTY_ID_OFFSET), p, THIRD_PARTY_ID_SIZE, true);
	}
	return true;
	
}

bool avrflash_erase_in_multitask_func()
{

	g_isEraseFlash = true;
	log_debug("start erase flash.");
	bool ret =false;
	U16 q = 0;
	U16 r = 0;
	if (current_app_type == APP_TYPE_BOOTLOADER)	
	{
		if(third_partyStartAddr_ >= (BOOT_LOADER_BEGIN + BOOT_LOADER_MAX_SIZE))
		{
			U16 erase_page_count = third_partyByteSize_/FLASH_PAGE_SIZE;
			erase_page_count += third_partyByteSize_%FLASH_PAGE_SIZE;
			q = ((third_partyStartAddr_ - BOOT_LOADER_BEGIN)/FLASH_PAGE_SIZE);
			r = ((third_partyStartAddr_ - BOOT_LOADER_BEGIN)%FLASH_PAGE_SIZE);
			U16 third_party_start_page_index = (q + r);
			U16 erase_index = third_party_start_page_index;
			do 
			{
				Disable_interrupt_level(1);
				vTaskSuspendAll();
				
				is_writing = true;
				//ret = flashc_erase_page(erase_index, true);//erase third_party
				
				is_writing = false;
				
				xTaskResumeAll();
				Enable_interrupt_level(1);
				erase_index++;
				erase_page_count--;
			} while ((ret==true) && (erase_page_count > 0));
			
		}
		else
		 ret = false;
	}
	else /* current_app == APP_TYPE_3_PARTY */
	{
		return false;
	}
	
	g_isEraseFlash = false;
	g_isEraseFlashFinished_ = true;
	return ret;
	
}


/*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Bootloader can only update 3rd party app space (0x80010000~0x80030000).
* FTA can only update bootloader app space (0x800000000~0x8000ffff).
*
*******************************************************************************/
bool avrflash_validate_flashing_addr(uint32_t address, uint32_t len)
{
	if (current_app_type == APP_TYPE_BOOTLOADER)
	{
		/* boot loader only has the ability to update 3rd party app */
		if (address < (BOOT_LOADER_BEGIN +BOOT_LOADER_MAX_SIZE) || (address + len) > (BOOT_FLASH_BEGIN + flash_size))
			return false;
		else
		{
			return true;
		}
	}
	else /* current_app == APP_TYPE_3_PARTY */
	{
		/* FTA only has the ability to update GOB boot loader app */
		if (address < BOOT_FLASH_BEGIN || address >= (BOOT_LOADER_BEGIN +BOOT_LOADER_MAX_SIZE))
			return false;
		else
			return true;
	}
}


bool avrflash_check_3_party_is_valid(void)
{
	df_third_party_info_t third_party_info;
	memset(&third_party_info, 0x00, sizeof(df_third_party_info_t));
		
	//read third_party info
	memcpy((void*)&third_party_info, (void*)THIRD_PARTY_INFO_START_ADD, THIRD_PARTY_INFO_SIZE);
	if(third_party_info.isValid != 1)return false;
	else
		return true;
	
}

bool avrflash_update_third_party_info(void)
{
	
	df_third_party_info_t third_party_info;
	memset(&third_party_info, 0x00, sizeof(df_third_party_info_t));
	//read third_party info
	memcpy((void*)&third_party_info, (void*)THIRD_PARTY_INFO_START_ADD, THIRD_PARTY_INFO_SIZE);

	
	//update flag and addr
	third_party_info.isValid = 1;
	memcpy(third_party_info.addr, &third_partyStartAddr_, sizeof(third_partyStartAddr_));
		
	//write third_party info
	write_flash_in_multitask(THIRD_PARTY_INFO_START_ADD, &third_party_info, THIRD_PARTY_INFO_SIZE, true);
	//flashc_memcpy(THIRD_PARTY_INFO_START_ADD, &third_party_info, THIRD_PARTY_INFO_SIZE, true);
	
}
static void flash_rw_example(const char *caption, nvram_data_t *nvram_data)
{
	
	 uint8_t write_data[128];
	for(int i = 0; i < 128; ++i)write_data[i] = i;
	
	uint16_t temp = 0xAB37;
	
	static char str[80];
	memset(str, 0x00, sizeof(str));
	
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
	flashc_memcpy((void *)str,  (void *)&nvram_data->var16, sizeof(nvram_data->var16),  false);
	
		
}

void avr_flash_test()
{
	flash_rw_example("user_page", &user_nvram_data);		
}

void bootloader_init(void)
{
	
	flash_read_app_start();
	
	if (current_app_type == APP_TYPE_BOOTLOADER)
	{
		df_boot_info_t boot_info;
		memset(&boot_info, 0x00, sizeof(df_boot_info_t));
	
		//read boot_info 
		memcpy((void*)&boot_info, (void*)BOOT_INFO_START_ADD, BOOT_INFO_SIZE);
		
		//reset version
		memcpy(boot_info.version, boot_version, sizeof(boot_version));	
		
		//write boot_info 
		flashc_memcpy(BOOT_INFO_START_ADD, &boot_version, BOOT_INFO_SIZE, true);
			
	}
	else
	{
		df_third_party_info_t third_party_info;
		memset(&third_party_info, 0x00, sizeof(df_third_party_info_t));
			
		//read third_party info
		memcpy((void*)&third_party_info, (void*)THIRD_PARTY_INFO_START_ADD, THIRD_PARTY_INFO_SIZE);

		//flashc_erase_user_page(true);
		//unsigned long mainaddr = (unsigned long)main;
		third_party_info.type = MOTO_PATROL;
		third_party_info.isValid = 1;
		//memcpy(third_party_info.addr, &mainaddr, sizeof(mainaddr));不需要重设固件执行地址
		memcpy(third_party_info.version, third_party_version, sizeof(third_party_version));
				
		//write third_party info
		flashc_memcpy(THIRD_PARTY_INFO_START_ADD, &third_party_info, THIRD_PARTY_INFO_SIZE, true);
	}
	
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

void parse_flash_protocol(flash_proto_t *p, U8 rx_sessionID)
{	
	bool ret =false;
	uint8_t opcode = p->opcode;
	flash_proto_t tx_buf;
	uint8_t info_data[sizeof(tx_buf.proto_payload.df_read_info_reply.info)] ={0};
	switch(opcode)
	{
		//case ENTER_BOOT_REQ_OPCODE:
		//
				//log_debug("rx ENTER_BOOT_REQ_OPCODE.");
				//if(0)
				//{
					//tx_buf.proto_payload.df_enter_boot_reply.result = DF_WAIT_TO_RESET;					
				//}
				//else
				//{			
					//tx_buf.proto_payload.df_enter_boot_reply.result = DF_SUCCESS;
					//g_is_inBOOT = true;
				//}
										//
				//tx_buf.opcode = ENTER_BOOT_RLY_OPCODE;
				//tx_buf.payload_len =1;
				//tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				//xcmp_send_session_broadcast(ENTER_BOOT_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				//
			//break;
			
		case READ_INFO_REQ_OPCODE:
		
				log_debug("rx READ_INFO_REQ_OPCODE.");
				tx_buf.opcode = READ_INFO_RLY_OPCODE;
				avrflash_read_info(info_data);
				memcpy(tx_buf.proto_payload.df_read_info_reply.info, info_data, sizeof(info_data));
				tx_buf.payload_len =33;
				tx_buf.proto_payload.df_read_info_reply.result = DF_SUCCESS;				
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(READ_INFO_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				
			break;
			
		case WRITE_ID_REQ_OPCODE:
		
				log_debug("rx WRITE_ID_REQ_OPCODE.");
				avrflash_write_id(p->proto_payload.df_write_id_request.condition,
									 p->proto_payload.df_write_id_request.id);
				tx_buf.opcode = WRITE_ID_RLY_OPCODE;
				tx_buf.payload_len =1;
				tx_buf.proto_payload.df_write_id_reply.result= DF_SUCCESS;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(WRITE_ID_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				
			break;
			
			
		case CHECK_MEMORY_REQ_OPCODE:
				
				log_debug("rx CHECK_MEMORY_REQ_OPCODE.");
				ret = avrflash_validate_flashing_addr(p->proto_payload.df_check_flash_memory_request.programStartAddr,
												p->proto_payload.df_check_flash_memory_request.fileSize);
												
												
				log_debug("FlashC Check:Start:0x%X, Len:0x%X\t Result:%d."
				, p->proto_payload.df_check_flash_memory_request.programStartAddr
				,p->proto_payload.df_check_flash_memory_request.fileSize
				,ret
				);
				
				if(ret == true)
				{
					tx_buf.proto_payload.df_check_flash_memory_reply.result = DF_SUCCESS;
					third_partyStartAddr_ = p->proto_payload.df_check_flash_memory_request.programStartAddr;//更新固件运行的起始地址
					if (current_app_type == APP_TYPE_3_PARTY)
						avrflash_update_third_party_info();//更新第三方应用的起始地址。
				}
				else
					tx_buf.proto_payload.df_check_flash_memory_reply.result = DF_FAILURE;
					
				tx_buf.opcode = CHECK_MEMORY_RLY_OPCODE;
				tx_buf.payload_len =1;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(CHECK_MEMORY_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				
				
				break;	
				
		case ERASE_REQ_OPCODE:
				
				log_debug("rx ERASE_REQ_OPCODE.");
				//start erase flash
				if(g_isEraseFlash == false)
					setTimer(ERASE_FLASH_TIMER, TIME_BASE_25MS, false, avrflash_erase_in_multitask_func, NULL);	
				if(g_isEraseFlashFinished_ == false)
				{			
					//avrflash_erase();
					tx_buf.proto_payload.df_erase_reply.result = DF_ERASING;
				}
				else
				{
					tx_buf.proto_payload.df_erase_reply.result = DF_SUCCESS;
					g_isEraseFlashFinished_ = false;//clear flag
				}

				tx_buf.opcode = ERASE_RLY_OPCODE;
				tx_buf.payload_len =1;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(EXIT_BOOT_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				
				break;		
			
		case PROGRAM_FLASH_REQ_OPCODE:
		
				log_debug("rx PROGRAM_FLASH_REQ_OPCODE.");
				
				ret = avrflash_validate_flashing_addr(p->proto_payload.df_program_request.addr,
									p->payload_len-sizeof(p->proto_payload.df_program_request.addr));
				if(ret == true )
				{
					write_flash_in_multitask(p->proto_payload.df_program_request.addr,
										p->proto_payload.df_program_request.data,
										(p->payload_len-sizeof(p->proto_payload.df_program_request.addr)),
										false);//已经擦除了。
					tx_buf.proto_payload.df_program_reply.result = DF_SUCCESS;
				}
				else
				{
					tx_buf.proto_payload.df_program_reply.result = DF_FAILURE;
				}
			
				tx_buf.opcode = PROGRAM_FLASH_RLY_OPCODE;
				tx_buf.payload_len =5;
				tx_buf.proto_payload.df_program_reply.addr = p->proto_payload.df_program_request.addr;
				tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				xcmp_send_session_broadcast(PROGRAM_FLASH_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				
			break;			
		//
		//case EXIT_BOOT_REQ_OPCODE:
		//
				//log_debug("rx EXIT_BOOT_REQ_OPCODE.");
				//
				//g_is_inBOOT = false;
				//avrflash_update_third_party_info();
				//
				//tx_buf.opcode = EXIT_BOOT_RLY_OPCODE;
				//tx_buf.payload_len =1;
				//tx_buf.proto_payload.df_exit_boot_reply.result = DF_SUCCESS;
				//tx_buf.checkSum = df_payload_checksunm(&(tx_buf.payload_len), (tx_buf.payload_len +1));
				//xcmp_send_session_broadcast(EXIT_BOOT_RLY_OPCODE, &tx_buf,3 + tx_buf.payload_len, rx_sessionID);
				//
			//break;
			
		default:
			log_debug("flash opcode err:[0x%x]", opcode);
			break;
		
	}
	
	
}