/*
 * app.h
 *
 * Created: 2016/3/15 14:18:10
 *  Author: JHDEV2
 */ 


#ifndef APP_H_
#define APP_H_
#include "log.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "string.h"
#include "queue.h"
#include "timer.h"
//#include "RFID.h"
//#include "xgrtc.h"
//#include "xgflash.h"
//#include "myusart.h"
//#include "voice.h"

#include <xnl.h>

#define __app_Thread_(x) void x(void * pvParameters)



//typedef struct
//{
	//
//}xQueueHandle_t;
//
//
//typedef struct
//{
	//xTaskHandle	app_cfg;
	//xTaskHandle	rxUsart;
	//xTaskHandle	execRawOp;
	//xTaskHandle execNand;
	//xTaskHandle controlPrg;
//}xTaskHandle_t;

typedef enum {
	OB_UNCONNECTEDWAITINGSTATUS,
	OB_CONNECTED,
	OB_CONNECTEDWAITTINGSYNTIME,
	OB_WAITINGAPPTASK
} OB_States;

//extern xQueueHandle_t x_queue_list;

//void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len);

void app_init( void );
void app_start( void );
//void DeviceInitializationStatus_brdcst_func(xcmp_fragment_t  * );
//void DeviceManagement_brdcst_func(xcmp_fragment_t * xcmp);
//void ToneControl_reply_func(xcmp_fragment_t * xcmp);
//void dcm_reply_func(xcmp_fragment_t * xcmp);
//void dcm_brdcst_func(xcmp_fragment_t * xcmp);
//void mic_reply_func(xcmp_fragment_t * xcmp);
//void mic_brdcst_func(xcmp_fragment_t * xcmp);
//void spk_brdcst_func(xcmp_fragment_t * xcmp);
//void spk_reply_func(xcmp_fragment_t * xcmp);
//void Volume_reply_func(xcmp_fragment_t * xcmp);
//void Volume_brdcst_func(xcmp_fragment_t * xcmp);
//void AudioRoutingControl_reply_func(xcmp_fragment_t * xcmp);
//void AudioRoutingControl_brdcst_func(xcmp_fragment_t * xcmp);
//void TransmitControl_reply_func(xcmp_fragment_t * xcmp);
//void TransmitControl_brdcst_func(xcmp_fragment_t * xcmp);
//void CallControl_brdcst_func(xcmp_fragment_t * xcmp);
//void DataSession_request_func(xcmp_fragment_t * xcmp);
//void DataSession_reply_func(xcmp_fragment_t * xcmp);
//void ShutDown_brdcst_func(xcmp_fragment_t * xcmp);
//void BatteryLevel_brdcst_func(xcmp_fragment_t * xcmp);
//void DataSession_brdcst_func(xcmp_fragment_t * xcmp);
//void ButtonConfig_reply_func(xcmp_fragment_t * xcmp);
//void Phyuserinput_brdcst_func(xcmp_fragment_t * xcmp);
//void ButtonConfig_brdcst_func(xcmp_fragment_t * xcmp);
//void SingleDetection_brdcst_func(xcmp_fragment_t * xcmp);
//void EnOB_reply_func(xcmp_fragment_t * xcmp);
//void EnOB_brdcst_func(xcmp_fragment_t * xcmp);
//void FD_request_func(xcmp_fragment_t * xcmp);
//void FD_reply_func(xcmp_fragment_t * xcmp);
//void FD_brdcst_func(xcmp_fragment_t * xcmp);
void vApplicationIdleHook( void );


#endif /* APP_H_ */