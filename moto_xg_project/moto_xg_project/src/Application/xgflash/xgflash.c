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
	char str[80];
	memset(str, 0x00, sizeof(str));
	
start:
	
	Disable_interrupt_level(1);
	 //bytes remained less than one page 
	flashc_memcpy((void *)str, (void *)LABEL_ADDRESS, LABEL_LENGTH,  true);
	current_page_number = flashc_get_page_number();
	Enable_interrupt_level(1);
	if (flashc_is_lock_error() || flashc_is_programming_error())
	{
		return false;
	}
	else
	{
		if(memcmp(XGFlashLabel, str, sizeof(XGFlashLabel)-1) != 0)//compare label
		{
			ERASE:
			//erase:40pages
			
			for(i=0; i < ((XG_MESSAGE_LISTINFO_BOUNDARY_ADD - XG_MESSAGE_LISTINFO_START_ADD)/(PageSize)); i++)//20k
			{
				current_page_number+=i;
				flashc_erase_page(current_page_number, true);
			}
			//set label
			Disable_interrupt_level(1);
			flashc_memcpy((void *)LABEL_ADDRESS, (void *)XGFlashLabel, LABEL_LENGTH,  true);
			Enable_interrupt_level(1);
			
			//set current_voice_index
			memset(str, 0x00, sizeof(str));
			
			Disable_interrupt_level(1);
			flashc_memcpy((void *)MESSAGE_NUMBERS_ADD, (void *)str, MESSAGE_NUMBERS_LENGTH,  true);
			Enable_interrupt_level(1);
			if (flashc_is_lock_error() || flashc_is_programming_error())
			{
				return false;
			}
			current_message_index = 0;
			log("\r\n----create xg message info okay!----\r\n");
		}
		else//success
		{
			//Get the current voice index
			
			Disable_interrupt_level(1);
			flashc_memcpy((void *)&current_message_index, (void *)MESSAGE_NUMBERS_ADD, MESSAGE_NUMBERS_LENGTH,  true);
			Enable_interrupt_level(1);
			if (flashc_is_lock_error() || flashc_is_programming_error())
			{
				return false;
			}
			else
			{
				//Calculates the offset address of the current stored voice
				if(current_message_index != 0){
							
					address = XG_MESSAGE_INFO_HEADER_START_ADD + ((current_message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
					Disable_interrupt_level(1);
					flashc_memcpy((void *)&str, (void *)address, XG_MESSAGE_INFO_HEADER_LENGTH,  true);
					flashc_memcpy((void *)&TEMP_BUF, (void *)LABEL_ADDRESS, 512,  true);
					Enable_interrupt_level(1);
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
							if(current_save_message_offset > 0x7bc000){
										
								log("\r\n----voice storage is full!!!----\r\n");
								//xgflash erase
								
								Disable_interrupt_level(1);
								//bytes remained less than one page
								flashc_memcpy((void *)str, (void *)LABEL_ADDRESS, LABEL_LENGTH,  true);
								current_page_number = flashc_get_page_number();
								Enable_interrupt_level(1);
								flashc_erase_page(current_page_number, true);
								goto start;

							}
						}
					}
				}
				log("\r\n----read voice info okay!----\r\n");
			}
		}
				
		memcpy(xg_message_count_ptr, &current_message_index, sizeof(current_message_index));
		list_init_success_flag = 1;
		return TRUE;
			
	}
	
}


void xg_flashc_init(void)
{
	
	static U32 falsh_size =0;
	static U32 page_count =0;
	static U32 page_count_per_region =0;
	static U32 current_page_number =0;
	static U32 current_region_number =0;
	static U32 current_region_first_page_number =0;
	
	falsh_size = flashc_get_flash_size();
	
	page_count = flashc_get_page_count();
	
	page_count_per_region =	flashc_get_page_count_per_region();
	
	flashc_memset((void *)0x8004b000, 0x0000, 8, 6, true);
	
	current_page_number = flashc_get_page_number();
	
	current_region_number = flashc_get_page_region(current_page_number);
	
	current_region_first_page_number =flashc_get_region_first_page_number(current_region_number);
	
	
	flashc_memset((void *)0x80021001, 0x0000, 8, 6, true);
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