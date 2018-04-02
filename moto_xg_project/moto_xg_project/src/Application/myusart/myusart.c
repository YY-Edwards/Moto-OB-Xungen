/*
 * myusart.c
 *
 * Created: 2018/4/1 11:27:25
 *  Author: Edwards
 */ 

#include "myusart.h"

volatile U8 peer_rx_status_flag = 0;//默认情况下，peer是处于发送状态，mcu是处于接收状态。

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
	if (usart1_test_cts_status() == 1)//0->1，模块向MCU发送数据完毕标志，则模块处于接收状态，MCU可以先请求发送.
	{
		peer_rx_status_flag=1;
		xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );
		DISENABLE_PEER_SEND_DATA;//禁止peer发送数据；
	} 
	else//1->0
	{
		peer_rx_status_flag=0;
	}
	
	//rx中断
	read_ret = usart_read_char(APP_USART, &c);
	if(read_ret == USART_SUCCESS)//成功读取一个字节数据
	{
		queue_ret = xQueueSendToBackFromISR(usart1_rx_xQueue, &c, 0);//insert data
		int remaining_bytes =0;
		remaining_bytes = uxQueueMessagesWaitingFromISR(usart1_rx_xQueue);//获取队列有效数据个数
		if(remaining_bytes>=150)//可能触发了信号量，但是任务不是最高级别，因此可能延迟响应，那么此时若继续中断，那么会再次触发信号，因为个数在不断递增
		{
			xSemaphoreGiveFromISR( xCountingSem, &xHigherPriorityTaskWoken );//触发信号量，通知打包数据并发送到radio
			DISENABLE_PEER_SEND_DATA;//禁止peer发送数据；理论上，peer每次发送前需要检查RST端口是否拉低
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
	char *usart_temp_ptr=(char *)pvPortMalloc(malloc_size);//动态分配堆空间数据； 最大不超过150bytes
	if(usart_temp_ptr==NULL)
	{
		mylog("pvPortMalloc  usart_temp_ptr failure!!!\n");
	}
	else
	{
		memset(usart_temp_ptr, 0x00, malloc_size)://清空
	}

	
	ENABLE_PEER_SEND_DATA;//允许模块向MCU发送usart数据
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
			
			if(index!=0)//有数据则打包发送
			{	
				package_usartdata_to_csbkdata(usart_temp_ptr, index);
				memset(usart_temp_ptr, 0x00, malloc_size)://清空
				
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
	//创建一个计算信号量，以记录第三方接口发送数据完成的中断事件。
	xCountingSem = xSemaphoreCreateCounting( 20, 0 );
	if(xCountingSem==NULL)
	{
		mylog("create xCountingSem failure!!!\n");
	}
	
	/*initialize the queue*/
	usart1_rx_xQueue = xQueueCreate(200, sizeof(char));//最大缓冲
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
	DISENABLE_PEER_SEND_DATA;//默认为高，不允许对端发送数据。
	
	
	
}
