/*
 * app.h
 *
 * Created: 2016/3/15 14:18:10
 *  Author: JHDEV2
 */ 


#ifndef APP_H_
#define APP_H_

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "RFID.h"
//#include "voice.h"

#include <xnl.h>

#define __app_Thread_(x) void x(void * pvParameters)

#define tskIDLE_PRIORITY			( ( unsigned portBASE_TYPE ) 0 )


typedef struct
{
	
}xQueueHandle_t;


typedef struct
{
	xTaskHandle	app_cfg;
	xTaskHandle	rxUsart;
	xTaskHandle	execRawOp;
	xTaskHandle execNand;
	xTaskHandle controlPrg;
}xTaskHandle_t;

extern xQueueHandle_t x_queue_list;

void app_init( void );
void app_start( void );

#endif /* APP_H_ */