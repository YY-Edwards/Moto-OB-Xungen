/*
 * xgflash.c
 *
 * Created: 2017/11/8 16:49:42
 *  Author: Edwards
 */ 

#include "xgflash.h"
#include "FreeRTOS.h"
#include "semphr.h"

/*the queue is used to storage failure-send message*/
volatile xQueueHandle message_storage_queue = NULL;

/*the queue is used to receive failure-send message*/
volatile xQueueHandle xg_resend_queue = NULL;

volatile const char XGFlashLabel[] = {"XUNGENG"};
static unsigned short current_message_index = 0;
static unsigned int	  current_save_message_offset = XG_MESSAGE_DATA_START_ADD;
static U8 list_init_success_flag = 0;
//U8 TEMP_BUF[4096];
static volatile xSemaphoreHandle xgflash_mutex = NULL;

void runXGFlashTestSAVE( void *pvParameters );
void runXGFlashTestREAD( void *pvParameters );
volatile  xTaskHandle save_handle;  
volatile  xSemaphoreHandle xBinarySemaphore;


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
	
	static U32 current_page_number =0;
	unsigned int i = 0;
	unsigned int address =0x00000000;
	static char str[80];
	memset(str, 0x00, sizeof(str));
	
start:
	
	 //bytes remained less than one page 
	flashc_memcpy((void *)str, (void *)LABEL_ADDRESS, LABEL_LENGTH,  true);
	flashc_memcpy((void *)LABEL_ADDRESS, (void *)LABEL_ADDRESS, LABEL_LENGTH,  false);//Ϊ�˻�ȡ��ǰҳ����
	current_page_number = flashc_get_page_number();
	if (flashc_is_lock_error() || flashc_is_programming_error())
	{
		return XG_FLASH_ACTION_FAIL;
	}
	else
	{
		if(memcmp(XGFlashLabel, str, sizeof(XGFlashLabel)-1) != 0)//compare label
		{
			ERASE:
			//erase:160pages		
			//for(i=0; i < ((XG_MESSAGE_DATA_BOUNDARY_ADD - XG_MESSAGE_LISTINFO_START_ADD)/(PageSize)); i++)//80k
			for(i=0; i <25; i++)//����̫��ҳ�������в�������
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
				return XG_FLASH_ACTION_FAIL;
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
				return XG_FLASH_ACTION_FAIL;
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
						log("\r\n----message storage is err!!!----\r\n");
						goto ERASE;
					}
					else
					{
						MessageList_Info_t *ptr = (MessageList_Info_t *)str;
						if(ptr->numb == current_message_index)
						{
							current_save_message_offset = ptr->address + ptr->offset;
							log("current_save_message_offset : %X\n", current_save_message_offset);
							if(current_save_message_offset > XG_MESSAGE_DATA_BOUNDARY_ADD){
										
								log("\r\n----message storage is full!!!----\r\n");
								//xgflash erase
								
								flashc_memset64((void *)LABEL_ADDRESS, (void *)0x00, LABEL_LENGTH,  true);
								goto start;

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
		}
				
		//memcpy(xg_message_count_ptr, &current_message_index, sizeof(current_message_index));
		list_init_success_flag = 1;
		return XG_OK;
			
	}
	
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
	
	flashc_memcpy((void *)current_save_message_offset, (void *)data_ptr, data_len,  true);
	if (flashc_is_lock_error() || flashc_is_programming_error())
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
		flashc_memcpy((void *)address, (void *)&ptr, XG_MESSAGE_INFO_HEADER_LENGTH,  true);
		
		//set message numbers
		flashc_memcpy((void *)MESSAGE_NUMBERS_ADD, (void *)&current_message_index, MESSAGE_NUMBERS_LENGTH,  true);

		if (flashc_is_lock_error() || flashc_is_programming_error())
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
		return XG_INVALID_PARAM;
	}
	
	U32 info_address =0x00000000;
	U32 data_address =0x00000000;
	char str[XG_MESSAGE_INFO_HEADER_LENGTH];
	memset(str, 0x00, sizeof(str));
	
	//find the message storage info by message_index
	info_address = XG_MESSAGE_INFO_HEADER_START_ADD + ((message_index -1)*XG_MESSAGE_INFO_HEADER_LENGTH);
	flashc_memcpy((void *)str, (void *)info_address, XG_MESSAGE_INFO_HEADER_LENGTH,  false);

	U16 bytes_remained;
	MessageList_Info_t *ptr = (MessageList_Info_t *)str;
	if(ptr->numb == message_index)
	{
		bytes_remained = ptr->offset;
		data_address = ptr->address;		
		flashc_memcpy((void *)buff_ptr, (void *)data_address, bytes_remained,  false);
		if (flashc_is_lock_error() || flashc_is_programming_error())
		{
			xSemaphoreGive(xgflash_mutex );//unlock
			return XG_FLASH_ACTION_FAIL;
		}
		if(erase)//erase the message
		{
			//erase data and reset:current_save_message_offset
			flashc_memset8((void *)data_address, 0x00, bytes_remained, true);
			current_save_message_offset-=bytes_remained;
			//erase info and reset:current_message_index
			current_message_index-=1;
			flashc_memset8((void *)info_address, 0x00, XG_MESSAGE_INFO_HEADER_LENGTH, true);
			flashc_memcpy((void *)MESSAGE_NUMBERS_ADD, (void *)&current_message_index, MESSAGE_NUMBERS_LENGTH,  true);
		}
		
		xSemaphoreGive(xgflash_mutex );//unlock
		return XG_OK;
	}
	xSemaphoreGive(xgflash_mutex );//unlock
	return XG_INVALID_PARAM;
		
}
xgflash_status_t xgflash_erase_info_region(void)
{
	if(!list_init_success_flag)return XG_ERROR;
	xgflash_status_t status = XG_ERROR;
	
	xSemaphoreTake(xgflash_mutex, portMAX_DELAY);//lock
	
	flashc_memset64((void *)LABEL_ADDRESS, (void *)0x00, LABEL_LENGTH,  true);
	if (flashc_is_lock_error() || flashc_is_programming_error())
	{
		status = XG_FLASH_ACTION_FAIL;
	}
	status = XG_ERASE_COMPLETED;
	
	xSemaphoreGive(xgflash_mutex );//unlock
	return status;
		
}

void runXGFlashTestSAVE( void *pvParameters )
{
	Bool firstTest = TRUE;
	static  portTickType xLastWakeTime;
	static xgflash_status_t status = XG_ERROR;
	const portTickType xFrequency = 3000;//2s,��ʱ�����Ѿ�������2s x  2000hz = 4000
	Message_Protocol_t data_ptr;
	static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	static  portTickType water_value;
	
	xLastWakeTime = xTaskGetTickCount();
	while(1)
	{
		vTaskDelayUntil( &xLastWakeTime, xFrequency / portTICK_RATE_MS  );//��ȷ����1000msΪ����ִ�С�

		//xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
		
		//xgflash_get_message_count();
		//Disable_interrupt_level(1);
		//taskENTER_CRITICAL();
		//flashc_memcpy((void *)0x80061234, (void *)write_data, 7,  true);
		//taskEXIT_CRITICAL();
		//data_ptr.data.XG_Time.Second+=1;
		//Enable_interrupt_level(1);
		//if (flashc_is_lock_error() || flashc_is_programming_error())
		//{
		//log("XG flashc_memcpy err...\n");
		//}
		//else
		nop();
		nop();
		nop();
		//water_value = uxTaskGetStackHighWaterMark(NULL);
		//log("water_value: %d\n", water_value);
		log("XG save okay!\n");
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
	}
	
}
void runXGFlashTestREAD( void *pvParameters )
{
	Bool firstTest = TRUE;
	static  portTickType xLastWakeTime;
	static xgflash_status_t status = XG_ERROR;
	const portTickType xFrequency = 3500;//2s,��ʱ�����Ѿ�������1.5s x  2000hz = 3000
	Message_Protocol_t *data_ptr = (Message_Protocol_t *) pvPortMalloc(sizeof(Message_Protocol_t));
	static U16 message_count = 0;
	
	xLastWakeTime = xTaskGetTickCount();
	
	while(1)
	{
		vTaskDelayUntil( &xLastWakeTime, xFrequency / portTICK_RATE_MS  );//��ȷ����1000msΪ����ִ�С�
		if(data_ptr == NULL)break;
		message_count = xgflash_get_message_count();
		if(message_count != 0)
		{
			log("message_count : %d\n", message_count);
			status = xgflash_get_message_data(message_count, data_ptr, true);
			if(status == XG_OK)
			{
				log("read out data : %d\n", data_ptr->data.XG_Time.Second);
			}
			else
			{
				log("get message err : %d\n", status);
			}
		}
		else
		continue;
		
	}
	vPortFree(data_ptr);
	log("data_ptr == NULL,exit runXGFlashTestREAD\n");
	
}

void create_xg_flash_test_task(void)
{
	
	xTaskCreate(
	runXGFlashTestSAVE
	,  (const signed portCHAR *)"XG_SAVE"
	,  550
	,  NULL
	,  tskFLASH_PRIORITY+4
	//, NULL);
	,  &save_handle);
	
	vTaskSuspend(save_handle);
	
	//xTaskCreate(
	//runXGFlashTestREAD
	//,  (const signed portCHAR *)"XG_READ"
	//,  configMINIMAL_STACK_SIZE
	//,  NULL
	//,  tskFLASH_PRIORITY
	//,  NULL );
	//
}

//U16	Current_total_message_count=0;
void xg_flashc_init(void)
{
	
	/* Create the mutex semaphore to guard a shared xgflash.*/
	xgflash_mutex = xSemaphoreCreateMutex();
	if (xgflash_mutex == NULL)
	{
		log("Create the xgflash_mutex semaphore failure\n");
	}
	
	/* Create the binary semaphore to Synchronize other threads.*/
	vSemaphoreCreateBinary(xBinarySemaphore);
	
	
	xg_resend_queue = xQueueCreate(20, sizeof(U32));
	/*initialize the queue*/
	message_storage_queue = xQueueCreate(20, sizeof(U32));
	for(int i= 0; i < MAX_MESSAGE_STORE; i++ )
	{
		set_message_store(&message_store[i]);//push <message_store> address to the message_storage_queue;
	}
		
	//flashc_lock_all_regions(false);
	//xgflash_list_info_init();
	//create_xg_flash_test_task();
	
	//Message_Protocol_t  xgmessage;
	//DateTime_t temp_time;
	//static const uint8_t write_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	//U16	Current_total_message_count=0;
	//U8 TEMP_BUF[10];
	//memset(TEMP_BUF, 0x00 , 10);
	//U8 i=0;
//
	//temp_time.Year		= 16;
	//temp_time.Month		= 2;
	//temp_time.Day		= 29;
	//temp_time.Hour		= 23;
	//temp_time.Minute		= 59;
	//temp_time.Second		= 40;
	//
	//memcpy(&xgmessage.data.XG_Time, &temp_time, sizeof(DateTime_t));
	//memcpy(&xgmessage.data.RFID_ID, write_data, 5);
	//
	//xgflash_message_save(&xgmessage.data.RFID_ID, 5, TRUE);
	//Current_total_message_count = xgflash_get_message_count();
	//
	//log("Current_total_message_count: %X\n", Current_total_message_count);
	//
	//xgflash_get_message_data(1, TEMP_BUF);
	//for(i=0; i<5; i++)
	//{
		//log("temp_buf: %X\n", TEMP_BUF[i]);
	//}
	//memset(TEMP_BUF, 0x00 , 10);
	//
	//memcpy(&xgmessage.data.RFID_ID, &write_data[1], 5);
	//xgflash_message_save(&xgmessage.data.RFID_ID, 5, TRUE);
	//Current_total_message_count = xgflash_get_message_count();
	//log("Current_total_message_count: %X\n", Current_total_message_count);
	//xgflash_get_message_data(2, TEMP_BUF);
	//for(i=0; i<5; i++)
	//{
		//log("temp_buf: %X\n", TEMP_BUF[i]);
	//}
	//memset(TEMP_BUF, 0x00 , 10);
	//
	////
	//memcpy(&xgmessage.data.RFID_ID, &write_data[2], 5);
	//xgflash_message_save(&xgmessage.data.RFID_ID, 5, TRUE);
	//Current_total_message_count = xgflash_get_message_count();
	//log("Current_total_message_count: %X\n", Current_total_message_count);
	//xgflash_get_message_data(3, TEMP_BUF);
	//for(i=0; i<5; i++)
	//{
		//log("temp_buf: %X\n", TEMP_BUF[i]);
	//}
	//memset(TEMP_BUF, 0x00 , 10);

	
	
#if 0

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
	
	#endif
	
}
