/*
 * rtc.h
 *
 * Created: 2015/10/29 星期四 下午 17:05:54
 *  Author: Administrator
 */ 

#ifndef RTC_H_
#define RTC_H_

#define PCF8563_ADDRESS        0x51					// PCF8563's TWI address//1010 001x----x101 0001

#define PCF8563_R_ADDRESS      0xA3            // PCF8563's Read TWI address
#define PCF8563_W_ADDRESS      0xA2            // PCF8563's Write TWI address
#define PCF8563_ADDR_LGT       1				// Address length of the PCF8563 memory

#define TWI_SPEED          200000				// Speed of TWI

#define RTC_PBACLK_FREQ_HZ 24000000	//24Mhz


#define now getTime



typedef enum
{
	rtc_success = 0,
	rtc_init_err,
	rtc_write_err,	
	mutex_null,
}rtc_err_t;


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

/*initialize TWI(PCF8563) diver*/
rtc_err_t rtc_init(void);

/*set time*/
rtc_err_t rtc_set_time(date_time_t * t);

/*read time*/
rtc_err_t rtc_read_time(date_time_t * t);


/***
更新时间
@功能：刷新系统时间RTC时间

@参数：无

@返回值:0成功，1失败
*/



date_time_t * getTime(void);
void local_start_pll0(void);
#endif /* RTC_H_ */