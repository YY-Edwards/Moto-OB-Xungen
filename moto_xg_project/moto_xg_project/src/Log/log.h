/*
 * log.h
 *
 * Created: 2015/11/24 9:34:34
 *  Author: JHDEV2
 */ 


#ifndef LOG_H_
#define LOG_H_

//#define _SAVE_LOG

#define _PRINT_LOG

#ifdef _PRINT_LOG

#include <usart.h>

#include "FreeRTOS.h"
#include "queue.h"

#  define EXAMPLE_USART                 (&AVR32_USART1)
#  define EXAMPLE_USART_RX_PIN          AVR32_USART1_RXD_0_0_PIN
#  define EXAMPLE_USART_RX_FUNCTION     AVR32_USART1_RXD_0_0_FUNCTION
#  define EXAMPLE_USART_TX_PIN          AVR32_USART1_TXD_0_0_PIN
#  define EXAMPLE_USART_TX_FUNCTION     AVR32_USART1_TXD_0_0_FUNCTION
#  define EXAMPLE_USART_CLOCK_MASK      AVR32_USART1_CLK_PBA
#  define EXAMPLE_PDCA_CLOCK_HSB        AVR32_PDCA_CLK_HSB
#  define EXAMPLE_PDCA_CLOCK_PB         AVR32_PDCA_CLK_PBA
#  define EXAMPLE_TARGET_MCUCLK_FREQ_HZ 48000000  // MCU clock target frequency, in Hz
#  define EXAMPLE_TARGET_PBACLK_FREQ_HZ 24000000  // PBA clock target frequency, in Hz

#endif


extern xQueueHandle logQueue;
int mylog(char * content, ...);
int logFromISR(char * content, ...);

#ifdef _PRINT_LOG
void log_init(void);

#endif

#endif /* LOG_H_ */