/*
 * app.h
 *
 * Created: 2016/3/15 14:18:10
 *  Author: JHDEV2
 */ 


#ifndef APP_H_
#define APP_H_

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "string.h"
#include "queue.h"
#include "timer.h"
#include "RFID.h"
#include "xgrtc.h"
#include "xgflash.h"
//#include "voice.h"

#include <xnl.h>

#define __app_Thread_(x) void x(void * pvParameters)

#define tskIDLE_PRIORITY			( ( unsigned portBASE_TYPE ) 0 )

static const U8 OB_Firmware_Version[3]={0x02, 0x01, 0x01}; 
//The 3-byte OB Firmware Version number uses a Major Number to track the major changes,
// Minor Number to track minor changes and Product ID Number to differentiate the product line.
/*Product ID Number:
				0x01   Patrol   
				0x02   Record
				0x03	...	
				...		...

*/
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

typedef enum {
	OB_UNCONNECTEDWAITINGSTATUS,
	OB_CONNECTED,
	OB_CONNECTEDWAITTINGSYNTIME,
	OB_WAITINGAPPTASK
} OB_States;

extern xQueueHandle_t x_queue_list;

void app_init( void );
void app_start( void );

#endif /* APP_H_ */