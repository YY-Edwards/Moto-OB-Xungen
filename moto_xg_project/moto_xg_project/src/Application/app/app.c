/*
 * app.c
 *
 * Created: 2016/3/15 14:17:57
 *  Author: JHDEV2
 */ 
#include "app.h"
#include "../Log/log.h"

#include <xcmp.h>
#include <payload.h>
#include <rtc.h>
#include <physical.h>
#include "string.h"

static __app_Thread_(app_cfg);

volatile U32 bunchofrandomstatusflags;

volatile U8 Speaker_is_unmute = 0;
volatile U8 Silent_flag = 0;
volatile U8 Terminator_Flag = 0;
volatile U8 AMBE_rx_flag = 0;
volatile U8 AMBE_tx_flag = 0;
volatile U8 AMBE_Media = 0;
volatile U8 Radio_Transmit_State = 0;// in standby or receive mode
volatile U8 Mic_is_Enabled = 0;
volatile U8 Call_Begin = 0;

volatile U8 VF_SN = 0;
volatile U8 Battery_Flag = Battery_Okay;

/* Declare a variable that will be incremented by the hook function. */
unsigned long ulIdleCycleCount = 0UL;

volatile U8  allocated_session_ID =0;
static volatile U8 connect_flag =0; 
static volatile U8 get_time_okay =TRUE; 


/*until receive/transmit payload data*/
static void app_payload_rx_proc(void  * payload);
static void app_payload_tx_proc(void  * payload);

/*the queue is used to storage failure-send message*/
extern volatile xQueueHandle message_storage_queue ;

/*the queue is used to receive failure-send message*/
extern volatile xQueueHandle xg_resend_queue ;

extern volatile xSemaphoreHandle SendM_CountingSemaphore;
extern volatile  xSemaphoreHandle xBinarySemaphore;
volatile xSemaphoreHandle count_mutex = NULL;
volatile U32 global_count =0;
extern /*information of xnl*/
volatile xnl_information_t xnl_information;
volatile char XCMP_Version[4];
//xnl_content_master_status_brdcst_t XCMP_Version;

//app func--list

void DeviceInitializationStatus_brdcst_func(xcmp_fragment_t  * xcmp)
{
	/*point to xcmp payload*/
	DeviceInitializationStatus_brdcst_t *ptr = (DeviceInitializationStatus_brdcst_t* )xcmp->u8;
	
	//log("DeviceInitializationStatus_brdcst...\n");
	
	memcpy(XCMP_Version, &(ptr->XCMPVersion[0]), sizeof(XCMP_Version));
	
	if (ptr->DeviceInitType == Device_Init_Complete)
	{
		bunchofrandomstatusflags |= 0x01;  //Need do nothing else.
	}
	else if(ptr->DeviceInitType  == Device_Init_Status)
	{
		bunchofrandomstatusflags  &= 0xFFFFFFFC; //Device Init no longer Complete.
		xcmp_DeviceInitializationStatus_request();
	}
	else//Device_Status_Update
	{
		//log("DeviceInitType : %x\n", ptr->DeviceInitType);
		//log("Device Type : %x\n", ptr->DeviceStatusInfo.DeviceType);
		//log("Device Status : %x\n", ptr->DeviceStatusInfo.DeviceStatus[1]);
		//log("Descriptor size : %x\n", ptr->DeviceStatusInfo.DeviceDescriptorSize);
		//log("Descriptor : %x\n", ptr->DeviceStatusInfo.DeviceDescriptor[0]);
	}
	//log("DeviceInitType : %x\n", ptr->DeviceInitType);
	
	//if (xcmp->u8[4] == 0x01)
	//{
		//bunchofrandomstatusflags |= 0x01;  //Need do nothing else.
	//}
	//else if(xcmp->u8[4] != 0x02)
	//{
		//bunchofrandomstatusflags  &= 0xFFFFFFFC; //Device Init no longer Complete.
		//xcmp_DeviceInitializationStatus_request();
	//}
}

void DeviceManagement_brdcst_func(xcmp_fragment_t * xcmp)
{
		U16 temp = 0;
		/*point to xcmp payload*/
		DeviceManagement_brdcast_t *ptr = (DeviceManagement_brdcast_t* )xcmp->u8;
		temp  = ptr->Device_Type << 8;
		temp |= ptr->XCMP_Device_ID;
		
		//log("DeviceManagement_brdcst...\n");
		//log("temp: %x\n", temp);
		//log("xnl_information.logical_address: %x\n", xnl_information.logical_address);
		//temp  = xcmp->u8[1] << 8;
		//temp |= xcmp->u8[2];
		if (temp == xnl_information.logical_address)
		{
			if (xcmp->u8[0] == 0x01)
			//if(ptr->Function == Start)
			{
				//Enable Option Board
				bunchofrandomstatusflags |= 0x00000002;
			}
			else
			{
				//Disable Option Board.
				//log("Device State : %d\n", );
				bunchofrandomstatusflags &= 0xFFFFFFFD;
			}
			//log("Function : %d\n", ptr->Function);
			//log("Device State : %d\n", ptr->Device_State);
		}
}

void ToneControl_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{		
		log("Tone OK\n");
		//fl_write("/test.txt", FILE_END, (void *)"send tone ok\r\n", sizeof("send tone ok\r\n") - 1);
	}
	else
	{
		log("Tone error");
	}
}

void dcm_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		if(xcmp->u8[1] == DCM_ENTER)
		{
			log("\n\r Dcm-Enter OK \n\r");
			
		}
		else if (xcmp->u8[1] == DCM_EXIT)
		{
			log("\n\r Dcm-Exit OK \n\r");
		}
		else
		{
			log("\n\r Dcm-Revoke \n\r");
		}
		
		log("dcm OK-mo%X", xcmp->u8[3]);
	}
	else
	{
		log("dcm error");
	}
}


void dcm_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	/*point to xcmp payload*/
	DeviceControlMode_brdcst_t *ptr = (DeviceControlMode_brdcst_t* )xcmp->u8;
	
	log("\n\r Dcm_brdcst \n\r");		
	log("\n\r Function: %x \n\r " ,  ptr->Function);
	log("\n\r ControlType: %x \n\r " ,  ptr->ControlType);
	log("\n\r ControlTypeSize: %x \n\r " ,  ptr->ControlTypeSize);
	
	
}

void mic_reply_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	MicControl_reply_t *ptr = (MicControl_reply_t* )xcmp->u8;
	
	log("\n\r Mic_reply \n\r");
	if (ptr->Result == 0x00)
	{
		
		if (ptr->Function == Mic_Disable)
		{
		
			log("\n\r Mic_close_ok \n\r " );
			log("\n\r Mic_type: %x \n\r " ,  ptr->Mic_Type);
			log("\n\r Signaling_type: %x \n\r " ,  ptr->Signaling_Type);
			log("\n\r Mic_state: %x \n\r " ,  ptr->Mic_State);
			log("\n\r Gain_offset: %x \n\r " ,  ptr->Gain_Offset);
			
		}
		else
		{
			log("\n\r Mic_function: %x \n\r ", ptr->Function );
		}
		
		
	}
	else 
	{
		
	
		log("\n\r Mic error \n\r");
		
	}
	
	
	
}

void mic_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	MicControl_brdcast_t *ptr = (MicControl_brdcast_t* )xcmp->u8;
	//log("\n\r Mic_brdcst \n\r");		
	//log("\n\r Mic_type: %x \n\r " ,  ptr->Mic_Type);
	//log("\n\r Signal_type: %x \n\r " ,  ptr->Signaling_Type);
	if (ptr->Mic_State == 0x00)
	{
		log("\n\r Mic_Disabled \n\r");	
		Mic_is_Enabled = 0;
	} 
	if(ptr->Mic_State == 0x11)
	{
		log("\n\r Mic_Enabled \n\r");
		Mic_is_Enabled = 1;
		
		if ((Mic_is_Enabled == 1) && (Call_Begin == 1))
		{
			//配置加密通道
			//xcmp_audio_route_encoder_AMBE();
		}
		
	}
	//log("\n\r Mic_state: %x \n\r " ,  ptr->Mic_State);
	//log("\n\r Gain_offset: %x \n\r " ,  ptr->Gain_Offset);
			
	
}

void spk_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		
		if(xcmp->u8[4])
		{
			Speaker_is_unmute = 1;
			
			//Silent_flag = 1;
		}
		log("spk OK -st%2x", xcmp->u8[4] );
		
	}
	else
	{
		Speaker_is_unmute = 0;
		log("spk error");
	}
}

void spk_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[3] == xcmp_Res_Success)//0x0000:mute
	{
		Speaker_is_unmute =0;
		//Silent_flag = 0;
		log("spk_s_close ");
		
		
	}
	else
	{
		//Silent_flag = 1;
		Speaker_is_unmute = 1;
		log("spk_s_open ");	
		
	}
	
	
	
	
}


void Volume_reply_func(xcmp_fragment_t * xcmp)
{
	
	/*point to xcmp payload*/
	VolumeControl_reply_t *ptr = (VolumeControl_reply_t* )xcmp->u8;
	
		if (ptr->Result == xcmp_Res_Success)
		{
			if (ptr->Function == Enable_IntelligentAudio)
			{
				log("\n\r Enable_IA OK \n\r");
				log("\n\r Attenuator_Number: %x \n\r",  ((ptr->Attenuator_Number[0]<<8) | (ptr->Attenuator_Number[1])) );
	
			}
			else
			{
				
				log("\n\r VolumeControl: %x \n\r", ptr->Function);
				
			}
			
			
		}
		
		else
		{
			log("\n\r Enable_IA error \n\r");
		}
	
	
	
}

void Volume_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	VolumeControl_brdcst_t *ptr = (VolumeControl_brdcst_t* )xcmp->u8;
	
	//log("Attenuator_Number: %x \n",  ((ptr->Attenuator_Number[0]<<8) | (ptr->Attenuator_Number[1])) );
	
	//log("Audio_Parameter: %x \n", ptr->Audio_Parameter);
	
	
}


void AudioRoutingControl_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		log("AudioRouting OK");
		//xcmp_IdleTestTone();//提示通道配置成功
		//xcmp_IdleTestTone();
		//xcmp_IdleTestTone();
		//Speaker_is_unmute = 1;
	}
	else
	{
		log("AudioRouting error");
		//log("\n\r AudioRouting result: %x \n\r", xcmp->u8[0]);
		
	}
}


void AudioRoutingControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	U16 num_routings = 0;
	U8 j = 0 ;
	
	num_routings = ((xcmp->u8[0]<< 8) | (xcmp->u8[1]) );
	//log("\n\r num_routings: %d \n\r", num_routings);
	
	//for(j = 0; j< num_routings ; j++ )
	//{
		//
		//
		//log("\n\r Audio-Input: %x \n\r", xcmp->u8[2+j*2]);
		//log("\n\r Audio-Output: %x \n\r", xcmp->u8[3+j*2]);
		//
		//
	//}
	//
	//log("\n\r Audio-Function: %x \n\r", xcmp->u8[3+j*2-1]);
	
	
	
}




void TransmitControl_reply_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	TransmitControl_reply_t *ptr = (TransmitControl_reply_t* )xcmp->u8;
	
	if (ptr->Result == xcmp_Res_Success)
	{
		
		log("\n\r  TransmitControl OK \n\r ");
		log("\n\r Function: %x \n\r", ptr->Function);
		log("\n\r Mode of Operation: %x \n\r", ptr->Mode_Of_Operation);
		log("\n\r State: %x \n\r", ptr->State);
		
		if (ptr->Function == KEY_UP)
		{
			//Speaker_is_unmute = 1;
		}
		else if (ptr->Function ==DE_KEY)
		{
			//Speaker_is_unmute = 0;
		}
		else
		{
				//Silent_flag = 1;;
		}
		
		
		//Silent_flag = 1;
	}
	else
	{
		log("TransmitControl error");
	}
	

}


void TransmitControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	//Speaker_is_unmute = 1;
	
	TransmitControl_brdcast_t *ptr = (TransmitControl_brdcast_t* )xcmp->u8;
	//log("\n\r  TransmitControl broadcast \n\r ");
	//log("\n\r  Mode_Of_Operation: %x \n\r ", ptr->Mode_Of_Operation );
	if (ptr->State == 0x00)
	{
		log("\n\r  Standby-Receive \n\r ");
		Radio_Transmit_State = 0;
	}
	if (ptr->State == 0x01)
	{
		log("\n\r  Transmit \n\r ");
		Radio_Transmit_State = 1;
		
	}
	//log("\n\r  State: %x \n\r ", ptr->State );
	//log("\n\r  State_change_reason: %x \n\r ", ptr->State_change_reason );
	//
	
	
}


void CallControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	//Speaker_is_unmute = 1;
	
	CallControl_brdcst_t *ptr = (CallControl_brdcst_t* )xcmp->u8;
	//log("\n\r  CallControl brst \n\r ");
	//log("\n\r  Call_type: %x \n\r ", ptr->Calltype );
	log("\n\r  Call_state: %x \n\r ", ptr->Callstate );
	if (ptr->Callstate == Call_Ended)//0x03
	{
		//恢复正常语音路径通道
		//xcmp_audio_route_revert();
		Call_Begin = 0;
		
	}
	
	if (ptr->Callstate == Call_Initiated)//0x04
	{
		Call_Begin = 1;
	}
	if (ptr->Callstate == Call_Decoded)//0x01
	{
		//配置解密同道
		//xcmp_audio_route_decoder_AMBE();

	}
	
	
}

void DataSession_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		log("DATArep OK \n");
		//log("Func: %X \n", xcmp->u8[1]);
		//log("ID: %X \n", xcmp->u8[2]);
		
	}
	else
	{	
		log("DATArep error \n");
		log("Result:  %X \n", xcmp->u8[0]);
		log("Func:  %X \n", xcmp->u8[1]);
		log("ID:  %X \n", xcmp->u8[2]);
	}
	
}
void ShutDown_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == Shut_Down_Device)
	{
		log("Shut_Down_Device \n");
		
	}
	else if(xcmp->u8[0] == Reset_Device)
	{
		log("Reset_Device \n");
	}
	
	
}


void BatteryLevel_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	BatteryLevel_brdcast_t *ptr = (BatteryLevel_brdcast_t* )(xcmp->u8);
	if(ptr->State == Battery_Okay)
		;//log("\n Battery Okay\n");
	else
		log("\n Battery Low !!!\n");
		
	//log("\n Battery charge: %X \n" , ptr->Charge);
	//log("\n Battery voltage: %X \n" , ptr->Voltage);
	
	Battery_Flag = ptr->State;

}

void DataSession_brdcst_func(xcmp_fragment_t * xcmp)
{
	U8 Session_number = 0;
	U16 data_length = 0;
	U32 card_id =0;
	U8 i = 0;
	xgflash_status_t return_value = XG_ERROR;
	
	/*point to xcmp payload*/
	DataSession_brdcst_t *ptr = (DataSession_brdcst_t* )xcmp->u8;
	
	if (ptr->State == CSBK_DATA_RX_Suc)
	{
		
		log("\n\r CSBK_RX OK \n\r");
		Session_number = ptr->DataPayload.Session_ID_Number;//xcmp->u8[1];
		
		data_length = (ptr->DataPayload.DataPayload_Length[0]<<8) | (ptr->DataPayload.DataPayload_Length[1]);//( xcmp->u8[2]<<8) | (xcmp->u8[3]);

		log("\n\r Session_ID: %x \n\r",Session_number );
		log("\n\r paylaod_length: %d \n\r",data_length );
		for(i=0; i<data_length; i++)
		{
			
			//log("\n\r payload[%d]: %X \n\r", i, xcmp->u8[4+i]);
			log("\n\r payload[%d]: %X \n\r", i, ptr->DataPayload.DataPayload[i]);
			
		}
		
	}
	else
	{
		//log("\n\r State: 0x %X \n\r", xcmp->u8[0]);
		Session_number = ptr->DataPayload.Session_ID_Number;//xcmp->u8[1];				
		data_length = (ptr->DataPayload.DataPayload_Length[0]<<8) | (ptr->DataPayload.DataPayload_Length[1]);//( xcmp->u8[2]<<8) | (xcmp->u8[3]);
		log("State: %X \n", ptr->State);
		if (ptr->State == DATA_SESSION_TX_Suc)
		{
			log("data transmit success\n");
			vTaskDelay(1000*2 / portTICK_RATE_MS);//延迟1000ms
			//xcmp_IdleTestTone(Tone_Start, BT_Connection_Success_Tone);//set tone to indicate connection success!!!
		}
		else if(ptr->State == DATA_SESSION_TX_Fail)
		{
			//Message_Protocol_t  xgmessage;
			//memcpy(&xgmessage, ptr->DataPayload.DataPayload, sizeof(Message_Protocol_t));
//
			//Message_Protocol_t * myptr = get_message_store();	
			//if(NULL != myptr)
			//{
				//memcpy(myptr, &xgmessage, sizeof(Message_Protocol_t));			
				////xQueueSend(xg_resend_queue, &myptr, 0);			
				//if (xQueueSend(xg_resend_queue, &myptr, 0) != pdPASS)
				//{
					//log("xg_resend_queue: full\n" );
					//xcmp_IdleTestTone(Tone_Start, Dispatch_Busy);//set tone to indicate queue full!!!
					//vTaskDelay(3000*2 / portTICK_RATE_MS);//延迟3000ms
					//xcmp_IdleTestTone(Tone_Stop, Dispatch_Busy);//set tone to indicate queue full!!!
				//}
				//else{
					//
					//xSemaphoreTake(count_mutex, portMAX_DELAY);
					//global_count++;
					//xSemaphoreGive(count_mutex);
				//}
			//}
			//else
			//{
				//log("myptr: err\n\r" );
			//}
			//xcmp_IdleTestTone(Tone_Start, MANDOWN_DISABLE_TONE);//set tone to indicate send-failure!!!
		}
		
		/* 'Give' the semaphore to unblock the task. */
		//xSemaphoreGive(xBinarySemaphore);
				
		//log("Session_ID: %x \n\r",Session_number );
		//log("paylaod_length: %d \n\r",data_length );
		//for(i=0; i<data_length; i++)
		//{
				//
			////log("\n\r payload[%d]: %X \n\r", i, xcmp->u8[4+i]);
			//log("\n\r payload[%d]: %X \n\r", i, ptr->DataPayload.DataPayload[i]);
				//
		//}
		
	}
	
}

void ButtonConfig_reply_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	ButtonConfig_reply_t *ptr = (ButtonConfig_reply_t* )(xcmp->u8);
	if (ptr->Result == xcmp_Res_Success)
	{
		log("\n\r Button_Config OK \n\r");
		
		log("\n\r Function: %X \n\r" , ptr->Function );
		
	}
	
	else
	{
		log("\n\r Button_Request error \n\r");
	}
	
}


void Phyuserinput_brdcst_func(xcmp_fragment_t * xcmp)
{
	U8 PUI_Source =0;
	U8 PUI_Type =0;
	U16 PUI_ID =0;
	U8 PUI_State =0;
	U8 PUI_State_Min_Value =0;
	U8 PUI_State_Max_Value =0;
	
	PUI_Source = xcmp->u8[0];
	PUI_Type = xcmp ->u8[1];
	PUI_ID = ((xcmp->u8[2]<<8) | xcmp->u8[3]);
	PUI_State = xcmp->u8[4];
	PUI_State_Min_Value = xcmp->u8[5];
	PUI_State_Max_Value = xcmp->u8[6];
	
	log("PhysicalUserInput_broadcast  \n\r"  );
	
	if((PUI_ID == 0x0060) && (PUI_State = 0x02) && (connect_flag == 1)){
		//log("send message\n");
		xcmp_IdleTestTone(Tone_Start, Ring_Style_Tone_9);//set tone to indicate the scan!!!
			
		vTaskDelay(2500*2 / portTICK_RATE_MS);//延迟1000ms
		//delay_ms(200);
		rfid_sendID_message();//send message	
		//scan_rfid_save_message();//scan and save message	
	}
	//log("\n\r PUI_Source: %X \n\r" , PUI_Source);
	//log("\n\r PUI_Type: %X \n\r" , PUI_Type);
	//log("\n\r PUI_ID: %X \n\r" , PUI_ID);
	//log("\n\r PUI_State: %X \n\r" , PUI_State);
	//log("\n\r PUI_State_Min_Value: %X \n\r" , PUI_State_Min_Value);
	//log("\n\r PUI_State_Max_Value: %X \n\r" , PUI_State_Max_Value);
	
}


void ButtonConfig_brdcst_func(xcmp_fragment_t * xcmp)
{
	U8 Num_Button =0;
	U8 i = 0 ;
	/*point to xcmp payload*/
	ButtonConfig_brdcst_t  *ptr = (ButtonConfig_brdcst_t* )xcmp->u8;
	
	Num_Button = ptr->NumOfButtons;
	
	log("\n\r ButtonConfig_broadcast  \n\r"  );
	
	log("\n\r Function: %X \n\r" , ptr->Function );
	
	log("\n\r NumOfButtons: %d \n\r" , Num_Button );
	
	log("\n\r ButtonInfoStructSize: %x \n\r" , ptr->ButtoInfoStructSize );
	
	for (i; i<Num_Button; i++)
	{
		log("\n\r ButtonInfo[%d].Bt_Identifier: %x \n\r" , i, 
				(ptr->ButtonInfo[i].ButtonIdentifier[0]<<8) | (ptr->ButtonInfo[i].ButtonIdentifier[1]) );
				
		log("\n\r ButtonInfo[%d].S_PressFeature: %x \n\r" , i,
				 (ptr->ButtonInfo[i].ShortPressFeature[0]<<8 )| (ptr->ButtonInfo[i].ShortPressFeature[1]) );
				 
		log("\n\r ButtonInfo[%d].Reserved1: %x \n\r" , i, 
				(ptr->ButtonInfo[i].Reserved1[0]<<8) |  (ptr->ButtonInfo[i].Reserved1[1]));
		
		log("\n\r ButtonInfo[%d].L_PressFeature: %x \n\r" , i,
				 (ptr->ButtonInfo[i].LongPressFeature[0]<<8) | (ptr->ButtonInfo[i].LongPressFeature[1]));
				 
		
		log("\n\r ButtonInfo[%d].Reserved2: %x \n\r" , i, 
				(ptr->ButtonInfo[i].Reserved2[0]<<8) | (ptr->ButtonInfo[i].Reserved2[1]));
		
	}
	

	
}


void SingleDetection_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == 0x11)
	{
		log("\n\r DMR_CSBK OK \n\r");
		get_time_okay = TRUE;
		
	}
	//if(xcmp->u8[1] == 0x11)
	else
	{
		log("SIGBRCST error\n");
		log("\Signal_type: %X \n\r", xcmp->u8[0] );
	}
	

	//;
}



void EnOB_reply_func(xcmp_fragment_t * xcmp)
{
		/*point to xcmp payload*/
	//En_OB_Control_reply_t *ptr = (En_OB_Control_reply_t* )xcmp->u8;
	//log("\n\r Xcmp_opcode: %x \n\r", xcmp->xcmp_opcode);
	
	if (xcmp->u8[0]== xcmp_Res_Success)
	{
		if (xcmp->u8[1] == EN_OB_Enter)
		{
		
			log("\n\r En_OB_Enter OK \n\r");
			
		}
		else if (xcmp->u8[1] == EN_OB_Exit )
		{
			log("\n\r En_OB_Exit OK \n\r");
		}
		else
		{
			
			log("\n\r En_OB_Control: %x \n\r", xcmp->u8[1]);
		}
		
	}
	
	else
	{
		log("\n\r En_OB_Control error \n\r");
		log("\n\r En_OB_result: %x \n\r", xcmp->u8[0]);
		
	}
	
	
}

void EnOB_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	
	log("\n\r En_OB Broadcast \n\r");
}



void FD_request_func(xcmp_fragment_t * xcmp)
{
	
	log("\n\r Forward Data Request \n\r");
	
	
}

void FD_reply_func(xcmp_fragment_t * xcmp)
{
	
	log("\n\r Forward Data Reply \n\r");
	
	
}

void FD_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	
	log("\n\r Forward Data Broadcast \n\r");
	
}



static const volatile app_exec_t the_app_list[MAX_APP_FUNC]=
{
    /*XCMP_REQUEST,XCMP_REPLY,XCMP_BOARDCAST-*/
    {NULL, NULL, DeviceInitializationStatus_brdcst_func},// 0x400 -- Device Initialization Status
    {NULL, NULL, NULL},// 0x401 -- Display Text
    {NULL, NULL, NULL},// 0x402 -- Indicator Update
    {NULL, NULL, NULL},// 0x403 --
    {NULL, NULL, NULL},// 0x404 --
    {NULL, NULL, Phyuserinput_brdcst_func},// 0x405 -- Physical User Input Broadcast
    {NULL, Volume_reply_func, Volume_brdcst_func},// 0x406 -- Volume Control
    {NULL, spk_reply_func, spk_brdcst_func},// 0x407 -- Speaker Control
    {NULL, NULL, NULL},// 0x408 -- Transmit Power Level
    {NULL, ToneControl_reply_func, NULL},// 0x409 -- Tone Control
    {NULL, NULL, ShutDown_brdcst_func},// 0x40A -- Shut Down
    {NULL, NULL, NULL},// 0x40B --
    {NULL, NULL, NULL},// 0x40C -- Monitor Control
    {NULL, NULL, NULL},// 0x40D -- Channel Zone Selection
    {NULL, mic_reply_func, mic_brdcst_func},// 0x40E -- Microphone Control
    {NULL, NULL, NULL},// 0x40F -- Scan Control
    {NULL, NULL, BatteryLevel_brdcst_func},// 0x410 -- Battery Level
    {NULL, NULL, NULL},// 0x411 -- Brightness
    {NULL, ButtonConfig_reply_func, ButtonConfig_brdcst_func},// 0x412 -- Button Configuration
    {NULL, NULL, NULL},// 0x413 -- Emergency Control
    {NULL, AudioRoutingControl_reply_func, AudioRoutingControl_brdcst_func},// 0x414 -- Audio Routing Control
    {NULL, TransmitControl_reply_func, TransmitControl_brdcst_func},// 0x415 -- Transmit Control
    {NULL, NULL, NULL},// 0x416 --
    {NULL, NULL, NULL},// 0x417 --
    {NULL, NULL, NULL},// 0x418 --
    {NULL, NULL, NULL},// 0x419 --
    {NULL, NULL, NULL},// 0x41A --
    {NULL, NULL, SingleDetection_brdcst_func},// 0x41B -- Signal Detection Broadcast
    {NULL, NULL, NULL},// 0x41C -- Remote Radio Control
    {NULL, DataSession_reply_func, DataSession_brdcst_func},// 0x41D -- Data Session
    {NULL, NULL, CallControl_brdcst_func},// 0x41E -- Call Control
    {NULL, NULL, NULL},// 0x41F -- Menu or List Navigation
    {NULL, NULL, NULL},// 0x420 -- Menu Control
    {NULL, dcm_reply_func, dcm_brdcst_func},// 0x421 -- Device Control Mode
    {NULL, NULL, NULL},// 0x422 -- Display Mode Control
    {NULL, NULL, NULL},// 0x423 --
    {NULL, NULL, NULL},// 0x424 --
    {NULL, NULL, NULL},// 0x425 --
    {NULL, NULL, NULL},// 0x426 --
    {NULL, NULL, NULL},// 0x427 --
    {NULL, NULL, DeviceManagement_brdcst_func},// 0x428 -- Device Management
    {NULL, NULL, NULL},// 0x429 --
    {NULL, NULL, NULL},// 0x42A --
    {NULL, NULL, NULL},// 0x42B --
    {NULL, NULL, NULL},// 0x42C --
    {NULL, NULL, NULL},// 0x42D -- Sub-audible Deviation
    {NULL, NULL, NULL},// 0x42E -- Radio Alarm Control
    {NULL, NULL, NULL},// 0x42F --
    {NULL, NULL, NULL},// 0x430 --
    {NULL, NULL, NULL},// 0x431 --
    {NULL, NULL, NULL},// 0x432 --
    {NULL, NULL, NULL},// 0x433 --
    {NULL, NULL, NULL},// 0x434 --
    {NULL, NULL, NULL},// 0x435 --
    {NULL, NULL, NULL},// 0x436 --
    {NULL, NULL, NULL},// 0x437 --
    {NULL, NULL, NULL},// 0x438 --
    {NULL, NULL, NULL},// 0x439 -- Pin Control
    {NULL, NULL, NULL},// 0x43A --
    {NULL, NULL, NULL},// 0x43B --
    {NULL, NULL, NULL},// 0x43C --
    {NULL, NULL, NULL},// 0x43D --
    {NULL, NULL, NULL},// 0x43E -- Bluetooth Status
    {NULL, NULL, NULL},// 0x43F --
    {NULL, NULL, NULL},// 0x440 --
    {NULL, NULL, NULL},// 0x441 --
    {NULL, NULL, NULL},// 0x442 --
    {NULL, NULL, NULL},// 0x443 --
    {NULL, NULL, NULL},// 0x444 --
    {NULL, NULL, NULL},// 0x445 --
    {NULL, NULL, NULL},// 0x446 --
    {NULL, NULL, NULL},// 0x447 --
    {NULL, NULL, NULL},// 0x448 -- Voice Announcement Control
    {NULL, NULL, NULL},// 0x449 -- Keypad Lock Control
    {NULL, NULL, NULL},// 0x44A --
    {NULL, NULL, NULL},// 0x44B -- Radio Wide Parameter Control
    {NULL, NULL, NULL},// 0x44C --
    {NULL, NULL, NULL},// 0x44D --
    {NULL, NULL, NULL},// 0x44E -- Screen Saver
    {NULL, NULL, NULL},// 0x44F --
	{NULL, NULL, NULL},// 0x450 --
	{NULL, NULL, NULL},// 0x451 --
	{NULL, NULL, NULL},// 0x452 --
	{NULL, NULL, NULL},// 0x453 --
	{NULL, NULL, NULL},// 0x454 --
	{NULL, NULL, NULL},// 0x455 --
	{NULL, NULL, NULL},// 0x456 --
	{NULL, NULL, NULL},// 0x457 --
	{FD_request_func, FD_reply_func ,FD_brdcst_func},// 0x458 -- Forward Data
	{NULL, NULL, NULL},// 0x459 --
	{NULL, NULL, NULL},// 0x45A --
	{NULL, NULL, NULL},// 0x45B --
	{NULL, NULL, NULL},// 0x45C --
	{NULL, NULL, NULL},// 0x45D --
	{NULL, NULL, NULL},// 0x45E --
	{NULL, NULL, NULL},// 0x45F --
	{NULL, NULL, NULL},// 0x460 --
	{NULL, NULL, NULL},// 0x461 --
	{NULL, NULL, NULL},// 0x462 --
	{NULL, NULL, NULL},// 0x463 --
	{NULL, NULL, NULL},// 0x464 --
	{NULL, EnOB_reply_func, EnOB_brdcst_func},// 0x465 --Enhanced Option Board Mode
	{NULL, NULL, NULL},// 0x466 --
    {NULL, NULL, NULL},// 0x467 --
	{NULL, NULL, NULL},// 0x468 --
	{NULL, NULL, NULL},// 0x469 --	
														
		
};

U8 message[20]={
	0x00,0x0a,
	0xe0,0x00,
	0x97,0x04,//递增
	0x0d,0x00,
	0x0a,0x00,
		
	0x37,
	0x00//CSBK munufactturing ID.byte2
	
	//0xe0,0x81,0x04,//;header
	//0x00,0x39


};
void app_init(void)
{	
	xcmp_register_app_list(the_app_list);
	
	//将app_payload_rx_proc更改为PCM加密功能
	payload_init( app_payload_rx_proc , app_payload_tx_proc );
	
	/* Create the mutex semaphore to guard a shared global_count.*/
	count_mutex = xSemaphoreCreateMutex();
	if (count_mutex == NULL)
	{
		log("Create the count_mutex semaphore failure\n");
	}
	
	
	static portBASE_TYPE res = 0;
	 res = xTaskCreate(
	app_cfg
	,  (const signed portCHAR *)"USER_P"
	,  800//1024//800//384
	,  NULL
	,  1
	,  NULL );
		
}

extern  char AudioData[];
extern U32 tc_tick;
extern volatile DateTime_t Current_time;
extern volatile  xTaskHandle save_handle; 
extern void xnl_send_device_master_query(void);
//extern portTickType water_value;
//extern portTickType tx_water_value;
//extern portTickType log_water_value;
static __app_Thread_(app_cfg)
{
	static int coun=0;

	static U16 message_count =0;
	U16 TONE_ID = Ring_Style_Tone_8;
	U32 destination = DEST;
	static  portTickType xLastWakeTime;
	const portTickType xFrequency = 4000;//2s,定时问题已经修正。2s x  2000hz = 4000
	U8 Burst_ID = 0;
	char card_id[4]={0};
	U16  * data_ptr;
	Message_Protocol_t *m_buff = (Message_Protocol_t *) pvPortMalloc(sizeof(Message_Protocol_t));
	static	OB_States OB_State = OB_UNCONNECTEDWAITINGSTATUS;
	static xgflash_status_t status = XG_ERROR;
	xLastWakeTime = xTaskGetTickCount();
	
	static const uint8_t test_data[8] = {0x11, 0x23, 0x33, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	char str[80];
	memset(str, 0x00, 80);
	
	
	/* 'Give' the semaphore to unblock the task. */
	 //if( xBinarySemaphore != NULL ){
		//xSemaphoreGive(xBinarySemaphore);
	 //}
	 xSemaphoreTake(xBinarySemaphore, portMAX_DELAY); 
		
	for(;;)
	{
		switch(OB_State)
		{
			case OB_UNCONNECTEDWAITINGSTATUS:
			
				if (0x00000003 == (bunchofrandomstatusflags & 0x00000003) && (!connect_flag))//确认连接成功了，再发送请求
				{
					connect_flag=1;
					xcmp_IdleTestTone(Tone_Start, Priority_Beep);//set tone to indicate connection success!!!
					xcmp_IdleTestTone(Tone_Start, Priority_Beep);//set tone to indicate connection success!!!
					OB_State = OB_WAITINGAPPTASK;
					log("connect OB okay!\n");
					log("XCMP_Version: %d.%d.%d.%d\n", XCMP_Version[0],  XCMP_Version[1],
													 XCMP_Version[2],  XCMP_Version[3]);
					log("OB_Firmware_Version: %d.%d.%d\n", OB_Firmware_Version[0],  OB_Firmware_Version[1], OB_Firmware_Version[2]);							 
				}
				else
				{
					nop();
					nop();
					nop();
					/*send device_master_query to connect radio*/
					xnl_send_device_master_query();
					log("connecting...\n");
					vTaskDelayUntil( &xLastWakeTime, (1000*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
					
					//log("Current time is :20%d:%2d:%2d, %2d:%2d:%2d\n",
					//Current_time.Year, Current_time.Month, Current_time.Day,
					//Current_time.Hour, Current_time.Minute, Current_time.Second);
				}
								
			break;
			case OB_WAITINGAPPTASK:
			
					//log("TONE_ID: %x\n", TONE_ID);		
					//xcmp_IdleTestTone(Tone_Start, TONE_ID);
					//vTaskDelayUntil( &xLastWakeTime, (5000*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
					//xcmp_IdleTestTone(Tone_Stop, TONE_ID);
					//TONE_ID++;
					//if(TONE_ID > 0x004f)TONE_ID = Ring_Style_Tone_8;
						
					
					//if (xSemaphoreTake(xBinarySemaphore, (1000*2) / portTICK_RATE_MS) == pdPASS)
					{
						
						//if(pdPASS == xQueueReceive(xg_resend_queue, &data_ptr, (1000*2) / portTICK_RATE_MS))
						//{
							//if(data_ptr!=NULL){//resend message
							//
								//log("receive Okay!\n");	
								//xSemaphoreTake(count_mutex, portMAX_DELAY);
								//global_count--;
								//xSemaphoreGive(count_mutex);
								//log("global_count:%d\n", global_count);	
																			//
								//xcmp_data_session_req(data_ptr, sizeof(Message_Protocol_t), DEST);	
								//if(xSemaphoreTake(xBinarySemaphore, (20000*2) / portTICK_RATE_MS) ==pdFALSE)
								//{
									//
									//if (xQueueSend(xg_resend_queue, &data_ptr, 0) != pdPASS)
									//{
										//log("xg_resend_queue: full\n" );
										//xcmp_IdleTestTone(Tone_Start, Dispatch_Busy);//set tone to indicate queue full!!!
										//vTaskDelay(3000*2 / portTICK_RATE_MS);//延迟3000ms
										//xcmp_IdleTestTone(Tone_Stop, Dispatch_Busy);//set tone to indicate queue full!!!
									//}
									//else{
											//
										//xSemaphoreTake(count_mutex, portMAX_DELAY);
										//global_count++;
										//xSemaphoreGive(count_mutex);
										//xcmp_IdleTestTone(Tone_Start, MANDOWN_DISABLE_TONE);//set tone to indicate send-failure!!!
									//}
				//
								//}	
								//else
								//{
									//set_message_store(data_ptr);
									//log("send message\n");
								//}						
				//
								////vTaskDelayUntil( &xLastWakeTime, (5000*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
							//
							//}
						//
						//}
					}
					
					if (0x00000003 != (bunchofrandomstatusflags & 0x00000003))//掉线
					{					
						OB_State = OB_UNCONNECTEDWAITINGSTATUS;
						connect_flag=0;
						log("OB disconnecting!!!\n");
					}
					
					//Disable_interrupt_level(1);
					//flashc_memcpy((void *)0x80061234, (void *)test_data, 7,  true);
					//Enable_interrupt_level(1);
					//if (flashc_is_lock_error() || flashc_is_programming_error())
					//{
						//log("flashc_memcpy-1 err...\n");
					//}
					//else
					//{
						//Disable_interrupt_level(1);
						//flashc_memcpy((void *)str, (void *)0x80061234, 7,  true);
						//Enable_interrupt_level(1);
						//if (flashc_is_lock_error() || flashc_is_programming_error())
						//{
							//log("flashc_memcpy-2 err...\n");
						//}
						//else
						//{
							//for (U8 i=0; i<7;i++)
							//{
								//log("str[%d]: %x\n", i, str[i]);
							//}
							//memset(str, 0x00, 80);
						//}
					//}
					
											
					nop();
					log("app task run!\n");
					
					
				
			break;
			default:
			break;
				
		} //End of switch on OB_State.
		vTaskDelayUntil( &xLastWakeTime, (1000*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
	}
}


static void app_payload_rx_proc(void  * payload)
{
	static  U8 times_counter = 0;
	
	times_counter++;
	if (times_counter == 3)
	{
		times_counter = 0 ;
		log("\n\r w: \n\r");
	}
	//log("\n\r w: \n\r");
	if (AMBE_tx_flag)//本地发送方的mic录音
	{
		//fl_write("AMBEvo.bit", FILE_END, payload, MAX_PAYLOAD_BUFF_SIZE * 2);
	}
	else
	{
		//fl_write("PCMvo.pcm", FILE_END, payload, MAX_PAYLOAD_BUFF_SIZE * 2);
	}
	
	//payload_fragment_t * ptr = (payload_fragment_t *)payload;
	//set_payload_idle(payload);

}


static void app_payload_tx_proc(void  * payload)
{
  log("R");
  
  //if (AMBE_flag)
  //{
	  //fl_read("AMBEvo.bit", FILE_BEGIN, payload, MAX_PAYLOAD_BUFF_SIZE * 2);
  //}
  //else
  //{
	  //fl_read("PCMvo.pcm", FILE_BEGIN, payload, MAX_PAYLOAD_BUFF_SIZE * 2);
  //}
  //
  //
  //set_payload_idle(payload);


}

void vApplicationIdleHook( void )
{
	/* This hook function does nothing but increment a counter. */
	ulIdleCycleCount++;
	
}


