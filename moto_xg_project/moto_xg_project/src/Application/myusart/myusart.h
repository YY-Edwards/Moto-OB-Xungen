/*
 * myusart.h
 *
 * Created: 2018/4/1 11:27:11
 *  Author: Edwards
 */ 

#include <usart.h>

#ifndef MYUSART_H_
#define MYUSART_H_

#  define APP_USART						(&AVR32_USART1)
#  define APP_USART_RX_PIN				AVR32_USART1_RXD_0_0_PIN
#  define APP_USART_RX_FUNCTION			AVR32_USART1_RXD_0_0_FUNCTION
#  define APP_USART_TX_PIN				AVR32_USART1_TXD_0_0_PIN
#  define APP_USART_TX_FUNCTION			AVR32_USART1_TXD_0_0_FUNCTION
#  define APP_USART_CTS_PIN				AVR32_USART1_CTS_0_0_PIN
#  define APP_USART_CTS_FUNCTION		AVR32_USART1_CTS_0_0_FUNCTION
#  define APP_USART_RTS_PIN				AVR32_USART1_RTS_0_0_PIN
#  define APP_USART_RTS_FUNCTION		AVR32_USART1_RTS_0_0_FUNCTION
#  define APP_USART_IRQ					AVR32_USART1_IRQ

#  define APP_USART_CLOCK_MASK      AVR32_USART1_CLK_PBA

#  define USART1_TARGET_PBACLK_FREQ_HZ 24000000  // PBA clock target frequency, in Hz

#define ENABLE_PEER_SEND_DATA		gpio_clr_gpio_pin(APP_USART_RTS_PIN)
#define DISENABLE_PEER_SEND_DATA	gpio_set_gpio_pin(APP_USART_RTS_PIN)

void third_party_interface_init(void);
void usart1_init(void);





#endif /* MYUSART_H_ */