/*
 * myusart.h
 *
 * Created: 2018/4/1 11:27:11
 *  Author: Edwards
 */ 


#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "log.h"
#include "usart.h"
#include "gpio.h"
#include "xcmp.h"
#include "string.h"
#include "xgflash.h"


#ifndef MYUSART_H_
#define MYUSART_H_

#  define APP_USART						(&AVR32_USART1)
#  define APP_USART_RX_PIN				AVR32_USART1_RXD_0_1_PIN
#  define APP_USART_RX_FUNCTION			AVR32_USART1_RXD_0_1_FUNCTION
#  define APP_USART_TX_PIN				AVR32_USART1_TXD_0_1_PIN
#  define APP_USART_TX_FUNCTION			AVR32_USART1_TXD_0_1_FUNCTION
#  define APP_USART_CTS_PIN				AVR32_USART1_CTS_0_1_PIN
#  define APP_USART_CTS_FUNCTION		AVR32_USART1_CTS_0_1_FUNCTION
#  define APP_USART_RTS_PIN				AVR32_USART1_RTS_0_1_PIN
#  define APP_USART_RTS_FUNCTION		AVR32_USART1_RTS_0_1_FUNCTION
#  define APP_USART_IRQ					AVR32_USART1_IRQ

#  define APP_USART_CLOCK_MASK      AVR32_USART1_CLK_PBA

#  define USART1_TARGET_PBACLK_FREQ_HZ 24000000  // PBA clock target frequency, in Hz
#  define MAX_USART_RX_QUEUE_DEEP      150//bytes
#  define MAX_USART_TX_QUEUE_DEEP      2048//bytes，扩大csbk接收队列深度
#  define MAX_CSBK_PACKAGE_DEEP        150//bytes

#define ENABLE_PEER_SEND_DATA		gpio_clr_gpio_pin(APP_USART_RTS_PIN)
#define DISENABLE_PEER_SEND_DATA	gpio_set_gpio_pin(APP_USART_RTS_PIN)

void third_party_interface_init(void);
void usart1_init(void);
void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len);
void usart1_send_char(U8 c);
void usart_disable_receiver(volatile avr32_usart_t *usart);
void usart_enable_receiver(volatile avr32_usart_t *usart);

#define FIXED_HEADER	0xABCD5A5A

#pragma pack(1)
//OB分布模式：一主多从
typedef struct
{			
	U32 header;
	//U8 header[4];//0xABCD5A5A
	U8 data_len[2];
	U8 unused[2];
	//U8 data_crc[2];没有实际作用，只需要保证空中数据流的传输
	
}my_custom_pro_t;
#pragma pack(0)


typedef enum {
	
	WAITING_CSBK_P_HEADER,
	READING_MIDDLE_FRAGMENT,
	WAITING_LAST_FRAGMENT
	
} csbk_rx_state_t;



#endif /* MYUSART_H_ */