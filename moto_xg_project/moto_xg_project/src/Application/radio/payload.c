/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: payload.c
Author: wxj
Version: 1.0.0.01
Date: 2016/4/6 13:22:10

Description:
History:
*/

/*include files are used to FreeRtos*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "physical.h"
#include "payload.h"

#include "../Log/log.h"

/*Defines the callback function is used to receive/transmit media*/
void ( *payload_rx_exec)(void * ) = NULL;
void ( *payload_tx_exec)(void * ) = NULL;

/**
Function: payload_tx_process
Parameters: void *
Description: Transmit the payload
Calls:
Called By:task
*/
static void payload_tx_process(void * pvParameters)
{
	/*To store the elements in the queue*/
	U16  * payload_ptr;
	
	if(NULL == phy_payload_frame_tx)
	{
		phy_payload_frame_tx = xQueueCreate(TX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t *));
	}
	
	for(;;)
	{
		if(pdTRUE == xQueueReceive( phy_payload_frame_tx, &payload_ptr,portMAX_DELAY ))
		{
			payload_tx_exec(payload_ptr);//app_payload_tx_proc();
		}

	}
}



/**
Function: payload_rx_process
Parameters: void *
Description: Receive the payload
Calls:
Called By:task
*/
static void payload_rx_process(void * pvParameters)
{
	/*To store the elements in the queue*/
	U16  * payload_ptr;
		
	if(NULL ==   phy_payload_frame_rx)
	{
		phy_payload_frame_rx = xQueueCreate(RX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t *));
	}
	
	for(;;)
	{
		if(pdTRUE == xQueueReceive( phy_payload_frame_rx, &payload_ptr,portMAX_DELAY ))
		{			
			payload_rx_exec(payload_ptr);//app_payload_rx_proc();
		}
	
	}
}

/**
Function: payload_init
Parameters: void 
Description: 
	register the function to receive/transmit payload
	Create the corresponding task;
Calls: xTaskCreate--freertos
Called By:app_init -- app.c
*/
void payload_init(void ( *payload_rx_func)(void * ), void ( *payload_tx_func)(void * ) )
{				
	payload_rx_exec = payload_rx_func;
	payload_tx_exec = payload_tx_func;
	
	
	/*create task*/	
	/*this task is used to receive xnl message*/
	//xTaskCreate(
	//payload_rx_process
	//,  (const signed portCHAR *)"PAYLOAD_RX"
	//,  1024
	//,  NULL
	//,  2
	//,  NULL
	//);
	//
	///*this task is used to transmit  payload message*/
	//xTaskCreate(
	//payload_tx_process
	//,  (const signed portCHAR *)"PAYLOAD_TX"
	//,  1024
	//,  NULL
	//,  2
	//,  NULL
	//);
	//
	
}

