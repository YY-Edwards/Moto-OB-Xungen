/*
 * timer.h
 *
 * Created: 2016/3/15 14:06:54
 *  Author: JHDEV2
 */ 


#ifndef TIMER_H_
#define TIMER_H_
#include "compiler.h"
#include "sysclk.h"
#include "pll.h"
#include "pm.h"
#include "flashc.h"

#define MAX_TIMERS 5
#define TIME_BASE_25MS 1
#define TIME_BASE_50MS 2
#define TIME_BASE_500MS 20

//Timer mappings for GOB
#define TIMER_TC_CHANNEL_ID         0
#define TIMER_TC_CLK_PIN            AVR32_TC_CLK0_0_1_PIN
#define TIMER_TC_CLK_FUNCTION       AVR32_TC_CLK0_0_1_FUNCTION
#define TIMER_TC_FS_PIN             AVR32_TC_B0_0_0_PIN
#define TIMER_TC_FS_FUNCTION        AVR32_TC_B0_0_0_FUNCTION
#define TIMER_EN_CLK_PIN            AVR32_TC_A0_0_0_PIN
#define TIMER_EN_CLK_FUNCTION       AVR32_TC_A0_0_0_FUNCTION

typedef void (*handler)(void*);  

typedef struct
{
	unsigned int	count;
	unsigned int	resetCount;
	handler			timerHandler;
	void *          param;//function parameter pointer.
} timer_info;

void tc_init(void);
void start_my_timer(void);
void stop_my_timer(void);
void local_start_pll0(void);
void local_start_timer(void);
void delay_ns(U32 ns);
void delay_us(U32 us);
void delay_ms(U32 ms);
void wait_10_ms(void);
void setTimer(unsigned char timer, unsigned int delay, unsigned char rearm, handler timehandler, void *param);
unsigned long get_system_time(void);




#endif /* TIMER_H_ */