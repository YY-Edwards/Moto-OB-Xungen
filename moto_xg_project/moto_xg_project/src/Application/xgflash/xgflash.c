/*
 * xgflash.c
 *
 * Created: 2017/11/8 16:49:42
 *  Author: Edwards
 */ 

#include "xgflash.h"


/*the queue is used to storage failure-send message*/
volatile xQueueHandle message_storage_queue = NULL;

/*the queue is used to receive failure-send message*/
volatile xQueueHandle xg_resend_queue = NULL;

volatile const char XGFlashLabel[] = {"PATROL"};
static unsigned short current_message_index = 0;
static unsigned int	  current_save_message_offset = XG_MESSAGE_DATA_START_ADD;
static U8 list_init_success_flag = 0;
//U8 TEMP_BUF[4096];
static volatile xSemaphoreHandle xgflash_mutex = NULL;

void runXGFlashTestSAVE( void *pvParameters );
void runXGFlashTestREAD( void *pvParameters );
volatile  xTaskHandle save_handle;  
//volatile  xSemaphoreHandle xBinarySemaphore;
volatile xSemaphoreHandle SendM_CountingSemaphore = NULL;
volatile Message_Protocol_t message_store[MAX_MESSAGE_STORE];

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

static xgflash_status_t xgflash_list_info_init(void)
{
	df_status_t return_code = DF_OK;
	static U32 current_page_number =0;
	unsigned int i = 0;
	unsigned int address =0x00000000;
	static char str[80];
	memset(str, 0x00, sizeof(str));
	
start:
	
	 //bytes remained less than one page 
	return_code = data_flash_read_block(LABEL_ADDRESS, LABEL_LENGTH, str);
	{
		if(memcmp(XGFlashLabel, str, sizeof(XGFlashLabel)-1) != 0)//compare label
		{
			ERASE:
			//erase list
			for(i; i < (XG_MESSAGE_LISTINFO_BOUNDARY_ADD/(64*1024)); i++)//2*64k
			{
				return_code = data_flash_erase_block(address, DF_BLOCK_64KB);
				if(return_code != DF_ERASE_COMPLETED)
				{
					return FALSE;
				}
				address+=65536;//64k*1024=65536bytes
			}
			//set label
			return_code = data_flash_write(XGFlashLabel, LABEL_ADDRESS, LABEL_LENGTH);
			
			//set current_voice_index
			memset(str, 0x00, sizeof(str));
			return_code = data_flash_write(str, MESSAGE_NUMBERS_ADD, MESSAGE_NUMBERS_LENGTH);
			if(return_code != DF_WRITE_COMPLETED)
			{
				return FALSE;
			}
			current_message_index = 0;
			log("\r\n----create xg message info okay!----\r\n");
		}
		else//success
		{
			log("\nLABEL: %s\n", str);
			//Get the current voice index
			return_code = data_flash_read_block(MESSAGE_NUMBERS_ADD, MESSAGE_NUMBERS_LENGTH, &current_message_index);								
			if(return_code == DF_OK)
			{
				//Calculates the offset address of the current stored message
				if(current_message_index != 0){
					
					log("current_message_index: %d\n", current_message_index);
					memset(str, 0x00, sizeof(str));	
					address = XG_MESSAGE_INFO_HEADER_START_ADD + ((current_message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
					return_code = data_flash_read_block(address, XG_MESSAGE_INFO_HEADER_LENGTH, (U8 *)str);			
					if(return_code == DF_OK)
					{
						MessageList_Info_t *ptr = (MessageList_Info_t *)str;
						if(ptr->numb == current_message_index)
						{
							current_save_message_offset = ptr->address + ptr->offset;
							log("current_save_message_offset : %X\n", current_save_message_offset);
							if(current_save_message_offset > XG_MESSAGE_DATA_BOUNDARY_ADD){
										
								log("\r\n----message storage is full!!!----\r\n");
								//xgflash erase

								return_code = data_flash_erase_block(0, DF_BLOCK_ALL);
								if(return_code == DF_ERASE_COMPLETED)goto start;
								else
								return FALSE;

							}
						}
						else
						{
							log("\r\n----message storage is err!!!----\r\n");
							goto ERASE;
						}
					}
				}
				log("\r\n----xoxo read message info okay!----\r\n");
			}
			else
				return FALSE;
			
		}
				
		list_init_success_flag = 1;
		return XG_OK;
			
	}
	return XG_ERROR;
	
}
U16 xgflash_get_message_count(void)
{
	if(!list_init_success_flag)return 0xFFFF;
	
	xSemaphoreTake(xgflash_mutex, portMAX_DELAY);
	U16 return_value = current_message_index;
	xSemaphoreGive(xgflash_mutex );

	return return_value;
	
}

static U32 current_bytes_remained = 0;
xgflash_status_t xgflash_message_save(U8 *data_ptr, U16 data_len, U8 data_end_flag)
{

	if(!list_init_success_flag)return XG_ERROR;
	U32 address = 0;
	static U32 bytes_remained = 0;
	df_status_t return_code = DF_WRITE_COMPLETED;
		
	current_bytes_remained+=data_len;//accumulate
	/* check input parameter */
	if (data_ptr == NULL || data_len > 0x0200)//512bytes
	{
		return XG_INVALID_PARAM;
	}
	if(current_bytes_remained > 0xF000)//data size > 60k,overout
	{
		//current_bytes_remained = 0;
		return XG_INVALID_PARAM;
	}
	
	xSemaphoreTake(xgflash_mutex, portMAX_DELAY) ;//lock
	
	//save data
	if(current_save_message_offset > XG_MESSAGE_DATA_BOUNDARY_ADD)//The message data is out of boundary
	{
		log("\r\n----message data is Out of bounds!!!\r\n----");
		xSemaphoreGive(xgflash_mutex );//unlock
		return XG_OUT_BOUNDARY;
	}
	
	
	return_code = data_flash_write((U8 *)data_ptr, current_save_message_offset, data_len);
	if(return_code != DF_WRITE_COMPLETED)
	{
		xSemaphoreGive(xgflash_mutex );//unlock
		return XG_FLASH_ACTION_FAIL;
	}
	
	current_save_message_offset+=data_len;
	//log("current_save_message_offset : %X\n", current_save_message_offset);
		
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
			log("\r\n----info list is Out of bounds!!!\r\n----");
			xSemaphoreGive(xgflash_mutex );//unlock
			return XG_OUT_BOUNDARY;
		}
		
		
		//set a message info by current_message_index	
		return_code = data_flash_write((U8 *)&ptr, address, XG_MESSAGE_INFO_HEADER_LENGTH);
		//set message numbers
		return_code = data_flash_write(&current_message_index, MESSAGE_NUMBERS_ADD, MESSAGE_NUMBERS_LENGTH);
		if(return_code != DF_WRITE_COMPLETED)
		{
			xSemaphoreGive(xgflash_mutex );//unlock
			return XG_FLASH_ACTION_FAIL;
		}
		
		current_bytes_remained = 0;//reset 0
	}
	
	xSemaphoreGive(xgflash_mutex );//unlock
	return XG_OK;

}

xgflash_status_t xgflash_get_message_data(U32 message_index, void *buff_ptr, bool erase)
{
	if(!list_init_success_flag)return XG_ERROR;
	
	xSemaphoreTake(xgflash_mutex, portMAX_DELAY);//lock
	/* check input parameter */
	if (message_index > current_message_index)
	{
		xSemaphoreGive(xgflash_mutex);//unlock
		return XG_INVALID_PARAM;
	}
	
	df_status_t return_code = DF_OK;
	U32 info_address =0x00000000;
	U32 data_address =0x00000000;
	char str[XG_MESSAGE_INFO_HEADER_LENGTH];
	memset(str, 0x00, sizeof(str));
	
	//find the message storage info by message_index
	info_address = XG_MESSAGE_INFO_HEADER_START_ADD + ((message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
	//flashc_memcpy((void *)str, (void *)info_address, XG_MESSAGE_INFO_HEADER_LENGTH,  false);
	return_code = data_flash_read_block(info_address, XG_MESSAGE_INFO_HEADER_LENGTH, (U8 *)str);
	if (return_code == DF_OK)
	{
		U16 bytes_remained;
		MessageList_Info_t *ptr = (MessageList_Info_t *)str;
		if(ptr->numb == message_index)
		{
			bytes_remained = ptr->offset;
			data_address = ptr->address;
			
			while (bytes_remained >= 1 && return_code == DF_OK)
			{
				if(bytes_remained < DF_DATA_SPACE_SIZE)//< 512bytes
				{
					return_code = data_flash_read_block(data_address, bytes_remained, buff_ptr);
					bytes_remained = 0;	/* end while loop */

				}
				else//bytes_remained > DF_DATA_SPACE_SIZE
				{
					return_code = data_flash_read_block(data_address, DF_DATA_SPACE_SIZE, buff_ptr);
					bytes_remained-=DF_DATA_SPACE_SIZE;
					data_address+=DF_DATA_SPACE_SIZE;
						
				}
				//memset(PLAYBACK_BUF, 0x00, DF_DATA_SPACE_SIZE);
			}
			
			if(erase)//erase the message
			{
				//erase data and reset:current_save_message_offset
				memset(str, 0x00, sizeof(str));
				return_code = data_flash_write((U8 *)str, data_address, bytes_remained);
				current_save_message_offset-=bytes_remained;
				//erase info and reset:current_message_index
				current_message_index-=1;
				return_code = data_flash_write((U8 *)str, info_address, XG_MESSAGE_INFO_HEADER_LENGTH);
				return_code = data_flash_write((U8 *)&current_message_index, MESSAGE_NUMBERS_ADD, XG_MESSAGE_INFO_HEADER_LENGTH);
			}
		
			xSemaphoreGive(xgflash_mutex);//unlock
			return XG_OK;
		}
		xSemaphoreGive(xgflash_mutex);//unlock
		return XG_INVALID_PARAM;
	}
	
	xSemaphoreGive(xgflash_mutex);//unlock
	return XG_FLASH_ACTION_FAIL;
		
}
xgflash_status_t xgflash_erase_info_region(void)
{
	if(!list_init_success_flag)return XG_ERROR;
	xgflash_status_t status = XG_ERROR;
	
	xSemaphoreTake(xgflash_mutex, portMAX_DELAY);//lock
	
	if(data_flash_erase_block(XG_MESSAGE_LISTINFO_START_ADD, DF_BLOCK_4KB) == DF_ERASE_COMPLETED)
		status = XG_ERASE_COMPLETED;
	
	xSemaphoreGive(xgflash_mutex );//unlock
	return status;
		
}
//
//void runXGFlashTestSAVE( void *pvParameters )
//{
	//Bool firstTest = TRUE;
	//static  portTickType xLastWakeTime;
	//static xgflash_status_t status = XG_ERROR;
	//const portTickType xFrequency = 20000;//2s,定时问题已经修正。2s x  2000hz = 4000
	//Message_Protocol_t data_ptr;
	//static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	//static  portTickType water_value;
	//
	//xLastWakeTime = xTaskGetTickCount();
	//while(1)
	//{
		//vTaskDelayUntil( &xLastWakeTime, xFrequency / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
//
		////xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
		//
		////xgflash_get_message_count();
		////Disable_interrupt_level(1);
		////taskENTER_CRITICAL();
		////flashc_memcpy((void *)0x80061234, (void *)write_data, 7,  true);
		////taskEXIT_CRITICAL();
		////data_ptr.data.XG_Time.Second+=1;
		////Enable_interrupt_level(1);
		////if (flashc_is_lock_error() || flashc_is_programming_error())
		////{
		////log("XG flashc_memcpy err...\n");
		////}
		////else
		//nop();
		//nop();
		//nop();
		////water_value = uxTaskGetStackHighWaterMark(NULL);
		////log("water_value: %d\n", water_value);
		////log("XG save okay!\n");
		//memset(&data_ptr.data.XG_Time.Minute, 0x01, 1);
		//status = xgflash_message_save(&data_ptr, sizeof(Message_Protocol_t), TRUE);
		//if(status == XG_OK)
		//{
			//log("XG save okay!\n");
			//data_ptr.data.XG_Time.Second+=1;
		//}
		//else
		//{
			//log("save message err : %d\n", status);
		//}
	//}
	//
//}
//void runXGFlashTestREAD( void *pvParameters )
//{
	//Bool firstTest = TRUE;
	//static  portTickType xLastWakeTime;
	//static xgflash_status_t status = XG_ERROR;
	//const portTickType xFrequency = 20000;//2s,定时问题已经修正。1.5s x  2000hz = 3000
	//Message_Protocol_t *data_ptr = (Message_Protocol_t *) pvPortMalloc(sizeof(Message_Protocol_t));
	//static U16 message_count = 0;
	//U8 return_err =0;
	//static char SN[10];
	//memset(SN, 0x00, 10);
	//
	//xLastWakeTime = xTaskGetTickCount();
	//
	//while(1)
	//{
		//vTaskDelayUntil( &xLastWakeTime, xFrequency / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
		//if(data_ptr == NULL)break;
		//message_count = xgflash_get_message_count();
		//if(message_count != 0)
		//{
			//log("message_count : %d\n", message_count);
			//status = xgflash_get_message_data(message_count, data_ptr, true);
			//if(status == XG_OK)
			//{
				//log("read out data : %d\n", data_ptr->data.XG_Time.Second);
				//
				//return_err = scan_patrol(SN);
				//if(return_err == 0){
					//
					//log("card_id : %X, %X, %X, %X\n", SN[0], SN[1], SN[2], SN[3]);
				//}
			//}
			//else
			//{
				//log("get message err : %d\n", status);
			//}
		//}
		//else
		//continue;
		//
	//}
	//vPortFree(data_ptr);
	//log("data_ptr == NULL,exit runXGFlashTestREAD\n");
	//
//}

void create_xg_flash_test_task(void)
{
	
	xTaskCreate(
	runXGFlashTestSAVE
	,  (const signed portCHAR *)"XG_SAVE"
	,  800
	,  NULL
	,  tskFLASH_PRIORITY+4
	//, NULL);
	,  &save_handle);
	
	//vTaskSuspend(save_handle);
	
	xTaskCreate(
	runXGFlashTestREAD
	,  (const signed portCHAR *)"XG_READ"
	,  880
	,  NULL
	,  tskFLASH_PRIORITY+4
	,  NULL );
	
}


//U16	Current_total_message_count=0;
void xg_flashc_init(void)
{
	
	///* Create the mutex semaphore to guard a shared xgflash.*/
	//xgflash_mutex = xSemaphoreCreateMutex();
	//if (xgflash_mutex == NULL)
	//{
		//log("Create the xgflash_mutex semaphore failure\n");
	//}
	//
	///* Create the binary semaphore to Synchronize other threads.*/
	////vSemaphoreCreateBinary(xBinarySemaphore);
	///* Create the SendM_Counting semaphore to Synchronize the event of resend-message.*/
	////计数最大值为300
	////初始值为1(当flash信息数量为0时：用户扫点 -> flash-save -> flash-count+1 -> take Sem -> send -> wait for give-Sem(success/fail))
	////如果此时反馈成功，则继续查询count值是否等于0/等待用户扫点
	////如果此时反馈失败，则flash-save
	////当flash信息数量！=0时；等待查询count值
	//SendM_CountingSemaphore = xSemaphoreCreateCounting(300, 1);
	//if (SendM_CountingSemaphore == NULL)
	//{
		//log("Create the SendM_Counting semaphore failure\n");
	//}
	//
	xg_resend_queue = xQueueCreate(100, sizeof(U32));
	/*initialize the queue*/
	message_storage_queue = xQueueCreate(100, sizeof(U32));
	for(int i= 0; i < MAX_MESSAGE_STORE; i++ )
	{
		set_message_store(&message_store[i]);//push <message_store> address to the message_storage_queue;
	}
	
	
	//data_flash_init();//interface
	
	//flashc_lock_all_regions(false);
	//xgflash_list_info_init();
	
	
	//create_xg_flash_test_task();
}















