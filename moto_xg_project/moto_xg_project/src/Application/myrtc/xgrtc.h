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
	U8 Year;			//年
	U8 Month;			//月
	U8 Day;				//日
	U8 Hour;			//时
	U8 Minute;			//分
	U8 Second;			//秒
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
Note: 本方法时间起点从2000-1-1 0:0:0开始,向后编码150年
*/
U32 RTC_EncodeTime(DateTime_t *DT);
void RTC_DecodeTime(U32 TimeData, DateTime_t *DT);



#endif /* XGRTC_H_ */