/*
 * xgrtc.h
 *
 * Created: 2017/11/6 14:03:50
 *  Author: Edwards
 */ 


#ifndef XGRTC_H_
#define XGRTC_H_

#include <avr32/io.h>
#if __GNUC__
#  include "intc.h"
#endif
#include "compiler.h"
#include "rtc/rtc.h"
#include "gpio.h"
#include "pm.h"
#include "log.h"
#include "timer.h"
#include "semphr.h"


typedef struct
{
	U8 Year;			//��
	U8 Month;			//��
	U8 Day;				//��
	U8 Hour;			//ʱ
	U8 Minute;			//��
	U8 Second;			//��
}DateTime_t;

#pragma  pack(1)
typedef struct{ //8bytes
	unsigned short int year;
	unsigned char month;
	unsigned char day;
	unsigned char week;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
}date_time_t;
#pragma  pack()

volatile DateTime_t Current_time;
static volatile DateTime_t temp_time;

char *print_i(char *str, unsigned int n);
void xg_rtc_init(void);
/*
Note: ������ʱ������2000-1-1 0:0:0��ʼ,������150��
*/
U32 RTC_EncodeTime(DateTime_t *DT);
void RTC_DecodeTime(U32 TimeData, DateTime_t *DT);



#endif /* XGRTC_H_ */