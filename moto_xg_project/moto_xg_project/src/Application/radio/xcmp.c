/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: xcmp.h
Author: wxj
Version: 1.0.0.01
Date: 2016/3/29 9:48:28

Description:
History:
*/

#include <string.h>
#include <stdio.h>

/*include files are used to FreeRtos*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "physical.h"
#include "xcmp.h"

#include "../log/log.h"

/*the queue is used to receive xcmp packet*/
static volatile xQueueHandle xcmp_frame_rx = NULL;

/*callback function are used to response XCMP message*/
/*for RADIO_STATUS*/
volatile app_exec_t radio_status= {NULL, NULL, NULL};

/*for VERSION_INFORMATION*/
volatile app_exec_t version_information= {NULL, NULL, NULL};

/*for LANGUAGE_PACK_INFORMATION*/
volatile app_exec_t language_pack_information= {NULL, NULL, NULL};

/*for MAX_LANG_PACK_DEF_NAME_SIZE*/
volatile app_exec_t automatic_frequency_correction_control= {NULL, NULL, NULL};

/*for CLONE_WRITE*/
volatile app_exec_t clone_write= {NULL, NULL, NULL};

/*for CLONE_READ*/
volatile app_exec_t clone_read= {NULL, NULL, NULL};

/*for others */
/*if opcode & 0x0400 == 0x0400*/	
volatile app_exec_t * app_list;

//static volatile xSemaphoreHandle xcmp_mutex = NULL;

/**
Function: xcmp_tx
Parameters: 
	void *: data point
	U32:	data length
Description: build xnl frame to send 
Calls:   
	xnl_tx -- xnl.c
Called By: ...
*/
static void xcmp_tx( void * data_ptr, U32 data_len)
//static void xcmp_tx( xcmp_fragment_t * xcmp, U8 payload_len)
{
	/*xnl frame will be sent*/
	xnl_fragment_t xnl_frame;

	/*
	Data Type 0x4000
	*/
	U32 q= 0;
	U32 r = 0;
	U16 fragement_type =0x0000;//default SINGLE_FRAGMENT
	
	q = data_len/MAX_XCMP_DATA_LENGTH;
	r = data_len%MAX_XCMP_DATA_LENGTH;
	
		
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.phy_header.check_sum = DEFAULT_VALUE; 
	
	/*Insert opcode*/
	xnl_frame.xnl_header.opcode = XNL_DATA_MSG;
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.xnl_header.flags = DEFAULT_VALUE;	
	xnl_frame.xnl_header.destination = DEFAULT_VALUE;
	xnl_frame.xnl_header.source = DEFAULT_VALUE;	
	xnl_frame.xnl_header.transaction_id = DEFAULT_VALUE;
	
	//逻辑上不会出现包不连续发送
	//xSemaphoreTake(xcmp_mutex, portMAX_DELAY);
	if(q != 0)//商
	{//need to send data in a sub package 
		int idx=0;
		fragement_type = 0x100;//first fragment
		for (; idx<q; idx++)
		{
			xnl_frame.phy_header.phy_control = (0x4000 | fragement_type | MAX_TRANSFER_UNIT);
			/*insert xcmp frame data*/
			memcpy(&(xnl_frame.xnl_payload.xnl_content_data_msg), data_ptr, MAX_XCMP_DATA_LENGTH);
			xnl_frame.xnl_header.payload_length = MAX_XCMP_DATA_LENGTH;	
			
			/* send xnl frame*/	
			xnl_tx(&xnl_frame);
			
			//clear 
			memset(&(xnl_frame.xnl_payload.xnl_content_data_msg) , 0x00, sizeof(xnl_frame));
			data_ptr+=MAX_XCMP_DATA_LENGTH;//指针偏移
			fragement_type = 0x200;//middle fragment
		}
	}
	if(r!=0)//余数
	{
		/*
		Length :
		= checksum + xnl header + data_len
		= checksum + xnl header + xcmp opcode + xcmp payload
		= 0x02 + 0x0C + 0x02 + xcmp payload
		*/
		
		xnl_frame.phy_header.phy_control  = ((0x4000 | fragement_type) + (sizeof(xnl_frame.phy_header.check_sum) + sizeof(xnl_header_t) + r));
		
		memcpy(&(xnl_frame.xnl_payload.xnl_content_data_msg), data_ptr, r);
		xnl_frame.xnl_header.payload_length = r;
		
		/* send xnl frame*/
		xnl_tx(&xnl_frame);

	}
	
	//xSemaphoreGive(xcmp_mutex);
}

/**
Function: xcmp_rx
Parameters: xcmp_fragment_t *
Description: push xcmp to queue 
Calls:   
	xQueueSend -- xnl.c
Called By: xnl_rx--callback
*/
static void xcmp_rx(xcmp_fragment_t xcmp)
{							
	
	xcmp_fragment_t * xcmp_ptr = get_xnl_idle();// pvPortMalloc(sizeof(xcmp_fragment_t));
	//get_xnl_idle(&xcmp_ptr);
	
	if(NULL != xcmp_ptr)
	{
		memcpy(xcmp_ptr, &xcmp, sizeof(xcmp_fragment_t));				
		xQueueSend(xcmp_frame_rx, &xcmp_ptr, 0);	
	}	
}

/**
Function: xcmp_exec_func
Parameters: 
	app_exec_t * --function 
	xcmp_fragment_t  * --xcmp frame
Description: Perform XCMP corresponding functions
Calls:   
Called By: xcmp_rx_process--task
*/
static void xcmp_exec_func(app_exec_t * exec, xcmp_fragment_t * xcmp)
{
	/*The message types*/
	switch(xcmp->xcmp_opcode & 0xF000)
	{
		case XCMP_REQUEST:
			if(NULL != exec->xcmp_rx_req)
			{
				exec->xcmp_rx_req(xcmp);
			}
			else 
			{
				/*No function to register the request message*/
				/*send not supported opcode to raido */
				xcmp_opcode_not_supported();
			}
			break;
			
		case XCMP_REPLY:		
			if(NULL != exec->xcmp_rx_reply)
			{
				exec->xcmp_rx_reply(xcmp);
			}
			break;
			
		case XCMP_BRDCAST:	
			if(NULL != exec->xcmp_rx_brdcst)
			{
				exec->xcmp_rx_brdcst(xcmp);
			}
			break;
			
		default:
			break;
	}
}


/**
Function: xcmp_rx_process
Parameters: void * pvParameters
Description: receive and execute xcmp message
Calls: xQueueReceive -- freerots
	xcmp_exec_func
Called By: task
*/
static void xcmp_rx_process(void * pvParameters)
{
	/*To store the elements in the queue*/
	xcmp_fragment_t xcmp;
	xcmp_fragment_t * ptr;
	//static  portTickType water_value;
		
	for(;;)
	{
		if(pdTRUE  ==xQueueReceive( xcmp_frame_rx, &ptr,  (10*2) / portTICK_RATE_MS ))//测试启用10ms超时机制
		{									
			if(NULL == ptr)
			{
				continue;
			}
			
			//log("\n\r R_xcmp : %4x \n\r",ptr->xcmp_opcode);//log:R_xcmp指令	
			//static  portTickType water_value;
			//water_value = uxTaskGetStackHighWaterMark(NULL);
			//log("water_value: %d\n", water_value);			
			switch(ptr->xcmp_opcode & 0x0FFF)
			{
				case RADIO_STATUS:				
					xcmp_exec_func(&radio_status, ptr);
					break;
					
				case VERSION_INFORMATION:
					xcmp_exec_func(&version_information, ptr);
					break;
					
				case LANGUAGE_PACK_INFORMATION:
					xcmp_exec_func(&language_pack_information, ptr);			
					break;
					
				case AUTOMATIC_FREQUENCY_CORRECTION_CONTROL:
					xcmp_exec_func(&automatic_frequency_correction_control
						, ptr);
					break;
					
				case CLONE_WRITE:
					xcmp_exec_func(&clone_write, ptr);
					break;
					
				case CLONE_READ:
					xcmp_exec_func(&clone_read, ptr);
					break;
					
				default:
				
					/*the xcmp message not in order list*/
					/*over the length of the list*/
					if((0x0400 != (ptr->xcmp_opcode & 0x0400)) 
						|| (MAX_APP_FUNC <= (ptr->xcmp_opcode & 0x00FF))
					)
					{
						/*xcmp request*/
						if( XCMP_REQUEST == (ptr->xcmp_opcode & 0xF000))
						{
							/*send not supported opcode to raido */
							xcmp_opcode_not_supported();
						}
					}
					else
					{					
						xcmp_exec_func( &app_list[ptr->xcmp_opcode & 0x00FF]
							, ptr);
					}
					break;
			}
			//vPortFree(ptr);
			set_xnl_idle(ptr);
		}

	}
}

/**
Function: xcmp_init
Description: initialize the xnl layer
	initialize the queue
	Create the corresponding task;
Calls:   
    xnl_init -- xnl.c
    xnl_register_xcmp_func -- xnl.c
	xQueueCreate -- freertos
	xTaskCreate -- freertos
Called By: main -- main.c
*/
void xcmp_init(void)
{
	///* Create the mutex semaphore to ensure that fragmented payloads are sent contiguously.*/
	//xcmp_mutex = xSemaphoreCreateMutex();
	//if (xcmp_mutex == NULL)
	//{
		//log("Create the xcmp_mutex failure\n");
	//}
	
	/*register the xcmp function(callback function)*/
	xnl_register_xcmp_func( xcmp_rx );
	
	/*initialize the queue*/
	xcmp_frame_rx = xQueueCreate(20, sizeof(xcmp_fragment_t *));
	/*create task*/	
	/*this task is used to execute xcmp message*/
	xTaskCreate(
	xcmp_rx_process
	,  (const signed portCHAR *)"XCMP_RX"
	, 1024//750//1024//800//384
	,  NULL
	,  tskXCMP_PRIORITY
	,  NULL
	);
	
	/*initialize the xnl*/
	xnl_init();
	
}

/**
Function: xcmp_register_app_list
Parameters: void * list
Description: register the app list
Calls: 
Called By: app_init -- app.c
*/
void xcmp_register_app_list(void * list)
{
	app_list = (app_exec_t *)list;
}

/**
Function: xcmp_opcode_not_supported
Parameters:
Description: register the app list
Calls: xcmp_tx
Called By:...
*/
void xcmp_opcode_not_supported( void )
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;

	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REPLY;
	
	/*The radio does not support this opcode.*/
	xcmp_farme.u8[0] = xcmp_Res_Opcode_Not_Supported;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, 1 + sizeof(xcmp_farme.xcmp_opcode));
}

extern volatile char XCMP_Version[4];
/**
Function: xcmp_DeviceInitializationStatus_request
Parameters:
Description: send device initialization status request
Calls: xcmp_tx
Called By:...
*/
void xcmp_DeviceInitializationStatus_request(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
		
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_BRDCAST | DEVICE_INITIALIZATION_STATUS;
	
	/*point to xcmp payload*/
	DeviceInitializationStatus_brdcst_t * ptr 
						 = (DeviceInitializationStatus_brdcst_t *)xcmp_farme.u8;
	
	/*xcmp version 8.1.0.5*///版本号未正确填写，纠正
	ptr->XCMPVersion[0] = XCMP_Version[0];
	ptr->XCMPVersion[1] = XCMP_Version[1];
	ptr->XCMPVersion[2] = XCMP_Version[2];
	ptr->XCMPVersion[3] = XCMP_Version[3];
	
	/*
	0x00:
	This is the message that the device sends at power up or after a reset. It 
	will give the initial status and capabilities for the device.
	*/
	ptr->DeviceInitType = 0x00;
	
	/*
	A device may, or may not, be a system master. A system master has the 
	ability to be the central point of control for the radio system. Other 
	types of devices may provide a set of services to the radio system, but do 
	not have the ability to be the master.
	
	0x07:OptionBoard,3rd party Option Board-based application.
	*/
	ptr->DeviceStatusInfo.DeviceType = 0x07;
	
	/*Power Up Success, Device has powered up with no errors*/
	ptr->DeviceStatusInfo.DeviceStatus[0] = 0x00;
	ptr->DeviceStatusInfo.DeviceStatus[1] = 0x00;
	
	/*
	This number is the size of the Device Descriptor data structure with a 
	maximum size not exceeding 255 bytes.
	*/
	ptr->DeviceStatusInfo.DeviceDescriptorSize = 0x00;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme
		, sizeof(DeviceInitializationStatus_brdcst_t) - MAX_DEVICE_DESC_SIZE + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_IdleTestTone
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_IdleTestTone(U8 type, U16 toneID)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | TONE_CONTROL;
	
	/*point to xcmp payload*/
	ToneControl_req_t * ptr = (ToneControl_req_t *)xcmp_farme.u8;
	
	/*Starts the specific tone*/	
	ptr->Function = type;
	
	/*This tone shall be sounded when radio landed on a priority channel*/
	//ptr->ToneIdentifier[0] = (Priority_Beep >> 8) & 0xFF;
	//ptr->ToneIdentifier[1] = Priority_Beep & 0xFF;
	
	ptr->ToneIdentifier[0] = (toneID >> 8) & 0xFF;
	ptr->ToneIdentifier[1] = toneID & 0xFF;	
	
	/*
	The alert tone is played according to any rules for alert tones, given the 
	current volume setting. Settings for this field are 8 to 255 (0x08 鈥?0xFF).
	*/
	ptr->ToneVolumeControl = Current_Volume;
	
	/*clear reserved*/
	memset(ptr->Reserved, 0, 8);
	
	/*send xcmp frame*/
	//note:length <=238bytes
	xcmp_tx( &xcmp_farme, sizeof(ToneControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_audio_route_mic
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_audio_route_mic(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
	
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
			
	ptr->Function = Routing_Func_Update_Source;
	
	
	unsigned short NumberofRoutings = 2;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] = NumberofRoutings & 0xFF;
	
	ptr->RoutingData[0].audioInput = IN_Microphone;
	ptr->RoutingData[0].audioOutput = OUT_Option_Board;
	
	ptr->RoutingData[1].audioInput = IN_Option_Board;
	ptr->RoutingData[1].audioOutput = OUT_Microphone_Data;
		//
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));
}



U8 DataPayload[20]={//注意payload的格式很重要，//Each CSBK can carry 10 bytes as payload including 2 bytes as fixed heading.
	
	
	
	
	0xBE,//10 11, 1110.byte1
	0x20,//CSBK munufactturing ID.byte2
	
	0x03,//byte3
	0x04,//byte4
	0x05,//byte5
	0x06,//byte6
	0x07,//byte7
	0x08,//byte8
	0x00,
	0x03,
	//0x09,//byte9
	//0x0A,//byte10
	//
	0xBF,//10 11, 1111.byte1
	0x20,//CSBK munufactturing ID.byte2
	//
	0x30,//byte3
	0x40,//byte4
	0x50,//byte5
	0x60,//byte6
	0x70,//byte7
	0x80,//byte8
	0x90,//byte9
	0xA0 //byte10

	
};


extern U8  MAX_ADDRESS_SIZE;

/**
Function: xcmp_data_session_req
Parameters:cmd
Description: send data-session request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_data_session_req(void *message, U16 length, U32 dest)
{
	//U8 *DMR_Raw_Data= &DataPayload[0];
	U8 i =0;
	
	//U8 SizeofAddress = Remote_Mototrbo_Address_Size;// 3;//可能会变化
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | DATA_SESSION;
	
	/*point to xcmp payload*/
	DataSession_req_t * ptr = (DataSession_req_t *)xcmp_farme.u8;
	
	if (length > 256)return -1;
	
	ptr->Function = Single_Data_Uint;
	
	ptr->DataDefinition.Data_Protocol_Version = TMP_Ver_1_1;//0x20
	
	ptr->DataDefinition.Dest_Address.Remote_Address_Type = Remote_IPV4_Address;//0x02
	
	ptr->DataDefinition.Dest_Address.Remote_Address_Size = Remote_IPV4_Address_Size;//0x04
	
	unsigned int addr = 0x0C000000 | (dest & 0x00FFFFFF);
	memcpy(ptr->DataDefinition.Dest_Address.Remote_Address, &addr, Remote_IPV4_Address_Size);
	//ptr->DataDefinition.Dest_Address.Remote_Address[0] = 0x0C;//12
	//ptr->DataDefinition.Dest_Address.Remote_Address[1] = 0x00;//0
	//ptr->DataDefinition.Dest_Address.Remote_Address[2] = 0x00;//0
	//ptr->DataDefinition.Dest_Address.Remote_Address[3] = 0x02;//2
	
	ptr->DataDefinition.Dest_Address.Remote_Port_Com[0] = (Remote_Port >>8) & 0xFF;//4007
	ptr->DataDefinition.Dest_Address.Remote_Port_Com[1] = Remote_Port & 0xFF;//
	
    
	ptr->DataPayload.Session_ID_Number = Session_ID;//0x00
	
	ptr->DataPayload.DataPayload_Length[0] =(length >> 8) & 0xFF ;//可能会变化
	ptr->DataPayload.DataPayload_Length[1] =length & 0xFF  ;//可能会变化
	
	memcpy(&(ptr->DataPayload.DataPayload[0]), message, length);
	
	xcmp_tx(&xcmp_farme, sizeof(DataSession_req_t) - (256 - length) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_data_session_brd
Parameters:message
Description: send data-session broadcast to radio
Calls: xcmp_tx
Called By:...
*/
void xcmp_data_session_brd( void *message, U16 length, U8 SessionID)
{
	U8 i =0;
	//static U8 SessionID = 0;
	
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_BRDCAST | DATA_SESSION;
	
	/*point to xcmp payload*/
	DataSession_brdcst_t * ptr = (DataSession_brdcst_t *)xcmp_farme.u8;
	
	
	ptr->State = Data_Session_Start;//0x10

	ptr->DataPayload.Session_ID_Number = SessionID;//0x00++
	
	if (length > 20)return -1;
	
	ptr->DataPayload.DataPayload_Length[0] =(length >> 8) & 0xFF ;//可能会变化
	ptr->DataPayload.DataPayload_Length[1] =length & 0xFF  ;//可能会变化
	
	//for (i=0; i< length ; i++)
	//{
		//
		//ptr->DataPayload.DataPayload[i] = message[i];//长度计算了吗？
		//
	//}
	
	memcpy(&(ptr->DataPayload.DataPayload[0]), message, length);

		
	xcmp_tx( &xcmp_farme, sizeof(DataSession_brdcst_t)-(sizeof(DataPayload_t) - length) + sizeof(xcmp_farme.xcmp_opcode));

	
}


/**
Function: xcmp_button_config
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_button_config(void)
{
		/*xcmp frame will be sent*/
		xcmp_fragment_t xcmp_farme;
		
		/*insert XCMP opcode*/
		xcmp_farme.xcmp_opcode = XCMP_REQUEST | BTN_CONFIG_REQUEST;
		
		/*point to xcmp payload*/
		ButtonConfig_req_t * ptr = (ButtonConfig_req_t *)xcmp_farme.u8;
		
		//ptr->ButtonInfo = (U8 *)malloc(sizeof(U8) * 1);//使用的时候再分配内存空间？测试看是否可行
		
		ptr->Function = Get_Info;//0x00
		
		ptr->NumOfButtons = 0x00;
		
		ptr->ButtoInfoStructSize = Button_Info_StructSize;
		
		ptr->ButtonInfo[0].ButtonIdentifier[0] = 0x00;
		ptr->ButtonInfo[0].ButtonIdentifier[1] = 0x00;
		
		ptr->ButtonInfo[0].ShortPressFeature[0] = 0x00;
		ptr->ButtonInfo[0].ShortPressFeature[1] = 0x00;
		
		ptr->ButtonInfo[0].Reserved1[0] = 0x00;
		ptr->ButtonInfo[0].Reserved1[1] = 0x00;
		
		ptr->ButtonInfo[0].LongPressFeature[0] = 0x00;
		ptr->ButtonInfo[0].LongPressFeature[1] = 0x00;
		
		ptr->ButtonInfo[0].Reserved2[0] = 0x00;
		ptr->ButtonInfo[0].Reserved2[1] = 0x00;
		
		
		
		/*send xcmp frame*///注意！！！！！！！！！！
		xcmp_tx( &xcmp_farme, sizeof(ButtonConfig_req_t) + sizeof(xcmp_farme.xcmp_opcode));
	
}


/**
Function: xcmp_enhanced_OB_mode
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_enter_enhanced_OB_mode(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | EN_OB_CONTROL;
	
	/*point to xcmp payload*/
	En_OB_Control_req_t * ptr = (En_OB_Control_req_t *)xcmp_farme.u8;
	
	ptr->Function = EN_OB_Enter;
		
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(En_OB_Control_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_exit_enhanced_OB_mode
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_exit_enhanced_OB_mode(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | EN_OB_CONTROL;
	
	/*point to xcmp payload*/
	En_OB_Control_req_t * ptr = (En_OB_Control_req_t *)xcmp_farme.u8;
	
	ptr->Function = EN_OB_Exit;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(En_OB_Control_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_volume_control
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_volume_control(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | VOLUME_CONTROL;
	
	/*point to xcmp payload*/
	VolumeControl_req_t * ptr = (VolumeControl_req_t *)xcmp_farme.u8;
	
	ptr->Attenuator_Number[0] = (All_Speakers >> 8) & 0xFF;
	ptr->Attenuator_Number[1] = All_Speakers & 0xff;
	
	ptr->Function = Enable_IntelligentAudio;
	
	ptr->Volume_Data = 0x00;
	

	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(VolumeControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}


/**
Function: xcmp_audio_route_speaker
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_audio_route_speaker(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
		
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
		
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
		
	ptr->Function = Routing_Func_Update_Source;
		
		
	unsigned short NumberofRoutings =  2;//2;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] =  NumberofRoutings & 0xFF;
		
	
	
	
	ptr->RoutingData[0].audioInput = IN_Pre_Speaker_Audio_Data;//IN_Pre_Speaker_Audio_Data;//IN_Microphone;//IN_Option_Board;
	ptr->RoutingData[0].audioOutput = OUT_Option_Board;//OUT_Option_Board;//OUT_Microphone_Data;//测试
	
	
	//ptr->RoutingData[0].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[0].audioOutput =OUT_Option_Board;// OUT_Speaker;
	
	ptr->RoutingData[1].audioInput = IN_Option_Board;//IN_Option_Board;
	ptr->RoutingData[1].audioOutput = OUT_Speaker;//OUT_Microphone_Data;//测试
	
	//ptr->RoutingData[1].audioInput = IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Microphone_Data;//测试OUT_Speaker;//
		
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));
}


/**
Function: xcmp_audio_route_revert
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_audio_route_revert(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
	
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = Routing_Func_Default_Source;
	
	
	unsigned short NumberofRoutings = 0;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] = NumberofRoutings & 0xFF;
	
	
	//ptr->RoutingData[0].audioInput = IN_Option_Board;
	//ptr->RoutingData[0].audioOutput = OUT_Speaker;
	
	//ptr->RoutingData[0].audioInput = IN_Option_Board;
	//ptr->RoutingData[0].audioOutput = OUT_Microphone_Data;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));
}


void xcmp_audio_route_AMBE(void)
{

	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
	
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = Routing_Func_Update_Source;
	
	
	unsigned short NumberofRoutings = 6;// 4;//2;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] =  NumberofRoutings & 0xFF;
	
	//ptr->RoutingData[0].audioInput = IN_Microphone;//Post_AMBE_Encoder;//IN_Pre_Speaker_Audio_Data;//IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[0].audioOutput = OUT_Option_Board;//Post_AMBE_Encoder;//OUT_Microphone_Data;//测试
	//注意：经测试发现，这里的路径配置，需要特别注意先后顺序，否则会提示参数错误。
	ptr->RoutingData[0].audioInput = Post_AMBE_Encoder;//IN_Option_Board;
	ptr->RoutingData[0].audioOutput = OUT_Option_Board;// OUT_Speaker;
	ptr->RoutingData[1].audioInput = IN_Option_Board;//IN_Option_Board;
	ptr->RoutingData[1].audioOutput = Post_AMBE_Encoder;// OUT_Speaker;
	
	ptr->RoutingData[2].audioInput = Pre_AMBE_Decoder;//IN_Option_Board;
	ptr->RoutingData[2].audioOutput = OUT_Option_Board;// OUT_Speaker;
	ptr->RoutingData[3].audioInput = IN_Option_Board;//IN_Option_Board;
	ptr->RoutingData[3].audioOutput = Pre_AMBE_Decoder;// OUT_Speaker;
	
	ptr->RoutingData[4].audioInput = Tx_Voice_Header;//IN_Option_Board;
	ptr->RoutingData[4].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[3].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Tx_Voice_Header;// OUT_Speaker;
	
	ptr->RoutingData[5].audioInput = Tx_Voice_Terminator;//IN_Option_Board;
	ptr->RoutingData[5].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[5].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[5].audioOutput = Tx_Voice_Terminator;// OUT_Speaker;
	
	//ptr->RoutingData[4].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[4].audioOutput = OUT_Option_Board;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = Post_AMBE_Encoder;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//
	//ptr->RoutingData[2].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[2].audioOutput = Post_AMBE_Encoder;// OUT_Speaker;
	
	//ptr->RoutingData[3].audioInput = Post_AMBE_Decoder;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Post_AMBE_Decoder;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;//OUT_Microphone_Data;//测试
	
	//ptr->RoutingData[1].audioInput = IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Microphone_Data;//测试OUT_Speaker;//
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));

	
}


/**
Function: xcmp_audio_route_encoder_AMBE
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_audio_route_encoder_AMBE(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
	
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = Routing_Func_Update_Source;
	
	
	unsigned short NumberofRoutings =  4;//2;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] =  NumberofRoutings & 0xFF;
	
	//ptr->RoutingData[0].audioInput = IN_Microphone;//Post_AMBE_Encoder;//IN_Pre_Speaker_Audio_Data;//IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[0].audioOutput = OUT_Option_Board;//Post_AMBE_Encoder;//OUT_Microphone_Data;//测试
	//注意：经测试发现，这里的路径配置，需要特别注意先后顺序，否则会提示参数错误。
	ptr->RoutingData[0].audioInput = Post_AMBE_Encoder;//IN_Option_Board;
	ptr->RoutingData[0].audioOutput = OUT_Option_Board;// OUT_Speaker;
	ptr->RoutingData[1].audioInput = IN_Option_Board;//IN_Option_Board;
	ptr->RoutingData[1].audioOutput = Post_AMBE_Encoder;// OUT_Speaker;
	
	ptr->RoutingData[2].audioInput = Tx_Voice_Header;//IN_Option_Board;
	ptr->RoutingData[2].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[3].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Tx_Voice_Header;// OUT_Speaker;
	
	ptr->RoutingData[3].audioInput = Tx_Voice_Terminator;//IN_Option_Board;
	ptr->RoutingData[3].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[5].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[5].audioOutput = Tx_Voice_Terminator;// OUT_Speaker;
	
	//ptr->RoutingData[4].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[4].audioOutput = OUT_Option_Board;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = Post_AMBE_Encoder;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//
	//ptr->RoutingData[2].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[2].audioOutput = Post_AMBE_Encoder;// OUT_Speaker;
	
	//ptr->RoutingData[3].audioInput = Post_AMBE_Decoder;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Post_AMBE_Decoder;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;//OUT_Microphone_Data;//测试
	
	//ptr->RoutingData[1].audioInput = IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Microphone_Data;//测试OUT_Speaker;//
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_audio_route_decoder_AMBE
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_audio_route_decoder_AMBE(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | AUDIO_ROUTING_CONTROL;
	
	/*point to xcmp payload*/
	AudioRoutingControl_req_t * ptr = (AudioRoutingControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = Routing_Func_Update_Source;
	
	
	unsigned short NumberofRoutings = 4;//2;
	ptr->NumberofRoutings[0] = (NumberofRoutings >> 8) & 0xFF;
	ptr->NumberofRoutings[1] =  NumberofRoutings & 0xFF;
	
	//ptr->RoutingData[0].audioInput = IN_Microphone;//Post_AMBE_Encoder;//IN_Pre_Speaker_Audio_Data;//IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[0].audioOutput = OUT_Option_Board;//Post_AMBE_Encoder;//OUT_Microphone_Data;//测试
	//注意：经测试发现，这里的路径配置，需要特别注意先后顺序，否则会提示参数错误。
	
	ptr->RoutingData[0].audioInput = Pre_AMBE_Decoder;//IN_Option_Board;
	ptr->RoutingData[0].audioOutput = OUT_Option_Board;// OUT_Speaker;
	ptr->RoutingData[1].audioInput = IN_Option_Board;//IN_Option_Board;
	ptr->RoutingData[1].audioOutput = Pre_AMBE_Decoder;// OUT_Speaker;

	
	ptr->RoutingData[2].audioInput = Tx_Voice_Header;//IN_Option_Board;
	ptr->RoutingData[2].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[3].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Tx_Voice_Header;// OUT_Speaker;
	
	ptr->RoutingData[3].audioInput = Tx_Voice_Terminator;//IN_Option_Board;
	ptr->RoutingData[3].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//ptr->RoutingData[5].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[5].audioOutput = Tx_Voice_Terminator;// OUT_Speaker;
	
	//ptr->RoutingData[4].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[4].audioOutput = OUT_Option_Board;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = Post_AMBE_Encoder;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;// OUT_Speaker;
	//
	//ptr->RoutingData[2].audioInput = IN_Option_Board;//IN_Option_Board;
	//ptr->RoutingData[2].audioOutput = Post_AMBE_Encoder;// OUT_Speaker;
	
	//ptr->RoutingData[3].audioInput = Post_AMBE_Decoder;//IN_Option_Board;
	//ptr->RoutingData[3].audioOutput = Post_AMBE_Decoder;// OUT_Speaker;
	
	//ptr->RoutingData[1].audioInput = IN_Microphone;//IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Option_Board;//OUT_Microphone_Data;//测试
	
	//ptr->RoutingData[1].audioInput = IN_Option_Board;
	//ptr->RoutingData[1].audioOutput = OUT_Microphone_Data;//测试OUT_Speaker;//
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(AudioRoutingControl_req_t) - (MAX_ROUTING_CTR - NumberofRoutings) * sizeof(RoutingData_t) + sizeof(xcmp_farme.xcmp_opcode));
}



/**
Function: xcmp_enter_device_control_mode
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_enter_device_control_mode(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | DEVICE_CONTROL_MODE;
	
	/*point to xcmp payload*/
	DeviceControlMode_req_t * ptr = (DeviceControlMode_req_t *)xcmp_farme.u8;
	
	ptr->Function = DCM_ENTER;
	ptr->ControlTypeSize = 1;
	ptr->ControlType = 0x03;// 0xEB;//user-input
	//ptr->ControlType = DCM_SPEAKER_CTRL;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(DeviceControlMode_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}


/**
Function: xcmp_enter_device_control_mode
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_exit_device_control_mode(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | DEVICE_CONTROL_MODE;
	
	/*point to xcmp payload*/
	DeviceControlMode_req_t * ptr = (DeviceControlMode_req_t *)xcmp_farme.u8;
	
	ptr->Function = DCM_EXIT;
	ptr->ControlTypeSize = 1;
	ptr->ControlType = 0x03;//DCM_SPEAKER_CTRL;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(DeviceControlMode_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_transmit_control
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_transmit_control( void )
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | KEYREQ;
	
	/*point to xcmp payload*/
	TransmitControl_req_t * ptr = (TransmitControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = KEY_UP ;
	ptr->Mode_Of_Operation = MODE_VOICE;
	ptr->TT_Source = 0x00;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(TransmitControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}




void xcmp_transmit_dekeycontrol(void)
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | KEYREQ;
	
	/*point to xcmp payload*/
	TransmitControl_req_t * ptr = (TransmitControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = DE_KEY ;
	ptr->Mode_Of_Operation = MODE_VOICE;
	ptr->TT_Source = 0x00;
	
	
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(TransmitControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
	
	
	
	
}

/**
Function: xcmp_function_mic
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_function_mic( void )
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | MIC_CONTROL;
	
	/*point to xcmp payload*/
	MicControl_req_t * ptr = (MicControl_req_t *)xcmp_farme.u8;
	
	ptr->Function = Mic_Disable;
	ptr->Mic_Type = Mic_External;
	ptr->Signaling_Type = Sig_Type_Digital;
	ptr->Gain_Offset = 0x17;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(MicControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}



/**
Function: xcmp_unmute_speaker
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_unmute_speaker( void )
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | SPEAKER_CONTROL;
	
	/*point to xcmp payload*/
	SpeakerControl_req_t * ptr = (SpeakerControl_req_t *)xcmp_farme.u8;
	
	ptr->SpeakerNumber[0] = (All >> 8) & 0xFF;
	ptr->SpeakerNumber[1] = All & 0xFF;
	
	ptr->Function[0] = (UNMUTED >> 8) & 0xFF;
	ptr->Function[1] = UNMUTED & 0xFF;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(SpeakerControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}

/**
Function: xcmp_unmute_speaker
Parameters:
Description: send tone request to test
Calls: xcmp_tx
Called By:...
*/
void xcmp_mute_speaker( void )
{
	/*xcmp frame will be sent*/
	xcmp_fragment_t xcmp_farme;
	
	/*insert XCMP opcode*/
	xcmp_farme.xcmp_opcode = XCMP_REQUEST | SPEAKER_CONTROL;
	
	/*point to xcmp payload*/
	SpeakerControl_req_t * ptr = (SpeakerControl_req_t *)xcmp_farme.u8;
	
	ptr->SpeakerNumber[0] = (All >> 8) & 0xFF;
	ptr->SpeakerNumber[1] = All & 0xFF;
	
	ptr->Function[0] = (MUTED >> 8) & 0xFF;
	ptr->Function[1] = MUTED & 0xFF;
	
	/*send xcmp frame*/
	xcmp_tx( &xcmp_farme, sizeof(SpeakerControl_req_t) + sizeof(xcmp_farme.xcmp_opcode));
}