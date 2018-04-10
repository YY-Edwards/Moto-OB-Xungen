/*
 * myusart.c
 *
 * Created: 2018/4/1 11:27:25
 *  Author: Edwards
 */ 

#include "myusart.h"

volatile xSemaphoreHandle xCountingSem=NULL;
volatile xSemaphoreHandle xcsbk_rx_finished_Sem=NULL;
volatile xQueueHandle usart1_rx_xQueue=NULL;
volatile xQueueHandle usart1_tx_xQueue=NULL;
volatile U8 peer_rx_status_flag = 0;//默认情况下，peer是处于发送状态，mcu是处于接收状态。


//------------------------------------------------------------------------------
/*! \name ISO7816 Control Functions
 */
//! @{

/*! \brief Enables the ISO7816 receiver.
 *
 * \param usart   Base address of the USART instance.
 */
void usart_enable_receiver(volatile avr32_usart_t *usart)
{
  usart->cr = AVR32_USART_CR_RXEN_MASK;
  ENABLE_PEER_SEND_DATA;
  
}

/*! \
 *
 * The ISO7816 receiver is disabled.
 *
 * \param usart   Base address of the USART instance.
 */
void usart_disable_receiver(volatile avr32_usart_t *usart)
{
	DISENABLE_PEER_SEND_DATA;
	usart->cr = AVR32_USART_CR_RXDIS_MASK;
}

//! @}




static int usart1_test_cts_ic(volatile avr32_usart_t *usart, volatile U32 *cst_status)
{
	static int temp =0x00000000;
	temp = (usart->csr & (AVR32_USART_CSR_CTSIC_MASK | AVR32_USART_CSR_CTS_MASK));//读取一次csr寄存器，则会自动清除cstic的标志，所以要一起读回cst端口值
	*cst_status = temp & AVR32_USART_CSR_CTS_MASK;//获取cts端口的输入电平
	return (temp & AVR32_USART_CSR_CTSIC_MASK);/*返回cts中断标志状态，判断是否发生中断事件*/
	
}

static int usart1_test_cts_status(volatile avr32_usart_t *usart)
{
  return (usart->csr & AVR32_USART_CSR_CTS_MASK);
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
	static portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	static  portBASE_TYPE queue_ret = pdPASS;
	static int read_ret = USART_RX_EMPTY;
	static int c =0;
	static char rx_char =0;
	static int cts_status =0;
	int remaining_bytes =0;
	//对B设备的发送（A设备接收）来说，如果A设备接收缓冲快满的时发出RTS 信号（意思通知B设备停止发送），
	//B设备通过CTS 检测到该信号，停止发送；一段时间后A设备接收缓冲有了空余，发出RTS 信号，指示B设备开始发送数据。
	//A设备发（B设备接收）类似。
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
	
	//CTS中断
	if(usart1_test_cts_ic(APP_USART, &cts_status))
	{
		//cts_status = usart1_test_cts_status(APP_USART);
		if (cts_status)//0->1，模块处于发送状态，等待空闲标识
		{
			if(peer_rx_status_flag)
			{
				peer_rx_status_flag=0;
				ENABLE_PEER_SEND_DATA;//使能RTS端口
			}
			
		}
		else
		//1->0，模块向MCU发送数据完毕标志，则模块处于空闲接收状态，MCU可以先请求发送.
		{
			if(!peer_rx_status_flag)
			{
				peer_rx_status_flag=1;
				remaining_bytes = uxQueueMessagesWaitingFromISR(usart1_rx_xQueue);//获取队列有效数据个数
				if(remaining_bytes)//如果队列不为空，则触发信号
				{
					xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );
					usart_disable_receiver(APP_USART);
					//禁止peer发送数据；
				}
			}
			
		}
	
	}
	
	//rx中断
	read_ret = usart_read_char(APP_USART, &c);
	if(read_ret == USART_SUCCESS)//成功读取一个字节数据
	{
		rx_char = c;
		queue_ret = xQueueSendToBackFromISR(usart1_rx_xQueue, &rx_char, &xHigherPriorityTaskWoken);//insert data
		remaining_bytes = uxQueueMessagesWaitingFromISR(usart1_rx_xQueue);//获取队列有效数据个数
		if(remaining_bytes>=MAX_USART_RX_QUEUE_DEEP)//可能触发了信号量，但是任务不是最高级别，因此可能延迟响应，那么此时若继续中断，那么会再次触发信号，因为个数在不断递增
		{
			xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );//触发信号量，通知打包数据并发送到radio
			usart_disable_receiver(APP_USART);
			//禁止peer发送数据；理论上，peer每次发送前需要检查自己的CTS端口是否拉低
		}
	}
	
	
}


volatile  portTickType usart1_task_water_value =0;
static void usart1_rx_data_task(void * pvParameters)
{
	static int ret =USART_RX_EMPTY;
	static  portBASE_TYPE queue_ret = pdPASS;
	int malloc_size =MAX_USART_RX_QUEUE_DEEP;
	char *usart_temp_ptr=(char *)pvPortMalloc(malloc_size);//动态分配堆空间数据； 最大不超过300bytes
	if(usart_temp_ptr==NULL)
	{
		mylog("pvPortMalloc  usart_temp_ptr failure!!!\n");
	}
	else
	{
		memset(usart_temp_ptr, 0x00, malloc_size);//清空
	}
	
	//允许模块向MCU发送数据，并拉低RTS信号
	usart_enable_receiver(APP_USART);
	static U32 index =0;
	static int rx_char =0;
	
	while(1)
	{
		usart1_task_water_value = uxTaskGetStackHighWaterMark(NULL);
		//mylog("usart1 task water_value: %d\n", usart1_task_water_value);
		
		if(xSemaphoreTake(xCountingSem, (200*2) / portTICK_RATE_MS) == pdTRUE)
		{
			mylog("xSemaphoreTake xCountingSem  okay!\n");
			index =0;//偏移量清零
			do 
			{
				rx_char = 0;
				if(index == malloc_size)break;//防止数据拷贝越界
				queue_ret = xQueueReceive(usart1_rx_xQueue, (usart_temp_ptr+index), 0);//注意：先进先出
				if(queue_ret == pdPASS)//队列不为空
				{
					index++;
				}
				
			} while (queue_ret == pdPASS);
			
			//允许模块向MCU发送数据，并拉低RTS信号
			usart_enable_receiver(APP_USART);
			
			if(index!=0)//有数据则打包发送
			{	
			
				int remaining_send_bytes =index;//需要发送的数据长度
				int offset =0;
					
				do 
				{
					if (remaining_send_bytes<=MAX_CSBK_PACKAGE_DEEP)
					{
						package_usartdata_to_csbkdata((usart_temp_ptr+offset), remaining_send_bytes);
						remaining_send_bytes = 0;
					}
					else//整包150bytes发送
					{
						package_usartdata_to_csbkdata((usart_temp_ptr+offset), MAX_CSBK_PACKAGE_DEEP);
						remaining_send_bytes -= MAX_CSBK_PACKAGE_DEEP;	
						offset +=MAX_CSBK_PACKAGE_DEEP;
					}
						
				} while (remaining_send_bytes>0);
			
				memset(usart_temp_ptr, 0x00, malloc_size);//清空
				
				mylog("send csbk data okay...\n");
				
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


void usart1_send_char(U8 c)
{
	static  portTickType xLastWakeTime;
	xLastWakeTime = xTaskGetTickCount();
	while(gpio_pin_is_high(APP_USART_CTS_PIN))//peer is sending
	{
		vTaskDelayUntil( &xLastWakeTime, (250*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
	}
	
	//if(peer_rx_status_flag)
	if(1)
	{
		usart_putchar(APP_USART, c);
	}
	else
	{
		mylog("peer busy!!!\n");
	}
	
}

void third_party_interface_init(void)
{
	//创建一个计算信号量，以记录第三方接口发送数据完成的中断事件。
	xCountingSem = xSemaphoreCreateCounting( 20, 0 );
	if(xCountingSem==NULL)
	{
		mylog("create xCountingSem failure!!!\n");
	}
	
	xcsbk_rx_finished_Sem = xSemaphoreCreateCounting( 20, 0 );
	if(xcsbk_rx_finished_Sem==NULL)
	{
		mylog("create xcsbk_rx_finished_Sem failure!!!\n");
	}
	
	/*initialize the queue*/
	usart1_rx_xQueue = xQueueCreate((MAX_USART_RX_QUEUE_DEEP+200), sizeof(char));//最大缓冲500
	if(usart1_rx_xQueue==NULL)
	{
		mylog("create usart1_rx_xQueue failure!!!\n");
	}
	
	usart1_tx_xQueue = xQueueCreate((MAX_USART_TX_QUEUE_DEEP), sizeof(char));//最大缓冲200
	if(usart1_tx_xQueue==NULL)
	{
		mylog("create usart1_tx_xQueue failure!!!\n");
	}
	
	
	usart1_init();
	
	portBASE_TYPE res = xTaskCreate(
	usart1_rx_data_task
	,  (const signed portCHAR *)"usart_rx"
	,  1200
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
		.baudrate     = 115200,//115200,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};

	// Assign GPIO to USART.
	gpio_enable_module(USART_GPIO_MAP,
	sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));
	
	
	//gpio_enable_gpio_pin(APP_USART_RTS_PIN);
	//DISENABLE_PEER_SEND_DATA;//默认为高，不允许对端发送数据。
	
	// Initialize USART in hardware handshaking mode.
	usart_init_hw_handshaking(APP_USART, &USART_OPTIONS, USART1_TARGET_PBACLK_FREQ_HZ);
	
	Disable_global_interrupt();
	
	INTC_register_interrupt(&usart1_int_handler, APP_USART_IRQ,
	AVR32_INTC_INT0);
	
	// Enable USART CTS interrupt.Enable USART Rx interrupt.
	APP_USART->ier = AVR32_USART_IER_CTSIC_MASK | AVR32_USART_IER_RXRDY_MASK;
	
	Enable_global_interrupt();
	
	usart_disable_receiver(APP_USART);//失能USART1接收,流控RTS默认为高，指示不允许对端发送数据。

}

extern volatile char radio_numb_array[1200];
extern volatile unsigned char host_flag;
void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len)
{
	
	#if 1

	if(payload_len == 0)//需要拆分为多个datasession指令来发送数据
	{
		mylog("payload_len empty!!!\n");
		return;
	}
	
	U32 q= payload_len/8;
	U32 r= payload_len%8;
	U32 counts=0;
	if(r!=0)
	{
		counts =q+2;//第一包为协议信息包，另外如不满8bytes数据需要不全
	}
	else
	{
		counts =q+1;//整8个数据包
	}
	if(counts>MAX_CSBK_UNIT)//需要拆分为多个datasession指令来发送数据
	{
		mylog("usart_payload beyond MAX_CSBK_UNIT\n");
		return;
	}
	
	CSBK_Pro_t * csbk_t_array_ptr=(CSBK_Pro_t*)pvPortMalloc(counts*sizeof(CSBK_Pro_t));//动态分配堆空间数据
	if(csbk_t_array_ptr==NULL)
	{
		mylog("pvPortMalloc failure!!!\n");
	}
	else
	memset(csbk_t_array_ptr, 0x00, counts*sizeof(CSBK_Pro_t));
	
	U32 remaining_len =payload_len;
	U32 idx =0;
	U32 data_ptr_index=0;
	U8 custom_csbk_opcode =0;
	if(host_flag)//compare label
	{
		custom_csbk_opcode = CSBK_Host_Opcode;//host->slave
	}
	else
	{
		custom_csbk_opcode = CSBK_Slave_Opcode;//slave->host
	}
	my_custom_pro_t custom_pro;
	memset(&custom_pro, 0x00, sizeof(custom_pro));//clear buff
	//memcpy(custom_pro.header, FIXED_HEADER, sizeof(FIXED_HEADER));
	custom_pro.header = FIXED_HEADER;
	custom_pro.data_len[0] = payload_len & 0xff;//数据长度低字节
	custom_pro.data_len[1] = ((payload_len >> 8) &0x00ff);////数据长度高字节，且默认数据长度双字节最大65535
	
	//打包CSBK数据
	//第一包数据放置协议信息,具有保护块标志
	csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value-0x0，如果开启保护模式，则对端接收的数据中没有此保护块
	csbk_t_array_ptr[idx].csbk_header.csbk_opcode =custom_csbk_opcode;//
	csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value-0x20
	csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;//-0x0
	memcpy(csbk_t_array_ptr[idx].csbk_data, &custom_pro, sizeof(custom_pro));
	//考虑放入校验等数据，未填充字段默认设置为0；
	//csbk_t_array_ptr[idx].csbk_data[0] = payload_len & 0xff;//数据长度低字节
	//csbk_t_array_ptr[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////数据长度高字节，且默认数据长度双字节最大65535
	
	
	do//将负载数据打包到CSBK数据的中间包数据和最后一包数据
	{
		idx++;
		csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value-0x0
		csbk_t_array_ptr[idx].csbk_header.csbk_opcode =custom_csbk_opcode;//
		csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
		
		if(remaining_len < CSBK_Payload_Length)//不超过8个字节
		{
			csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包-0x1
			memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//拷贝CSBK数据
			remaining_len =0;//清零剩余数据长度，并退出循环
		}
		else//整包
		{
			memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//拷贝CSBK数据
			data_ptr_index+=CSBK_Payload_Length;//地址指针偏移
			remaining_len-=CSBK_Payload_Length;//剩余数据长度
			if(remaining_len == 0)
			{
				csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			}
			else
			{
				csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			}
		}
		
	} while (remaining_len!=0);
	
	mylog("send csbk_ptr data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	
	//注意作为主机，此处需要实现一对多的功能。
	U32 dest_id =0;
	//U32 radio_counts = csbk_flash_get_radio_id_total();
	//U32 offset =0;
	static  portTickType xLastWakeTime;
	//xLastWakeTime = xTaskGetTickCount();
	//while(radio_counts>0)
	{	
		//dest_id = 
		//(((radio_numb_array[offset+3]<<24) & 0xff000000)
		//|((radio_numb_array[offset+2]<<16) & 0x00ff0000)
		//|((radio_numb_array[offset+1]<<8) & 0x0000ff00)
		//|(radio_numb_array[offset] & 0x000000ff));
		
		
		//On the receiving side, all the XCMP device (both OB and Non-IP peripheral) can
		//receive the CSBK broadcast /*so that no need for target IP address specified in the
		//sending side XCMP request.*/ After receiving each CSBK, MOTOTRBO Radio checks
		//the CRC error, removes the CRC field and then broadcasts to XCMP devices the
		//remaining bytes (CSBK without CRC) as the payload of Data Session XCMP command.
	
		//dest_id = 0x01;
		//mylog("send to: %d\n", dest_id);
		//csbk_raw相当于broadcast,不需要指定目标ID
		xcmp_data_session_csbk_raw_req(csbk_t_array_ptr, sizeof(CSBK_Pro_t)*(idx+1));//最多一次只能发送22个csbk数据包
		
		//offset +=RADIO_ID_NUMB_SIZE;
		//radio_counts--;
		//增加延时发送，以缓冲大量发送导致的缓冲队列长时间为空的情况
		//vTaskDelayUntil( &xLastWakeTime, (200*2) / portTICK_RATE_MS );//精确的以1000ms为周期执行。
	
	} 
	
	//xcmp_data_session_csbk_raw_req(csbk_t_array_ptr, sizeof(CSBK_Pro_t)*(idx+1));//最多一次只能发送22个csbk数据包

	vPortFree(csbk_t_array_ptr);
	csbk_t_array_ptr=NULL;

	#else


	CSBK_Pro_t csbk_t_array[100];//一个xcmp指令最大可以拥有100个csbk—pakage
	memset(csbk_t_array, 0x00, sizeof(csbk_t_array));
	U32 remaining_len =payload_len;
	U32 idx =0;
	U32 data_ptr_index=0;

	//打包CSBK数据
	//第一包数据放置协议信息
	csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
	csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
	csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
	//考虑放入校验等数据，未填充字段默认设置为0；
	csbk_t_array[idx].csbk_data[0] = payload_len & 0xff;//数据长度低字节
	csbk_t_array[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////数据长度高字节，且默认数据长度双字节最大65535
	

	do//将负载数据打包到CSBK数据的中间包数据和最后一包数据
	{
		idx++;
		csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
		csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
		csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
		
		if(remaining_len < CSBK_Payload_Length)//不超过8个字节
		{
			csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//拷贝CSBK数据
			remaining_len =0;//清零剩余数据长度，并退出循环
		}
		else//整包
		{
			memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//拷贝CSBK数据
			data_ptr_index+=CSBK_Payload_Length;//地址指针偏移
			remaining_len-=CSBK_Payload_Length;//剩余数据长度
			if(remaining_len == 0)
			{
				csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			}
			else
			{
				csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			}
		}
		
	} while (remaining_len!=0);

	mylog("send csbk_t data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	
	xcmp_data_session_csbk_raw_req(csbk_t_array, sizeof(CSBK_Pro_t)*(idx+1));
	
	#endif
}