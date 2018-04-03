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
#  define MAX_USART_RX_QUEUE_DEEP      300//bytes
#  define MAX_CSBK_PACKAGE_DEEP        150//bytes

#define ENABLE_PEER_SEND_DATA		gpio_clr_gpio_pin(APP_USART_RTS_PIN)
#define DISENABLE_PEER_SEND_DATA	gpio_set_gpio_pin(APP_USART_RTS_PIN)

void third_party_interface_init(void);
void usart1_init(void);
void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len);


#endif /* MYUSART_H_ */