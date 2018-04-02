/*
 * myusart.c
 *
 * Created: 2018/4/1 11:27:25
 *  Author: Edwards
 */ 

#include "myusart.h"

volatile xSemaphoreHandle xCountingSem=NULL;
volatile xQueueHandle usart1_rx_xQueue=NULL;
volatile U8 peer_rx_status_flag = 0;//Ĭ������£�peer�Ǵ��ڷ���״̬��mcu�Ǵ��ڽ���״̬��

static int usart1_test_cts_status()
{
  return (APP_USART->csr & AVR32_USART_CSR_CTS_MASK);
}

/**
 * \brief The USART interrupt handler.
 *
 * \note The `__attribute__((__interrupt__))' (under GNU GCC for AVR32) and
 *       `__interrupt' (under IAR Embedded Workbench for Atmel AVR32) C function
 *       attributes are used to manage the `rete' instruction.
 */
#if defined (__GNUC__)
__attribute__((__interrupt__))
#elif defined(__ICCAVR32__)
__interrupt
#endif
static void usart1_int_handler(void)
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	static  portBASE_TYPE queue_ret = pdPASS;
	static int read_ret = USART_RX_EMPTY;
	int c;
	//��B�豸�ķ��ͣ�A�豸���գ���˵�����A�豸���ջ��������ʱ����RTS �źţ���˼֪ͨB�豸ֹͣ���ͣ���
	//B�豸ͨ��CTS ��⵽���źţ�ֹͣ���ͣ�һ��ʱ���A�豸���ջ������˿��࣬����RTS �źţ�ָʾB�豸��ʼ�������ݡ�
	//A�豸����B�豸���գ����ơ�
	/*
	 * In the code line below, the interrupt priority level does not need to
	 * be explicitly masked as it is already because we are within the
	 * interrupt handler.
	 * The USART Rx interrupt flag is cleared by side effect when reading
	 * the received character.
	 * Waiting until the interrupt has actually been cleared is here useless
	 * as the call to usart_write_char will take enough time for this before
	 * the interrupt handler is left and the interrupt priority level is
	 * unmasked by the CPU.
	 */
	//usart_read_char(EXAMPLE_USART, &c);
//
	//// Print the received character to USART. This is a simple echo.
	//usart_write_char(EXAMPLE_USART, c);
	
	//CTS�ж�
	if (usart1_test_cts_status() == 1)//0->1��ģ����MCU����������ϱ�־����ģ�鴦�ڽ���״̬��MCU������������.
	{
		peer_rx_status_flag=1;
		xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );
		DISENABLE_PEER_SEND_DATA;//��ֹpeer�������ݣ�
	} 
	else//1->0
	{
		peer_rx_status_flag=0;
	}
	
	//rx�ж�
	read_ret = usart_read_char(APP_USART, &c);
	if(read_ret == USART_SUCCESS)//�ɹ���ȡһ���ֽ�����
	{
		queue_ret = xQueueSendToBackFromISR(usart1_rx_xQueue, &c, 0);//insert data
		int remaining_bytes =0;
		remaining_bytes = uxQueueMessagesWaitingFromISR(usart1_rx_xQueue);//��ȡ������Ч���ݸ���
		if(remaining_bytes>=150)//���ܴ������ź�����������������߼�����˿����ӳ���Ӧ����ô��ʱ�������жϣ���ô���ٴδ����źţ���Ϊ�����ڲ��ϵ���
		{
			xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );//�����ź�����֪ͨ������ݲ����͵�radio
			DISENABLE_PEER_SEND_DATA;//��ֹpeer�������ݣ������ϣ�peerÿ�η���ǰ��Ҫ���RST�˿��Ƿ�����
		}
	}
	
	
}


volatile  portTickType usart1_task_water_value =0;
static void usart1_rx_data_task(void * pvParameters)
{
	//static  portTickType usart1_task_water_value =0;
	static int ret =USART_RX_EMPTY;
	static  portBASE_TYPE queue_ret = pdPASS;
	//static  portTickType xLastWakeTime;
	//xLastWakeTime = xTaskGetTickCount();
	int malloc_size =150;
	char *usart_temp_ptr=(char *)pvPortMalloc(malloc_size);//��̬����ѿռ����ݣ� ��󲻳���150bytes
	if(usart_temp_ptr==NULL)
	{
		mylog("pvPortMalloc  usart_temp_ptr failure!!!\n");
	}
	else
	{
		memset(usart_temp_ptr, 0x00, malloc_size);//���
	}

	
	ENABLE_PEER_SEND_DATA;//����ģ����MCU����usart����
	static U32 index =0;
	static int rx_char =0;
	
	while(1)
	{
		usart1_task_water_value = uxTaskGetStackHighWaterMark(NULL);
		//mylog("usart1 task water_value: %d\n", usart1_task_water_value);
		
		if(xSemaphoreTake(xCountingSem, (200*2) / portTICK_RATE_MS) == pdTRUE)
		{
			mylog("xSemaphoreTake xCountingSem  okay!\n");
			index =0;//ƫ��������
			do 
			{
				rx_char = 0;
				if(index == malloc_size)break;//��ֹ���ݿ���Խ��
				queue_ret = xQueueReceive(usart1_rx_xQueue, (usart_temp_ptr+index), 0);//ע�⣺�Ƚ��ȳ�
				if(queue_ret == pdPASS)//���в�Ϊ��
				{
					index++;
				}
				
			} while (queue_ret == pdPASS);
			
			if(index!=0)//��������������
			{	
				package_usartdata_to_csbkdata(usart_temp_ptr, index);
				memset(usart_temp_ptr, 0x00, malloc_size);//���
				
			}
			else
			{
				mylog("no usart data!!!\n");
			}

		}
		
	}
	
	vPortFree(usart_temp_ptr);
	usart_temp_ptr=NULL;

}


void third_party_interface_init(void)
{
	//����һ�������ź������Լ�¼�������ӿڷ���������ɵ��ж��¼���
	xCountingSem = xSemaphoreCreateCounting( 20, 0 );
	if(xCountingSem==NULL)
	{
		mylog("create xCountingSem failure!!!\n");
	}
	
	/*initialize the queue*/
	usart1_rx_xQueue = xQueueCreate(200, sizeof(char));//��󻺳�
	if(usart1_rx_xQueue==NULL)
	{
		mylog("create usart1_rx_xQueue failure!!!\n");
	}
	
	usart1_init();
	
	portBASE_TYPE res = xTaskCreate(
	usart1_rx_data_task
	,  (const signed portCHAR *)"usart_rx"
	,  750
	,  NULL
	,  tskUSART_PRIORITY
	,  NULL );
	
}
void usart1_init(void)
{
	
	static const gpio_map_t USART_GPIO_MAP =
	{
		{APP_USART_RX_PIN, APP_USART_RX_FUNCTION},
		{APP_USART_TX_PIN, APP_USART_TX_FUNCTION},
		{APP_USART_CTS_PIN, APP_USART_CTS_FUNCTION},
		{APP_USART_RTS_PIN, APP_USART_RTS_FUNCTION}
			
	};

	// USART options.
	static const usart_options_t USART_OPTIONS =
	{
		.baudrate     = 19200,//115200,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};

	// Assign GPIO to USART.
	gpio_enable_module(USART_GPIO_MAP,
	sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	
	
	Disable_global_interrupt();
	
	INTC_register_interrupt(&usart1_int_handler, APP_USART_IRQ,
	AVR32_INTC_INT0);
	
	// Enable USART CTS interrupt.
	APP_USART->ier = AVR32_USART_IER_CTSIC_MASK;
	// Enable USART Rx interrupt.
	EXAMPLE_USART->ier = AVR32_USART_IER_RXRDY_MASK;
	
	Enable_global_interrupt();
	
	// Initialize USART in hardware handshaking mode.
	usart_init_hw_handshaking(APP_USART, &USART_OPTIONS, APP_USART_CLOCK_MASK);
	
	//gpio_enable_gpio_pin(APP_USART_RTS_PIN);
	DISENABLE_PEER_SEND_DATA;//Ĭ��Ϊ�ߣ��������Զ˷������ݡ�
	
	
	
}


void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len)
{
	
	#if 1

	if(payload_len == 0)//��Ҫ���Ϊ���datasessionָ������������
	{
		mylog("payload_len empty!!!\n");
		return;
	}
	
	U32 q= payload_len/8;
	U32 r= payload_len%8;
	U32 counts=0;
	if(r!=0)
	{
		counts =q+2;//��һ��ΪЭ����Ϣ���������粻��8bytes������Ҫ��ȫ
	}
	else
	{
		counts =q+1;//��8�����ݰ�
	}
	if(counts>MAX_CSBK_UNIT)//��Ҫ���Ϊ���datasessionָ������������
	{
		mylog("usart_payload beyond MAX_CSBK_UNIT\n");
		return;
	}
	
	CSBK_Pro_t * csbk_t_array_ptr=(CSBK_Pro_t*)pvPortMalloc(counts*sizeof(CSBK_Pro_t));//��̬����ѿռ�����
	if(csbk_t_array_ptr==NULL)
	{
		mylog("pvPortMalloc failure!!!\n");
	}
	else
	memset(csbk_t_array_ptr, 0x00, counts*sizeof(CSBK_Pro_t));
	
	U32 remaining_len =payload_len;
	U32 idx =0;
	U32 data_ptr_index=0;

	//���CSBK����
	//��һ�����ݷ���Э����Ϣ
	csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
	csbk_t_array_ptr[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
	csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
	//���Ƿ���У������ݣ�δ����ֶ�Ĭ������Ϊ0��
	csbk_t_array_ptr[idx].csbk_data[0] = payload_len & 0xff;//���ݳ��ȵ��ֽ�
	csbk_t_array_ptr[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////���ݳ��ȸ��ֽڣ���Ĭ�����ݳ���˫�ֽ����65535
	
	
	do//���������ݴ����CSBK���ݵ��м�����ݺ����һ������
	{
		idx++;
		csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
		csbk_t_array_ptr[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
		csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
		
		if(remaining_len < CSBK_Payload_Length)//������8���ֽ�
		{
			csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//�������ݵ����һ��
			memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//����CSBK����
			remaining_len =0;//����ʣ�����ݳ��ȣ����˳�ѭ��
		}
		else//����
		{
			memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//����CSBK����
			data_ptr_index+=CSBK_Payload_Length;//��ַָ��ƫ��
			remaining_len-=CSBK_Payload_Length;//ʣ�����ݳ���
			if(remaining_len == 0)
			{
				csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//�������ݵ����һ��
			}
			else
			{
				csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			}
		}
		
	} while (remaining_len!=0);
	
	mylog("send csbk_ptr data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	
	xcmp_data_session_csbk_raw_req(csbk_t_array_ptr, sizeof(CSBK_Pro_t)*(idx+1), 3);//���һ��ֻ�ܷ���22��csbk���ݰ�

	vPortFree(csbk_t_array_ptr);
	csbk_t_array_ptr=NULL;

	#else


	CSBK_Pro_t csbk_t_array[100];//һ��xcmpָ��������ӵ��100��csbk��pakage
	memset(csbk_t_array, 0x00, sizeof(csbk_t_array));
	U32 remaining_len =payload_len;
	U32 idx =0;
	U32 data_ptr_index=0;

	//���CSBK����
	//��һ�����ݷ���Э����Ϣ
	csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
	csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
	csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
	//���Ƿ���У������ݣ�δ����ֶ�Ĭ������Ϊ0��
	csbk_t_array[idx].csbk_data[0] = payload_len & 0xff;//���ݳ��ȵ��ֽ�
	csbk_t_array[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////���ݳ��ȸ��ֽڣ���Ĭ�����ݳ���˫�ֽ����65535
	

	do//���������ݴ����CSBK���ݵ��м�����ݺ����һ������
	{
		idx++;
		csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
		csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
		csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
		
		if(remaining_len < CSBK_Payload_Length)//������8���ֽ�
		{
			csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//�������ݵ����һ��
			memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//����CSBK����
			remaining_len =0;//����ʣ�����ݳ��ȣ����˳�ѭ��
		}
		else//����
		{
			memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//����CSBK����
			data_ptr_index+=CSBK_Payload_Length;//��ַָ��ƫ��
			remaining_len-=CSBK_Payload_Length;//ʣ�����ݳ���
			if(remaining_len == 0)
			{
				csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//�������ݵ����һ��
			}
			else
			{
				csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			}
		}
		
	} while (remaining_len!=0);

	mylog("send csbk_t data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	
	xcmp_data_session_csbk_raw_req(csbk_t_array, sizeof(CSBK_Pro_t)*(idx+1), 3);
	
	#endif
}