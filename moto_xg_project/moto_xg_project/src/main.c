/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: mian.c
Author: Edwards
Version: 1.0.0.
Date: 2017/08/08 15:30:51

Description:
History:
*/

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "intc.h"
#include "timer.h"
#include "log.h"
#include "xcmp.h"
#include "xgrtc.h"
#include "app.h"


int main (void)
{
	
	//Force SSC_TX_DATA_ENABLE Disabled as soon as possible.
	AVR32_GPIO.port[1].ovrs  =  0x00000001;  //Value will be high.
	AVR32_GPIO.port[1].oders =  0x00000001;  //Output Driver will be Enabled.
	AVR32_GPIO.port[1].gpers =  0x00000001;  //Enable as GPIO.
		
	Disable_global_interrupt();
	local_start_pll0();
		
	INTC_init_interrupts();
		
	log_init();
	log("----start debug----");
		
	//voc_init();

	//tc_init();
	
	rfid_init();
		
	app_init();
		
	xcmp_init();

	local_start_timer();
	
	xg_rtc_init();
		
	vTaskStartScheduler();
	return 0;
	
}

	

