/*
 * xgflash.c
 *
 * Created: 2017/11/8 16:49:42
 *  Author: Edwards
 */ 

#include "xgflash.h"

volatile const char XGFlashLabel[] = {"XUNGENG"};
static unsigned short current_message_index = 0;
static unsigned int	  current_save_message_offset = XG_MESSAGE_DATA_START_ADD;
static U8 list_init_success_flag = 0;
U8 TEMP_BUF[4096];

/*! \brief This is an example demonstrating flash read / write data accesses
 *         using the FLASHC driver.
 *
 * \param caption     Caption to print before running the example.
 * \param nvram_data  Pointer to the NVRAM data structure to use in the example.
 */
/*
void flash_rw_example(const char *caption, nvram_data_t *nvram_data)
{
	static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};

	Disable_interrupt_level(1);
	flashc_memset((void *)nvram_data, 0x00, 8, sizeof(*nvram_data), true);
	Enable_interrupt_level(1);
	
	Disable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var8,   &write_data, sizeof(nvram_data->var8),   true);
	Enable_interrupt_level(1);
	
	Disable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var16,  &write_data, sizeof(nvram_data->var16),  true);
	Enable_interrupt_level(1);
	flashc_memcpy((void *)&nvram_data->var8_3, &write_data, sizeof(nvram_data->var8_3), true);
	flashc_memcpy((void *)&nvram_data->var32,  &write_data, sizeof(nvram_data->var32),  true);
	
}

*/

static Bool xgflash_list_info_init(U8 *xg_message_count_ptr)
{
	
	static U32 current_page_number =0;
	unsigned int i = 0;
	unsigned int address =0x00000000;
	static char str[80];
	memset(str, 0x00, sizeof(str));
	
start:
	
	 //bytes remained less than one page 
	flashc_memcpy((void *)str, (void *)LABEL_ADDRESS, LABEL_LENGTH,  true);
	flashc_memcpy((void *)LABEL_ADDRESS, (void *)LABEL_ADDRESS, LABEL_LENGTH,  false);//为了获取当前页号码
	current_page_number = flashc_get_page_number();
	if (flashc_is_lock_error() || flashc_is_programming_error())
	{
		return false;
	}
	else
	{
		if(memcmp(XGFlashLabel, str, sizeof(XGFlashLabel)-1) != 0)//compare label
		{
			ERASE:
			//erase:160pages		
			//for(i=0; i < ((XG_MESSAGE_DATA_BOUNDARY_ADD - XG_MESSAGE_LISTINFO_START_ADD)/(PageSize)); i++)//80k
			for(i=0; i <25; i++)//擦除太多页程序运行不正常。
			{
				current_page_number+=i;
				flashc_erase_page(current_page_number, true);
			}
			//set label
			flashc_memcpy((void *)LABEL_ADDRESS, (void *)XGFlashLabel, LABEL_LENGTH,  true);
			
			//set current_voice_index
			memset(str, 0x00, sizeof(str));
			
			flashc_memcpy((void *)MESSAGE_NUMBERS_ADD, (void *)str, MESSAGE_NUMBERS_LENGTH,  true);
			if (flashc_is_lock_error() || flashc_is_programming_error())
			{
				return false;
			}
			current_message_index = 0;
			log("\r\n----create xg message info okay!----\r\n");
		}
		else//success
		{
			log("\nLABEL: %s\n", str);
			//Get the current voice index		
			flashc_memcpy((void *)&current_message_index, (void *)MESSAGE_NUMBERS_ADD, MESSAGE_NUMBERS_LENGTH,  false);
			if (flashc_is_lock_error() || flashc_is_programming_error())
			{
				return false;
			}
			else
			{
				//Calculates the offset address of the current stored message
				if(current_message_index != 0){
					memset(str, 0x00, sizeof(str));	
					address = XG_MESSAGE_INFO_HEADER_START_ADD + ((current_message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
					flashc_memcpy((void *)str, (void *)address, XG_MESSAGE_INFO_HEADER_LENGTH,  false);
					if (flashc_is_lock_error() || flashc_is_programming_error())
					{
						log("\r\n----voice storage is err!!!----\r\n");
						goto ERASE;
					}
					else
					{
						MessageList_Info_t *ptr = (MessageList_Info_t *)str;
						if(ptr->numb == current_message_index)
						{
							current_save_message_offset = ptr->address + ptr->offset;
							if(current_save_message_offset > XG_MESSAGE_DATA_BOUNDARY_ADD){
										
								log("\r\n----voice storage is full!!!----\r\n");
								//xgflash erase
								
								flashc_memset64((void *)LABEL_ADDRESS, (void *)0x00, LABEL_LENGTH,  true);
								goto start;

							}
						}
						else
						{
							log("\r\n----voice storage is err!!!----\r\n");
							goto ERASE;
						}
					}
				}
				log("\r\n----xoxo read voice info okay!----\r\n");
			}
		}
				
		memcpy(xg_message_count_ptr, &current_message_index, sizeof(current_message_index));
		list_init_success_flag = 1;
		return TRUE;
			
	}
	
}

static U32 current_bytes_remained = 0;
Bool xg_message_data_save(MessageData_t *data_ptr, U16 data_len, U8 data_end_flag)
{

	if(!list_init_success_flag)return FALSE;
	U32 address = 0;
	static U32 bytes_remained = 0;
		
	current_bytes_remained+=data_len;//accumulate
	/* check input parameter */
	if (data_ptr == NULL || data_len > 0x0200)//512bytes
	{
		return FALSE;
	}
	if(current_bytes_remained > 0xF000)//data size > 60k,overout
	{
		//current_bytes_remained = 0;
		return FALSE;
	}
	//save data
	if(current_save_message_offset > XG_MESSAGE_DATA_BOUNDARY_ADD)//The message data is out of boundary
	{
		return FALSE;
		log("\r\n----voice data is Out of bounds!!!\r\n----");
	}
	
	Disable_interrupt_level(1);
	flashc_memcpy((void *)current_save_message_offset, (void *)data_ptr, data_len,  true);
	Enable_interrupt_level(1);
	if (flashc_is_lock_error() || flashc_is_programming_error())
	{
		return FALSE;
	}
	current_save_message_offset+=data_len;
		
	MessageList_Info_t ptr;
		
	if(data_end_flag == TRUE)//save a message-info into list at the end of the resend-event
	{
		current_message_index++;
		ptr.numb		= current_message_index;
		ptr.address		= (current_save_message_offset - current_bytes_remained);
		ptr.offset		= current_bytes_remained;
		
		address = XG_MESSAGE_INFO_HEADER_START_ADD + ((current_message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
		if(address > XG_MESSAGE_LISTINFO_BOUNDARY_ADD)//The number of messages is out of bounds
		{
			return FALSE;
			log("\r\n----info list is Out of bounds!!!\r\n----");
		}
		//set a message info by current_message_index		
		Disable_interrupt_level(1);
		flashc_memcpy((void *)address, (void *)&ptr, XG_MESSAGE_INFO_HEADER_LENGTH,  true);
		Enable_interrupt_level(1);
		
		//set message numbers
		Disable_interrupt_level(1);
		flashc_memcpy((void *)MESSAGE_NUMBERS_ADD, (void *)&current_message_index, MESSAGE_NUMBERS_LENGTH,  true);
		Enable_interrupt_level(1);

		if (flashc_is_lock_error() || flashc_is_programming_error())
		{
			return FALSE;
		}
		
		current_bytes_remained = 0;//reset 0
	}
		
	return TRUE;

}



U8 Current_total_message_counter[2] = {0};
void xg_flashc_init(void)
{
	
	
	//flashc_lock_all_regions(false);
	xgflash_list_info_init(Current_total_message_counter);
	log("xgflash message counter : %d\n", (Current_total_message_counter[0]&0xFF00) + Current_total_message_counter[1]);
	//flashc_lock_all_regions(true);
	//
	//static U32 falsh_size =0;
	//static U32 page_count =0;
	//static U32 page_count_per_region =0;
	//static U32 current_page_number =0;
	//static U32 current_region_number =0;
	//static U32 current_region_first_page_number =0;
	//static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	//static U8 read_data[8]={0};
	//
	//falsh_size = flashc_get_flash_size();
	//
	//page_count = flashc_get_page_count();
	//
	//page_count_per_region =	flashc_get_page_count_per_region();
	//
	////flashc_memset((void *)0x8004b000, 0x0000, 8, 6, true);
	////Disable_global_interrupt();
	//flashc_memcpy((void *)read_data,  (void *)0x80041000, 6,  true);
	////Enable_global_interrupt();
	//current_page_number = flashc_get_page_number();
	
	//current_region_number = flashc_get_page_region(current_page_number);
	
	//current_region_first_page_number =flashc_get_region_first_page_number(current_region_number);
	
	
	//flashc_memset((void *)0x80021001, 0x0000, 8, 6, true);
	//char ch =0;
	//nvram_data_t  *flash_nvram_data ;
	//flash_nvram_data = 0x80003200;//LABEL_ADDRESS;
	//nvram_data_t temp_data;
	//static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	//
	//flashc_lock_all_regions(false);
	//
	//flashc_memset((void *)0x80021001, 0x0000, 8, 6, true);
	//
	//flashc_memcpy((void *)0x80021007,  &write_data, 6,  true);
	//
	//flashc_memcpy((void *)0x80021007,  &write_data, 8,  true);
	//
	////flash_rw_example(ch, flash_nvram_data);
	////flash_rw_example(ch, &temp_data);
	
}

