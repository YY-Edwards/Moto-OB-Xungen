/*
 * myusart.c
 *
 * Created: 2018/4/1 11:27:25
 *  Author: Edwards
 */ 

#include "myusart.h"

volatile U8 peer_allow_send_flag = 0;

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
	//int c;
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
	
	if (usart1_test_cts_status() == 1)//0->1
	{
		peer_allow_send_flag=0;
	} 
	else//1->0
	{
		peer_allow_send_flag=1;
	}
	
	
	
}


void third_party_interface_init(void)
{
	
	usart1_init();
	
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
	
	Enable_global_interrupt();
	
	// Initialize USART in hardware handshaking mode.
	usart_init_hw_handshaking(APP_USART, &USART_OPTIONS, APP_USART_CLOCK_MASK);
	
	//gpio_enable_gpio_pin(APP_USART_RTS_PIN);
	DISENABLE_PEER_SEND_DATA;//默认为高，不允许对端发送数据。
	
	
	
}
