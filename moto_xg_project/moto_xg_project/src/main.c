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
#include "log.h"
#include "FreeRTOS.h"
#include "task.h"
#include "intc.h"
#include "timer.h"
#include "xcmp.h"
#include "xgrtc.h"
//#include "xgflash.h"
#include "RFID.h"
#include "app.h"
//#include "myusart.h"

void avr_flash_test();
extern volatile U32 tc_tick;

int main (void)
{
	
	//Force SSC_TX_DATA_ENABLE Disabled as soon as possible.
	AVR32_GPIO.port[1].ovrs  =  0x00000001;  //Value will be high.
	AVR32_GPIO.port[1].oders =  0x00000001;  //Output Driver will be Enabled.
	AVR32_GPIO.port[1].gpers =  0x00000001;  //Enable as GPIO.
		
	Disable_global_interrupt();
	local_start_pll0();
	INTC_init_interrupts();
	
#if 0

	
	
	tc_init();
	
	start_my_timer();
	
	Enable_global_interrupt();
	
	while(1)
	{
		U32 count = 0;
		//if(tc_tick == 500)
		//{
			//count = 9;
		//}
		//for(U32 i = count; i<100; i++)
		//{
			avr_flash_test();
		//}
		//stop_my_timer();
		count = 0;
	}
	
#endif		
	log_init();
	log_debug("----start debug----");
	
	//avr_flash_test();
	
	//third_party_interface_init();//usart1
	
	//xg_flashc_init();
		
	//voc_init();

	//tc_init();
	
	//rfid_init();//csbk-ob：无此硬件接口,注意如果没有先开启flash模块，则需要单独初始化spi引脚接口
		
	app_init();
	
	xg_rtc_init();
		
	xcmp_init();

	local_start_timer();
		
	vTaskStartScheduler();
	return 0;
	
}

	

