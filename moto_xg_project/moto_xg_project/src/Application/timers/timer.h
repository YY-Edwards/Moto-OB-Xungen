/*
 * timer.h
 *
 * Created: 2016/3/15 14:06:54
 *  Author: JHDEV2
 */ 


#ifndef TIMER_H_
#define TIMER_H_

//Timer mappings for GOB
#define TIMER_TC_CHANNEL_ID         0
#define TIMER_TC_CLK_PIN            AVR32_TC_CLK0_0_1_PIN
#define TIMER_TC_CLK_FUNCTION       AVR32_TC_CLK0_0_1_FUNCTION
#define TIMER_TC_FS_PIN             AVR32_TC_B0_0_0_PIN
#define TIMER_TC_FS_FUNCTION        AVR32_TC_B0_0_0_FUNCTION
#define TIMER_EN_CLK_PIN            AVR32_TC_A0_0_0_PIN
#define TIMER_EN_CLK_FUNCTION       AVR32_TC_A0_0_0_FUNCTION

void tc_init(void);
void local_start_timer(void);

#endif /* TIMER_H_ */