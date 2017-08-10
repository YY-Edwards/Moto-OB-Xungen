/*
 * RFID.c
 *
 * Created: 2017/8/10 星期四 下午 16:21:29
 *  Author: Edwards
 */ 

#include "RFID.h"


void rfid_init()
{
	
	tc_init();//用定时器200ms自动寻卡
	
	rc522_init();
	
	
	
	
	
	
}