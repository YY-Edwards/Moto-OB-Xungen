/*
 * voice.c
 *
 * Created: 2017/6/9 星期五 下午 17:32:44
 *  Author: Edwards
 */ 
#include "voice.h"

static Bool voice_list_info_init(U8 *voice_count_ptr);

volatile const char FlashLabel[] = { "MOTOREC"};
static unsigned short current_voice_index = 0;
static unsigned int	  current_save_voice_offset = VOICE_DATA_START_ADDRESS;
static U8 list_init_success_flag = 0;

extern char AMBE_AudioData[];

U8 TEMP_BUF[4096];


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2017 SHJH, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: voice_list_info_init
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by voc_init() to create voice(AMBE+) storage list for data flash.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
*	input param: 2bytes
*   
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 20-Fri-17   Edwards    none        Initial draft
*
*******************************************************************************/
static Bool voice_list_info_init(U8 *voice_count_ptr)
{
	df_status_t return_code = DF_OK;
	unsigned int i = 0;
	unsigned int address =0x00000000;
	char str[80];
	memset(str, 0x00, sizeof(str));
	
	start:

	/* bytes remained less than one page */
	return_code = data_flash_read_block(LABEL_ADDRESS, LABEL_LENGTH, (U8 *)str);
	if(return_code == DF_OK)
	{
		if(memcmp((U8*)FlashLabel, str, sizeof(FlashLabel)-1) != 0)//compare label
		{
			ERASE:
			//erase
			for(i; i < (VOICE_LIST_BOUNDARY/(64*1024)); i++)//8*64k
			{
				return_code = data_flash_erase_block(address, DF_BLOCK_64KB);
				if(return_code != DF_ERASE_COMPLETED)
				{
					return FALSE;
				}
				address+=65536;//64k*1024=65536bytes
			}
			//set label
			return_code = data_flash_write((U8 *)FlashLabel, LABEL_ADDRESS, LABEL_LENGTH);
			//set current_voice_index
			memset(str, 0x00, sizeof(str));
			return_code = data_flash_write((U8 *)str, VOICE_NUMBERS_ADDRESS, VOICE_NUMBERS_LENGTH);
			if(return_code != DF_WRITE_COMPLETED)
			{
				return FALSE;
			}
			current_voice_index = 0;
			mylog("\r\n----create voice info okay!----\r\n");
		}
		else//success
		{
			mylog("\nLABEL: %s\n", str);
			//Get the current voice index
			return_code = data_flash_read_block(VOICE_NUMBERS_ADDRESS, VOICE_NUMBERS_LENGTH, (U8*)&current_voice_index);
			if(return_code == DF_OK)
			{
				//Calculates the offset address of the current stored voice
				if(current_voice_index != 0){
					
					address = START_ADDRESS_OF_VOICE_INFO + ((current_voice_index -1)*VOICE_INFO_LENGTH);
					return_code = data_flash_read_block(address, VOICE_INFO_LENGTH, (U8 *)str);
					return_code = data_flash_read_block(LABEL_ADDRESS, 512, (U8 *)TEMP_BUF);
					if(return_code == DF_OK)
					{
						VoiceList_Info_t *ptr = (VoiceList_Info_t *)str;
						if(ptr->numb == current_voice_index)
						{
							current_save_voice_offset = ptr->address + ptr->offset;
							mylog("current_save_voice_offset : %X\n", current_save_voice_offset);
							if(current_save_voice_offset > 0x7bc000){
								
								mylog("\r\n----voice storage is full!!!----\r\n");
								//chip erase
								return_code = data_flash_erase_block(0, DF_BLOCK_ALL);
								if(return_code == DF_ERASE_COMPLETED)goto start;
								else
								return FALSE;
							}
						}
						else
						{
							mylog("\r\n----voice storage is err!!!----\r\n");
							goto ERASE;
							//return FALSE;
						}
					}
				}
				mylog("\r\n----read voice info okay!----\r\n");
			}
			else
			return FALSE;
		}
		
		memcpy(voice_count_ptr, &current_voice_index, sizeof(current_voice_index));
		list_init_success_flag = 1;
		return TRUE;
	}
	return FALSE;

}


Bool voc_read_info( unsigned int voice_index,  VoiceList_Info_t * voice)
{
	if(!list_init_success_flag)return FALSE;
		
	/* check input parameter */
	if (voice_index > current_voice_index)
	{
		return -1;
	}
	
	df_status_t return_code = DF_OK;
	char str[VOICE_INFO_LENGTH];
	memset(str, 0x00, sizeof(str));
	unsigned int address =0x00000000;
	//find the voice storage info by voice_index
	address = START_ADDRESS_OF_VOICE_INFO + ((voice_index -1)*VOICE_INFO_LENGTH);
	return_code = data_flash_read_block(address, VOICE_INFO_LENGTH, (U8 *)str);
	if (return_code == DF_OK)
	{
		if(((VoiceList_Info_t *)str)->numb == voice_index)
		{
			memcpy(voice, str, sizeof(VoiceList_Info_t));
			return TRUE;
			
		}
		else
			return FALSE;
	}
	else
		return FALSE;
	
}


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2017 SHJH, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: playback_voice_data
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by playback to read voice data.
*
*---------------------------------- SYNOPSIS -----------------------------------
*
*--------------------------- DETAILED DESCRIPTION ------------------------------
*
* To keep read operation more controllable, a 4KB limitation is applied on return
* parameter length.
*
*------------------------------- REVISIONS -------------------------------------
* Date        Name      Prob#       Description
* ----------  --------  ----------  --------------------------------------------
* 20-Fri-17   Edwards    none        Initial draft
*
*******************************************************************************/
U8 PLAYBACK_BUF[512];
Bool playback_voice_data(U32 voice_index)
{
	if(!list_init_success_flag)return FALSE;
	
	/* check input parameter */
	if (voice_index > current_voice_index)
	{
		return -1;
	}
	df_status_t return_code = DF_OK;
	unsigned int address =0x00000000;
	char str[VOICE_INFO_LENGTH];
	memset(str, 0x00, sizeof(str));
	//find the voice storage info by voice_index
	address = START_ADDRESS_OF_VOICE_INFO + ((voice_index -1)*VOICE_INFO_LENGTH);
	return_code = data_flash_read_block(address, VOICE_INFO_LENGTH, (U8 *)str);
	if (return_code == DF_OK)
	{
		U16 bytes_remained;
		VoiceList_Info_t *ptr = (VoiceList_Info_t *)str;
		if(ptr->numb == voice_index)
		{
			bytes_remained = ptr->offset;
			address = ptr->address;
			
			while (bytes_remained >= 1 && return_code == DF_OK)
			{
				if(bytes_remained < DF_DATA_SPACE_SIZE)//< 512bytes
				{
					return_code = data_flash_read_block(address, bytes_remained, PLAYBACK_BUF);
					bytes_remained = 0;	/* end while loop */

				}
				else//bytes_remained > DF_DATA_SPACE_SIZE
				{
					return_code = data_flash_read_block(address, DF_DATA_SPACE_SIZE, PLAYBACK_BUF);
					bytes_remained-=DF_DATA_SPACE_SIZE;
					address+=DF_DATA_SPACE_SIZE;
					
				}
				memset(PLAYBACK_BUF, 0x00, DF_DATA_SPACE_SIZE);
			}
			return TRUE;
		}
	}
	
	return -1;
	
}


/*******************************************************************************
*
*                     C L U S T E R    F U N C T I O N
*
*            COPYRIGHT 2017 SHJH, INC. ALL RIGHTS RESERVED.
*
********************************************************************************
*
* FUNCTION NAME: voc_save_data
*
*---------------------------------- PURPOSE ------------------------------------
*
* This function is called by record to save voice data.
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
* 20-Fri-17   Edwards    none        Initial draft
*
*******************************************************************************/
static U32 current_bytes_remained = 0;

Bool voc_save_data(void *data_ptr, U16 data_len, U8 voice_end_flag)
{
	if(!list_init_success_flag)return FALSE;
	
//	U32 address = 0;
	//static U32 bytes_remained = 0;
	df_status_t return_code = DF_WRITE_COMPLETED;
	
	current_bytes_remained+=data_len;//accumulate
	/* check input parameter */
	if (data_ptr == NULL || data_len > 0x1000)
	{
		return FALSE;
	}
	if(current_bytes_remained > 0xFFFF)//data size > 65535bytes == 64k,overout
	{
		//current_bytes_remained = 0;
		return FALSE;
	}
	//save data
	if(current_save_voice_offset > DF_MAX_ADDR)//The voice data is out of bounds
	{
		return FALSE;
		mylog("\r\n----voice data is Out of bounds!!!\r\n----");
	}
	return_code = data_flash_write((U8 *)data_ptr, current_save_voice_offset, data_len);
	if(return_code != DF_WRITE_COMPLETED)
	{
		return FALSE;
	}
	current_save_voice_offset+=data_len;
	
	
	//VoiceList_Info_t *ptr = malloc(sizeof(VoiceList_Info_t));
	//if(ptr ==NULL)return FALSE;
	
	if(voice_end_flag == TRUE)//save a voice-info into list at the end of the recording
	{
		current_voice_index++;
		//ptr->numb		= current_voice_index;
		//ptr->address	= (current_save_voice_offset - bytes_remained);
		//ptr->offset		= bytes_remained;
		//
		//address = START_ADDRESS_OF_VOICE_INFO + ((current_voice_index -1)*VOICE_INFO_LENGTH);
		//if(address > VOICE_LIST_BOUNDARY)//The number of messages is out of bounds
		//{
			//if(ptr != NULL)
			//free(ptr);
			//ptr = NULL;
			//return FALSE;
			//mylog("\r\n----info list is Out of bounds!!!\r\n----");
		//}
		////set a voice info by current_voice_index
		//return_code = data_flash_write((U8 *)ptr, address, VOICE_INFO_LENGTH);
		////set voice numbers
		//return_code = data_flash_write(&current_voice_index, VOICE_NUMBERS_ADDRESS, VOICE_NUMBERS_LENGTH);
		//if(return_code != DF_WRITE_COMPLETED)
		//{
			//if(ptr != NULL)
			//free(ptr);
			//ptr = NULL;
			//return FALSE;
		//}
		//
		//current_bytes_remained = 0;//reset 0
	}
	
	//if(ptr != NULL)
	//free(ptr);
	//ptr = NULL;
	return TRUE;

}

Bool voc_save_info(VoiceList_Info_t * voice)
{
	//VoiceList_Info_t *ptr = malloc(sizeof(VoiceList_Info_t));
	//if(ptr ==NULL)return FALSE;
	U32 address = 0;
	df_status_t return_code = DF_WRITE_COMPLETED;
	
	//if(voice_end_flag == TRUE)//save a voice-info into list at the end of the recording
	{
		//current_voice_index++;
		voice->numb		= current_voice_index;
		voice->address	= (current_save_voice_offset - current_bytes_remained);
		voice->offset		= current_bytes_remained;
		
		address = START_ADDRESS_OF_VOICE_INFO + ((current_voice_index -1)*VOICE_INFO_LENGTH);
		if(address > VOICE_LIST_BOUNDARY)//The number of messages is out of bounds
		{
			return FALSE;
			mylog("\r\n----info list is Out of bounds!!!\r\n----");
		}
		//set a voice info by current_voice_index
		return_code = data_flash_write((U8 *)voice, address, VOICE_INFO_LENGTH);
		//set voice numbers
		return_code = data_flash_write((U8*)&current_voice_index, VOICE_NUMBERS_ADDRESS, VOICE_NUMBERS_LENGTH);
		if(return_code != DF_WRITE_COMPLETED)
		{
			return FALSE;
		}
		
		current_bytes_remained = 0;//reset 0
	}
	
	return TRUE;

}

VoiceList_Info_t voice_ptr;
VoiceList_Info_t voice_temp;

void voc_read_write_test(void)
{
	U16 status = 0xff;
	
	/**save**/
	voc_save_data(AMBE_AudioData, 600, TRUE);
	//save voice information
	voc_save_info(&voice_ptr);
	
	voc_save_data(&AMBE_AudioData[600], 300, FALSE);
	voc_save_data(&AMBE_AudioData[950], 500, TRUE);
	memset(&voice_ptr, 0x00, sizeof(voice_ptr));
	voice_ptr.Header.Header = 0xABCD5A5A;
	//save voice information
	voc_save_info(&voice_ptr);
	
	
	voc_save_data(AMBE_AudioData, 1024, TRUE);
	//save voice information
	voc_save_info(&voice_ptr);
	
	
	/**read**/
	status = voc_read_info(1, &voice_temp);
	if(status ==TRUE){
		
		playback_voice_data(1);
		memset(&voice_temp, 0x00, sizeof(voice_temp));
		
	}
	
	status = voc_read_info(3, &voice_temp);
	if(status ==TRUE){
		
		playback_voice_data(3);
		memset(&voice_temp, 0x00, sizeof(voice_temp));
		
	}
	
	status = voc_read_info(20, &voice_temp);
	if(status ==TRUE){
			
		playback_voice_data(20);
		memset(&voice_temp, 0x00, sizeof(voice_temp));
			
	}
	
	
}

U8 Current_total_voice[2] = {0};
void voc_init(void)
{
	data_flash_init();
	
	// after 5 seconds, perform a test on write and read back 0x5A5A to address 0x00001002
	// then read address 0x00001002 every 5s and report to radio with failure
	//test_data_flash(FALSE);
	//create_data_flash_test_task();
	voice_list_info_init(Current_total_voice);
	
	//voc_read_write_test();

}