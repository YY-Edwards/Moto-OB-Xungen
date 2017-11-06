/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: ssc.c
Author: wxj
Version: 1.0.0.02
Date: 2015/10/29 17:25:19

Description:
History:
*/

#include <avr32/io.h>
#include <string.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "rtc.h"

#include "pm.h"
#include "twi.h"
#include "gpio.h"
#include "flashc.h"

static volatile date_time_t date_time;

static volatile xSemaphoreHandle rtc_mutex = NULL;
/**
Function: local_start_pll0
Description: initialize TWI(PCF8563) diver
Calls: 
    PCF8563_init
Called By: ..
Return:rtc_err_t
*/


/**
Function: rtc_init
Description: initialize TWI(PCF8563) diver
Calls: 
    PCF8563_init
Called By: ..
Return:rtc_err_t
*/
rtc_err_t rtc_init(void)
{
	/* Create the mutex semaphore to guard a shared RTC(TWI).*/	
	rtc_mutex = xSemaphoreCreateMutex();
	
	if(NULL != rtc_mutex)
	{
		/*See if we can obtain the semaphore. If the semaphore is not available wait aways to see if it becomes free*/
		xSemaphoreTake( rtc_mutex, portMAX_DELAY);
	}
	else
	{
		return mutex_null;
	}
	
	/*able to obtain the semaphore and can now access the shared RTC(TWI)*/	
				
	static const gpio_map_t TWI_GPIO_MAP =
	{		
		{AVR32_TWI_SDA_0_0_PIN, AVR32_TWI_SDA_0_0_FUNCTION},
		{AVR32_TWI_SCL_0_0_PIN, AVR32_TWI_SCL_0_0_FUNCTION}		
	};
	
	twi_options_t opt;
	
	/*twi_package_t packet, packet_received*/
	static int status;

	/*TWI gpio pins configuration*/	
	gpio_enable_module(TWI_GPIO_MAP, sizeof(TWI_GPIO_MAP) / sizeof(TWI_GPIO_MAP[0]));
	
	/* options settings*/
	opt.pba_hz = RTC_PBACLK_FREQ_HZ;//FOSC0;24Mhz
	opt.speed  = TWI_SPEED;//200Khz
	opt.chip   = PCF8563_ADDRESS;

	/*initialize TWI driver with options*/
	status = twi_master_init(&AVR32_TWI, &opt);
	
	/*finished accessing the shared resource.Release the semaphore.*/
	xSemaphoreGive(rtc_mutex);		
		
	if (status == TWI_SUCCESS)
	{
		return rtc_success;
	}
		
	return rtc_init_err;
}

/**
Function: write_a_byte
Description: send a byte form twi
Calls: 
Return:U32
*/
uint32_t  write_a_byte(uint32_t subaddress, uint32_t  datatosend)
{
	U32 TWI_Status = 0;
	
	AVR32_TWI.cr =   AVR32_TWI_CR_MSEN_MASK | AVR32_TWI_CR_SVDIS_MASK;
	AVR32_TWI.mmr =  PCF8563_ADDRESS        << AVR32_TWI_MMR_DADR_OFFSET   |
	PCF8563_ADDR_LGT       << AVR32_TWI_MMR_IADRSZ_OFFSET |
	0                      << AVR32_TWI_MMR_MREAD_OFFSET;
	AVR32_TWI.iadr =  subaddress;

	AVR32_TWI.thr = datatosend;  //Higher order bits discarded.

	do
	{
		TWI_Status =  AVR32_TWI.sr & 0x00000104;
	}
	while (TWI_Status == 0);
	while ((AVR32_TWI.sr & 0x00000001) == 0x00000000); //Wait for complete.
	return (TWI_Status);
}

/**
Function: read_a_byte
Description: read a byte form twi
Calls: 
Return:U32
*/
U32 read_a_byte(U32 subaddress, S8 *datareceived)
{
	U32 TWI_Status = 0;

	AVR32_TWI.cr   =  AVR32_TWI_CR_MSEN_MASK | AVR32_TWI_CR_SVDIS_MASK;
	AVR32_TWI.mmr  =  PCF8563_ADDRESS        << AVR32_TWI_MMR_DADR_OFFSET   |
	PCF8563_ADDR_LGT		<< AVR32_TWI_MMR_IADRSZ_OFFSET |
	1					<< AVR32_TWI_MMR_MREAD_OFFSET;
	AVR32_TWI.iadr =  subaddress;

	AVR32_TWI.cr   =  AVR32_TWI_START_MASK | AVR32_TWI_STOP_MASK;

	do
	{
		TWI_Status =  AVR32_TWI.sr & 0x00000102;
	}
	while (TWI_Status == 0);

	if (!(TWI_Status & 0x00000100))
	{
		*datareceived = AVR32_TWI.rhr;
	}
	while ((AVR32_TWI.sr & 0x00000001) == 0x00000000); //Wait for complete.
	return (TWI_Status);
}

/**
Function: rtc_set_time
Description: set time
Calls: 
    my_writeabyte
Return:rtc_err_t
*/
rtc_err_t rtc_set_time(date_time_t * t)
{	
	uint32_t res = 0;
		
	if(NULL != rtc_mutex)
	{
		/*See if we can obtain the semaphore. If the semaphore is not available wait aways to see if it becomes free*/
		xSemaphoreTake( rtc_mutex, portMAX_DELAY);
	}
	else
	{
		return mutex_null;
	}
	
	/*able to obtain the semaphore and can now access the shared RTC(TWI)*/	
	/*stop PCF8563*/
	res |= write_a_byte(0x00, 0x20);
	
	/*write date time in BCD code*/		
	res |= write_a_byte(0x02,((t->second / 10) << 4) + t->second % 10);
	res |= write_a_byte(0x03,((t->minute / 10) << 4) + t->minute % 10);
	res |= write_a_byte(0x04,((t->hour / 10) << 4) + t->hour % 10);
	res |= write_a_byte(0x05,((t->day / 10) << 4) + t->day % 10);
		
	res |= write_a_byte(0x06,((t->week / 10) << 4) + t->week % 10);
	res |= write_a_byte(0x07,((t->month / 10) << 4) + t->month % 10);
	res |= write_a_byte(0x08,(((t->year-2000) / 10) << 4) + (t->year-2000) % 10);
	
	/*start PCF8563*/	
	res |= write_a_byte(0x00, 0x00);
	
	/*finished accessing the shared resource.Release the semaphore.*/
	xSemaphoreGive(rtc_mutex);	
	
	if(res)
	{
		return rtc_success;
	}

	return rtc_write_err;	
}

/**
Function: rtc_read_time
Description: read time
Calls: 
    my_writeabyte
Return:rtc_err_t
*/
rtc_err_t rtc_read_time(date_time_t * t)
{	
	if(NULL != rtc_mutex)
	{
		/*See if we can obtain the semaphore. If the semaphore is not available wait aways to see if it becomes free*/
		xSemaphoreTake( rtc_mutex, portMAX_DELAY);
	}
	else
	{
		return mutex_null;
	}
	
	/*able to obtain the semaphore and can now access the shared RTC(TWI)*/	
	uint32_t res = 0;
	int RTC_Status;
	
	unsigned char time[8];
	
	/*read second*/
	res = read_a_byte(0x02, &time[0]);	
	t->second = ((time[0] & 0x7F) >> 4) * 10 + (time[0] & 0x0F);
	
	/*read minute*/
	res = read_a_byte(0x03, &time[1]);
	t->minute = ((time[1] & 0x7F) >> 4) * 10 + (time[1] & 0x0F);
	
	/*read hour*/
	res = read_a_byte(0x04, &time[2]);
	t->hour = ((time[2] & 0x3F) >> 4) * 10 + (time[2] & 0x0F);
	
	/*read day*/
	res = read_a_byte(0x05, &time[3]);
	t->day = ((time[3] & 0x3F) >> 4) * 10 + (time[3] & 0x0F);
	
	/*read week*/
	res = read_a_byte(0x06, &time[4]);
	t->week = (time[4] & 0x0F);
	
	/*read month*/
	res = read_a_byte(0x07, &time[5]);
	t->month = ((time[5] & 0x1F) >> 4) * 10 + (time[5] & 0x0F);
	
	/*read year*/
	res = read_a_byte(0x08, &time[6]);
	t->year = ((time[6] & 0xFF) >> 4) * 10 + (time[6] & 0x0F) + 2000;
	
	/*finished accessing the shared resource.Release the semaphore.*/	
	xSemaphoreGive(rtc_mutex);	
		
	if(res)
	{
		return rtc_success;
	}

	return rtc_write_err;
}


/**
Function: getTime
Description: define now
Calls:
Return:date_time_t *
*/
date_time_t * getTime(void)
{
	rtc_read_time(&date_time);
	return &date_time;
}