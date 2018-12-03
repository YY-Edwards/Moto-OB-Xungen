/*
 * app.c
 *
 * Created: 2016/3/15 14:17:57
 *  Author: JHDEV2
 */ 
#include "app.h"

#include <xcmp.h>
#include <payload.h>
#include <rtc.h>
#include <physical.h>
#include "string.h"

static __app_Thread_(app_cfg);
static void send_message(void * pvParameters);

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
volatile U8 connect_flag =0; 
static volatile U8 get_time_okay =TRUE; 


/*until receive/transmit payload data*/
static void app_payload_rx_proc(void  * payload);
static void app_payload_tx_proc(void  * payload);

/*the queue is used to storage failure-send message*/
extern volatile xQueueHandle message_storage_queue ;

/*the queue is used to receive failure-send message*/
extern volatile xQueueHandle xg_resend_queue ;

//extern volatile xSemaphoreHandle SendM_CountingSemaphore;
extern volatile  xSemaphoreHandle xBinarySemaphore;
volatile U32 global_count =0;
volatile xSemaphoreHandle count_mutex = NULL;
volatile xnl_information_t xnl_information;
volatile char XCMP_Version[4] ={0};
	
extern volatile xSemaphoreHandle xcsbk_rx_finished_Sem;
extern volatile xQueueHandle usart1_tx_xQueue;


//app func--list

void DeviceInitializationStatus_brdcst_func(xcmp_fragment_t  * xcmp)
{
		/*point to xcmp payload*/
		DeviceInitializationStatus_brdcst_t *ptr = (DeviceInitializationStatus_brdcst_t* )xcmp->u8;
		
		//mylog("DeviceInitializationStatus_brdcst...\n");
		//memcpy(&XCMP_Version[0], (ptr->XCMPVersion), sizeof(XCMP_Version));
		
		if (ptr->DeviceInitType == Device_Init_Complete)
		{
			bunchofrandomstatusflags |= 0x01;  //Need do nothing else.
			//mylog("device init complete...\n");
		}
		else if(ptr->DeviceInitType  == Device_Init_Status)
		{
			bunchofrandomstatusflags  &= 0xFFFFFFFC; //Device Init no longer Complete.
			XCMP_Version[0]= ptr->XCMPVersion[0];
			XCMP_Version[1]= ptr->XCMPVersion[1];
			XCMP_Version[2]= ptr->XCMPVersion[2];
			XCMP_Version[3]= ptr->XCMPVersion[3];
			xcmp_DeviceInitializationStatus_request();
			//mylog("device init request..\n");
		}
		else//Device_Status_Update
		{
			//mylog("DeviceInitType : %x\n", ptr->DeviceInitType);
			//mylog("Device Type : %x\n", ptr->DeviceStatusInfo.DeviceType);
			//mylog("Device Status : %x\n", ptr->DeviceStatusInfo.DeviceStatus[1]);
			//mylog("Descriptor size : %x\n", ptr->DeviceStatusInfo.DeviceDescriptorSize);
			//mylog("Descriptor : %x\n", ptr->DeviceStatusInfo.DeviceDescriptor[0]);
		}
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
			
		//mylog("DeviceManagement_brdcst...\n");
		//mylog("temp: %x\n", temp);
		//mylog("xnl_information.logical_address: %x\n", xnl_information.logical_address);
		if (temp == xnl_information.logical_address)
		{
			if (xcmp->u8[0] == 0x01)
			{
				//Enable Option Board
				bunchofrandomstatusflags |= 0x00000002;
				mylog("Enable Option Board\n");
			}
			else
			{
				//Disable Option Board.
				//mylog("Device State : %d\n", );
				bunchofrandomstatusflags &= 0xFFFFFFFD;
				mylog("Disable Option Board\n");
			}
			//mylog("Function : %d\n", ptr->Function);
			//mylog("Device State : %d\n", ptr->Device_State);
		}	
		//U8 temp = 0;
		//temp  = xcmp->u8[1] << 8;
		//temp |= xcmp->u8[2];
		////if (temp == theXNL_Ctrlr.XNL_DeviceLogicalAddress)
		//{
			//if (xcmp->u8[0] == 0x01)
			//{
				//bunchofrandomstatusflags |= 0x00000002;
			//}
			//else
			//{
				//bunchofrandomstatusflags &= 0xFFFFFFFD;
			//}
		//}
}

void ToneControl_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{		
		mylog("Tone OK\n");
		//fl_write("/test.txt", FILE_END, (void *)"send tone ok\r\n", sizeof("send tone ok\r\n") - 1);
	}
	else
	{
		mylog("Tone error");
	}
}

void dcm_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		if(xcmp->u8[1] == DCM_ENTER)
		{
			mylog("\n\r Dcm-Enter OK \n\r");
			
		}
		else if (xcmp->u8[1] == DCM_EXIT)
		{
			mylog("\n\r Dcm-Exit OK \n\r");
		}
		else
		{
			mylog("\n\r Dcm-Revoke \n\r");
		}
		
		mylog("dcm OK-mo%X", xcmp->u8[3]);
	}
	else
	{
		mylog("dcm error");
	}
}


void dcm_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	/*point to xcmp payload*/
	DeviceControlMode_brdcst_t *ptr = (DeviceControlMode_brdcst_t* )xcmp->u8;
	
	mylog("\n\r Dcm_brdcst \n\r");		
	mylog("\n\r Function: %x \n\r " ,  ptr->Function);
	mylog("\n\r ControlType: %x \n\r " ,  ptr->ControlType);
	mylog("\n\r ControlTypeSize: %x \n\r " ,  ptr->ControlTypeSize);
	
	
}

void mic_reply_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	MicControl_reply_t *ptr = (MicControl_reply_t* )xcmp->u8;
	
	mylog("\n\r Mic_reply \n\r");
	if (ptr->Result == 0x00)
	{
		
		if (ptr->Function == Mic_Disable)
		{
		
			mylog("\n\r Mic_close_ok \n\r " );
			mylog("\n\r Mic_type: %x \n\r " ,  ptr->Mic_Type);
			mylog("\n\r Signaling_type: %x \n\r " ,  ptr->Signaling_Type);
			mylog("\n\r Mic_state: %x \n\r " ,  ptr->Mic_State);
			mylog("\n\r Gain_offset: %x \n\r " ,  ptr->Gain_Offset);
			
		}
		else
		{
			mylog("\n\r Mic_function: %x \n\r ", ptr->Function );
		}
		
		
	}
	else 
	{
		
	
		mylog("\n\r Mic error \n\r");
		
	}
	
	
	
}

void mic_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	MicControl_brdcast_t *ptr = (MicControl_brdcast_t* )xcmp->u8;
	//mylog("\n\r Mic_brdcst \n\r");		
	//mylog("\n\r Mic_type: %x \n\r " ,  ptr->Mic_Type);
	//mylog("\n\r Signal_type: %x \n\r " ,  ptr->Signaling_Type);
	if (ptr->Mic_State == 0x00)
	{
		mylog("\n\r Mic_Disabled \n\r");	
		Mic_is_Enabled = 0;
	} 
	if(ptr->Mic_State == 0x11)
	{
		mylog("\n\r Mic_Enabled \n\r");
		Mic_is_Enabled = 1;
		
		if ((Mic_is_Enabled == 1) && (Call_Begin == 1))
		{
			//配置加密通道
			//xcmp_audio_route_encoder_AMBE();
		}
		
	}
	//mylog("\n\r Mic_state: %x \n\r " ,  ptr->Mic_State);
	//mylog("\n\r Gain_offset: %x \n\r " ,  ptr->Gain_Offset);
			
	
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
		mylog("spk OK -st%2x", xcmp->u8[4] );
		
	}
	else
	{
		Speaker_is_unmute = 0;
		mylog("spk error");
	}
}

void spk_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[3] == xcmp_Res_Success)//0x0000:mute
	{
		Speaker_is_unmute =0;
		//Silent_flag = 0;
		mylog("spk_s_close ");
		
		
	}
	else
	{
		//Silent_flag = 1;
		Speaker_is_unmute = 1;
		mylog("spk_s_open ");	
		
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
				mylog("\n\r Enable_IA OK \n\r");
				mylog("\n\r Attenuator_Number: %x \n\r",  ((ptr->Attenuator_Number[0]<<8) | (ptr->Attenuator_Number[1])) );
	
			}
			else
			{
				
				mylog("\n\r VolumeControl: %x \n\r", ptr->Function);
				
			}
			
			
		}
		
		else
		{
			mylog("\n\r Enable_IA error \n\r");
		}
	
	
	
}

void Volume_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
//	VolumeControl_brdcst_t *ptr = (VolumeControl_brdcst_t* )xcmp->u8;
	
	//mylog("Attenuator_Number: %x \n",  ((ptr->Attenuator_Number[0]<<8) | (ptr->Attenuator_Number[1])) );
	
	//mylog("Audio_Parameter: %x \n", ptr->Audio_Parameter);
	
	
}


void AudioRoutingControl_reply_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == xcmp_Res_Success)
	{
		mylog("AudioRouting OK");
		//xcmp_IdleTestTone();//提示通道配置成功
		//xcmp_IdleTestTone();
		//xcmp_IdleTestTone();
		//Speaker_is_unmute = 1;
	}
	else
	{
		mylog("AudioRouting error");
		//mylog("\n\r AudioRouting result: %x \n\r", xcmp->u8[0]);
		
	}
}


void AudioRoutingControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	U16 num_routings = 0;
//	U8 j = 0 ;
	
	num_routings = ((xcmp->u8[0]<< 8) | (xcmp->u8[1]) );
	//mylog("\n\r num_routings: %d \n\r", num_routings);
	
	//for(j = 0; j< num_routings ; j++ )
	//{
		//
		//
		//mylog("\n\r Audio-Input: %x \n\r", xcmp->u8[2+j*2]);
		//mylog("\n\r Audio-Output: %x \n\r", xcmp->u8[3+j*2]);
		//
		//
	//}
	//
	//mylog("\n\r Audio-Function: %x \n\r", xcmp->u8[3+j*2-1]);
	
	
	
}




void TransmitControl_reply_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	TransmitControl_reply_t *ptr = (TransmitControl_reply_t* )xcmp->u8;
	
	if (ptr->Result == xcmp_Res_Success)
	{
		
		mylog("\n\r  TransmitControl OK \n\r ");
		mylog("\n\r Function: %x \n\r", ptr->Function);
		mylog("\n\r Mode of Operation: %x \n\r", ptr->Mode_Of_Operation);
		mylog("\n\r State: %x \n\r", ptr->State);
		
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
		mylog("TransmitControl error");
	}
	

}


void TransmitControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	//Speaker_is_unmute = 1;
	
	TransmitControl_brdcast_t *ptr = (TransmitControl_brdcast_t* )xcmp->u8;
	//mylog("\n\r  TransmitControl broadcast \n\r ");
	//mylog("\n\r  Mode_Of_Operation: %x \n\r ", ptr->Mode_Of_Operation );
	if (ptr->State == 0x00)
	{
		mylog("\n\r  Standby-Receive \n\r ");
		Radio_Transmit_State = 0;
	}
	if (ptr->State == 0x01)
	{
		mylog("\n\r  Transmit \n\r ");
		Radio_Transmit_State = 1;
		
	}
	//mylog("\n\r  State: %x \n\r ", ptr->State );
	//mylog("\n\r  State_change_reason: %x \n\r ", ptr->State_change_reason );
	//
	
	
}


void CallControl_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	//Speaker_is_unmute = 1;
	
	CallControl_brdcst_t *ptr = (CallControl_brdcst_t* )xcmp->u8;
	//mylog("\n\r  CallControl brst \n\r ");
	//mylog("\n\r  Call_type: %x \n\r ", ptr->Calltype );
	mylog("\n\r  Call_state: %x \n\r ", ptr->Callstate );
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
		mylog("DATArep OK \n");
		//mylog("Func: %X \n", xcmp->u8[1]);
		//mylog("ID: %X \n", xcmp->u8[2]);
		
	}
	else
	{	
		mylog("DATArep error \n");
		mylog("Result:  %X \n", xcmp->u8[0]);
		mylog("Func:  %X \n", xcmp->u8[1]);
		mylog("ID:  %X \n", xcmp->u8[2]);
	}
	
}
void ShutDown_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == Shut_Down_Device)
	{
		mylog("Shut_Down_Device \n");
		
	}
	else if(xcmp->u8[0] == Reset_Device)
	{
		mylog("Reset_Device \n");
	}
	
	
}


void BatteryLevel_brdcst_func(xcmp_fragment_t * xcmp)
{
	/*point to xcmp payload*/
	BatteryLevel_brdcast_t *ptr = (BatteryLevel_brdcast_t* )(xcmp->u8);
	if(ptr->State == Battery_Okay)
		;//mylog("\n Battery Okay\n");
	else
		mylog("\n Battery Low !!!\n");
		
	//mylog("\n Battery charge: %X \n" , ptr->Charge);
	//mylog("\n Battery voltage: %X \n" , ptr->Voltage);
	
	Battery_Flag = ptr->State;

}

extern volatile unsigned char host_flag;
void DataSession_brdcst_func(xcmp_fragment_t * xcmp)
{
	U8 Session_number = 0;
	U16 data_length = 0;
	static  portBASE_TYPE queue_ret = pdPASS;
	static U16 payload_len = 0;
	static U8 payload_count =0;
	static U8 remaining_bytes=0;
	static U8 last_temp[10]={0};
	U16 offset=0;
	//static csbk_rx_state_t rx_status = WAITING_CSBK_P_HEADER;
//	U32 card_id =0;
	U8 i = 0;
//	xgflash_status_t return_value = XG_ERROR;
	
	/*point to xcmp payload*/
	DataSession_brdcst_t *ptr = (DataSession_brdcst_t* )xcmp->u8;
	
	if (ptr->State == CSBK_DATA_RX_Suc)
	{
		
		mylog("\n\r CSBK_RX OK \n\r");
		Session_number = ptr->DataPayload.Session_ID_Number;//xcmp->u8[1];
		
		data_length = (ptr->DataPayload.DataPayload_Length[0]<<8) | (ptr->DataPayload.DataPayload_Length[1]);//( xcmp->u8[2]<<8) | (xcmp->u8[3]);

		//mylog("\n\r Session_ID: %x \n\r",Session_number );
		//mylog("\n\r paylaod_length: %d \n\r",data_length );
		
		if(data_length == sizeof(CSBK_Pro_t))
		{		
			CSBK_Pro_t *csbk_ptr = (CSBK_Pro_t *)(ptr->DataPayload.DataPayload);//将csbk_ptr指向负载数据
			////my_custom_pro_t *custom_pro =  (my_custom_pro_t *)(csbk_ptr->csbk_data);
			////
			////if(//拼接CSBK包
			////(csbk_ptr->csbk_manufacturing_id == CSBK_Third_PARTY)
			////&&(((host_flag == 1) && (csbk_ptr->csbk_header.csbk_opcode == CSBK_Slave_Opcode)) 
				////||((host_flag == 0) && (csbk_ptr->csbk_header.csbk_opcode == CSBK_Host_Opcode)))
				////)	
			//////if (host_flag == 1)//主机
				//////if((csbk_ptr->csbk_manufacturing_id == CSBK_Third_PARTY) && (csbk_ptr->csbk_header.csbk_opcode == CSBK_Slave_Opcode))//接收从机的CSBK数据			
			//////else //从机	
				//////if((csbk_ptr->csbk_manufacturing_id == CSBK_Third_PARTY) && (csbk_ptr->csbk_header.csbk_opcode == CSBK_Host_Opcode))//接收主机的CSBK数据		
							//////
			////{
				////switch(rx_status)
				////{
					////case WAITING_CSBK_P_HEADER:
					////
							////if(custom_pro->header == FIXED_HEADER)
							////{
								////payload_len = ((custom_pro->data_len[0]) | ((custom_pro->data_len[1]<<8) & 0xff00));//获取数据包长度
								////if(payload_len<=8)//判断是否有中间包
								////{
									////rx_status = WAITING_LAST_FRAGMENT;
								////}
								////else
								////{
									////rx_status = READING_MIDDLE_FRAGMENT;
								////}
								////remaining_bytes = payload_len;
							////}				
							//////mylog("WAITING_CSBK_P_HEADER \n\r");
							////break;
					////
					////case READING_MIDDLE_FRAGMENT://注意考虑一种情况：中间包出现重复的情况，考虑出现重复中间包则丢弃。(判断重复中间包的逻辑是否正确？)				
							////
							////if(csbk_ptr->csbk_header.csbk_LB != CSBK_LB_FALSE)//如果中间包丢失，则丢弃
							////{
								////U8 rx_char =0;
								////while((xQueueReceive(usart1_tx_xQueue, &rx_char, 0)) == pdPASS);
								////rx_status = WAITING_CSBK_P_HEADER;
								////payload_len = 0;
								////payload_count =0;
								////remaining_bytes=0;
								////mylog("csbk-middle lost!!! \n\r");
								////break;
							////}
							////
							////if(memcmp(last_temp,  csbk_ptr->csbk_data, 8)==0)
							////{
								////mylog("repeated middle fragment!!!\n\r");
								////break;
							////}
							////memcpy(last_temp, csbk_ptr->csbk_data, 8);					
							//////sendqueue
							////offset =0;
							////do
							////{
								//////若队列满，则丢弃
								////queue_ret = xQueueSendToBack(usart1_tx_xQueue, &(csbk_ptr->csbk_data[offset]), 0);//insert data
								////offset++;
								////
							////} while (offset<8);//拷贝8个数据
							////
							////payload_count+=8;
							////
							////remaining_bytes = payload_len - payload_count;
							////if(remaining_bytes<=8)//判断是否应该等待最后一包数据
							////{
								////rx_status = WAITING_LAST_FRAGMENT;
							////}
							//////mylog("READING_MIDDLE_FRAGMENT \n\r");			
							////break;
					////
					////case WAITING_LAST_FRAGMENT:
							////
								////if(csbk_ptr->csbk_header.csbk_LB == CSBK_LB_TRUE)//最后一包数据)
								////{
									////offset =0;
									////do
									////{
										////queue_ret = xQueueSendToBack(usart1_tx_xQueue, &(csbk_ptr->csbk_data[offset]), 0);//insert data
										////offset++;
									////
									////} while (offset<remaining_bytes);//拷贝剩余数据
	////
									//////触发事件，发送数据到usart1
									////xSemaphoreGive(xcsbk_rx_finished_Sem);
									////xcmp_IdleTestTone(Tone_Start, BT_Connection_Success_Tone);
								////}
								////else//clear 队列，最后一包丢失时则清零队列（有两种情况：中间包有重复包，则数据长度异常；或者最后一包数据丢失）
								////{
									////U8 rx_char =0;
									////while((xQueueReceive(usart1_tx_xQueue, &rx_char, 0)) == pdPASS);
									////mylog("csbk err!!! \n\r");	
								////}
								////
								////rx_status = WAITING_CSBK_P_HEADER;
								////payload_len = 0;
								////payload_count =0;
								////remaining_bytes=0;
								//////mylog("WAITING_LAST_FRAGMENT \n\r");	
							////
							////break;
					////
					////default:
							////break;
					////
				////}
						
			}
			else
			{
				mylog("no my csbk type\n\r");
			}
			
			//mylog("remaining_bytes:%d \n\r", remaining_bytes);	
			//if((csbk_ptr->csbk_manufacturing_id == CSBK_Third_PARTY) && (csbk_ptr->csbk_header.csbk_opcode == CSBK_Opcode))
			//{
				//
				//if (csbk_ptr->csbk_header.csbk_PF == CSBK_PF_TRUE)//第一包数据
				//{
					//payload_len = ((csbk_ptr->csbk_data[0]) | ((csbk_ptr->csbk_data[1]<<8) & 0xff00));//获取数据包长度
				//} 
				//else if(csbk_ptr->csbk_header.csbk_LB == CSBK_LB_TRUE)//最后一包数据
				//{
					//remaining_bytes = payload_len - payload_count;
					//if(remaining_bytes)//非0 
					//{
						//offset =0;
						//do
						//{
							//queue_ret = xQueueSendToBack(usart1_tx_xQueue, &(csbk_ptr->csbk_data[2+offset]), portMAX_DELAY);//insert data
							//offset++;
								//
						//} while (offset<remaining_bytes);//拷贝剩余数据
						//
					//}
					////触发事件，发送数据到usart1
					//xSemaphoreGive(xcsbk_rx_finished_Sem);
				//}
				//else//中间数据包
				//{
					////sendqueue		
					//do
					//{
						//queue_ret = xQueueSendToBack(usart1_tx_xQueue, &(csbk_ptr->csbk_data[2+offset]), portMAX_DELAY);//insert data
						//offset++;
						//
					//} while (offset<sizeof(csbk_ptr->csbk_data));//拷贝8个数据
					//
					//payload_count+=sizeof(csbk_ptr->csbk_data);
				//}
		//
			//}
			//else
			//{
				//mylog("no my csbk type\n\r");
			//}
		
		//}
		//for(i=0; i<data_length; i++)
		//{
			//
			////mylog("\n\r payload[%d]: %X \n\r", i, xcmp->u8[4+i]);
			////mylog("\n\r payload[%d]: %X \n\r", i, ptr->DataPayload.DataPayload[i]);
			//
			//
		//}
		
	}
	else
	{
		//mylog("\n\r State: 0x %X \n\r", xcmp->u8[0]);
		Session_number = ptr->DataPayload.Session_ID_Number;//xcmp->u8[1];				
		data_length = (ptr->DataPayload.DataPayload_Length[0]<<8) | (ptr->DataPayload.DataPayload_Length[1]);//( xcmp->u8[2]<<8) | (xcmp->u8[3]);
		mylog("State: %X \n", ptr->State);
		if (ptr->State == DATA_SESSION_TX_Suc)
		{
			//允许模块向MCU发送数据，并拉低RTS信号
			//usart_enable_receiver(APP_USART);
			mylog("data transmit success\n");
			//vTaskDelay(1000*2 / portTICK_RATE_MS);//延迟1000ms
			xcmp_IdleTestTone(Tone_Start, BT_Connection_Success_Tone);//set tone to indicate connection success!!!
		}
		else if(ptr->State == DATA_SESSION_TX_Fail)
		{
			mylog("data transmit failure\n");
			//Message_Protocol_t  xgmessage;
			//memcpy(&xgmessage, ptr->DataPayload.DataPayload, sizeof(Message_Protocol_t));
			
			//Message_Protocol_t * myptr = get_message_store();	
			//if(NULL != myptr)
			//{
				//memcpy(myptr, &xgmessage, sizeof(Message_Protocol_t));			
				////xQueueSend(xg_resend_queue, &myptr, 0);
				//if (xQueueSend(xg_resend_queue, &myptr, 0) != pdPASS)
				//{
					//mylog("xg_resend_queue: full\n" );
					//xcmp_IdleTestTone(Tone_Start, Dispatch_Busy);//set tone to indicate queue full!!!
					//vTaskDelay(3000*2 / portTICK_RATE_MS);//延迟3000ms
					//xcmp_IdleTestTone(Tone_Stop, Dispatch_Busy);//set tone to indicate queue full!!!
				//}
				//else
				//{
					//xSemaphoreTake(count_mutex, portMAX_DELAY);
					//global_count++;
					//xSemaphoreGive(count_mutex);
				//}
			//}
			//else
			//{
				//mylog("myptr: err\n\r" );
			//}
			xcmp_IdleTestTone(Tone_Start, MANDOWN_DISABLE_TONE);//set tone to indicate send-failure!!!
			
			//resend-csbk
			//xcmp_data_session_csbk_raw_req(ptr->DataPayload.DataPayload, data_length);//最多一次只能发送22个csbk数据包
				
		}
		
		//if((ptr->State == DATA_SESSION_TX_Fail) || (ptr->State == DATA_SESSION_TX_Suc))
		//{		
			////if( xSemaphoreGive( SendM_CountingSemaphore ) != pdTRUE )
			//if( xSemaphoreGive( xBinarySemaphore ) != pdTRUE )
			//{
				//mylog("xSemaphoreGive: err\n\r" );
			//}
		//}
		
			
			
		//mylog("Session_ID: %x \n\r",Session_number );
		//mylog("paylaod_length: %d \n\r",data_length );
		//for(i=0; i<data_length; i++)
		//{
				//
			////mylog("\n\r payload[%d]: %X \n\r", i, xcmp->u8[4+i]);
			//mylog("\n\r payload[%d]: %X \n\r", i, ptr->DataPayload.DataPayload[i]);
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
		mylog("\n\r Button_Config OK \n\r");
		
		mylog("\n\r Function: %X \n\r" , ptr->Function );
		
	}
	
	else
	{
		mylog("\n\r Button_Request error \n\r");
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
	
	mylog("PhysicalUserInput_broadcast  \n\r"  );
	
	if((PUI_ID == 0x0060) && (PUI_State = 0x02) && (connect_flag == 1)){
		//mylog("send message\n");
		xcmp_IdleTestTone(Tone_Start, Ring_Style_Tone_9);//set tone to indicate the scan!!!
			
		vTaskDelay(1000*2 / portTICK_RATE_MS);//延迟1000ms
		//delay_ms(200);
		rfid_sendID_message();//send message		
		//scan_rfid_save_message();
	}
	//mylog("\n\r PUI_Source: %X \n\r" , PUI_Source);
	//mylog("\n\r PUI_Type: %X \n\r" , PUI_Type);
	//mylog("\n\r PUI_ID: %X \n\r" , PUI_ID);
	//mylog("\n\r PUI_State: %X \n\r" , PUI_State);
	//mylog("\n\r PUI_State_Min_Value: %X \n\r" , PUI_State_Min_Value);
	//mylog("\n\r PUI_State_Max_Value: %X \n\r" , PUI_State_Max_Value);
	
}


void ButtonConfig_brdcst_func(xcmp_fragment_t * xcmp)
{
	U8 Num_Button =0;
	U8 i = 0 ;
	/*point to xcmp payload*/
	ButtonConfig_brdcst_t  *ptr = (ButtonConfig_brdcst_t* )xcmp->u8;
	
	Num_Button = ptr->NumOfButtons;
	
	mylog("\n\r ButtonConfig_broadcast  \n\r"  );
	
	mylog("\n\r Function: %X \n\r" , ptr->Function );
	
	mylog("\n\r NumOfButtons: %d \n\r" , Num_Button );
	
	mylog("\n\r ButtonInfoStructSize: %x \n\r" , ptr->ButtoInfoStructSize );
	
	for (; i<Num_Button; i++)
	{
		mylog("\n\r ButtonInfo[%d].Bt_Identifier: %x \n\r" , i, 
				(ptr->ButtonInfo[i].ButtonIdentifier[0]<<8) | (ptr->ButtonInfo[i].ButtonIdentifier[1]) );
				
		mylog("\n\r ButtonInfo[%d].S_PressFeature: %x \n\r" , i,
				 (ptr->ButtonInfo[i].ShortPressFeature[0]<<8 )| (ptr->ButtonInfo[i].ShortPressFeature[1]) );
				 
		mylog("\n\r ButtonInfo[%d].Reserved1: %x \n\r" , i, 
				(ptr->ButtonInfo[i].Reserved1[0]<<8) |  (ptr->ButtonInfo[i].Reserved1[1]));
		
		mylog("\n\r ButtonInfo[%d].L_PressFeature: %x \n\r" , i,
				 (ptr->ButtonInfo[i].LongPressFeature[0]<<8) | (ptr->ButtonInfo[i].LongPressFeature[1]));
				 
		
		mylog("\n\r ButtonInfo[%d].Reserved2: %x \n\r" , i, 
				(ptr->ButtonInfo[i].Reserved2[0]<<8) | (ptr->ButtonInfo[i].Reserved2[1]));
		
	}
	

	
}


void SingleDetection_brdcst_func(xcmp_fragment_t * xcmp)
{
	if (xcmp->u8[0] == 0x11)
	{
		mylog("\n\r DMR_CSBK OK \n\r");
		get_time_okay = TRUE;
		
	}
	//if(xcmp->u8[1] == 0x11)
	else
	{
		mylog("SIGBRCST error\n");
		mylog("Signal_type: %X \n\r", xcmp->u8[0] );
	}
	

	//;
}



void EnOB_reply_func(xcmp_fragment_t * xcmp)
{
		/*point to xcmp payload*/
	//En_OB_Control_reply_t *ptr = (En_OB_Control_reply_t* )xcmp->u8;
	//mylog("\n\r Xcmp_opcode: %x \n\r", xcmp->xcmp_opcode);
	
	if (xcmp->u8[0]== xcmp_Res_Success)
	{
		if (xcmp->u8[1] == EN_OB_Enter)
		{
		
			mylog("\n\r En_OB_Enter OK \n\r");
			
		}
		else if (xcmp->u8[1] == EN_OB_Exit )
		{
			mylog("\n\r En_OB_Exit OK \n\r");
		}
		else
		{
			
			mylog("\n\r En_OB_Control: %x \n\r", xcmp->u8[1]);
		}
		
	}
	
	else
	{
		mylog("\n\r En_OB_Control error \n\r");
		mylog("\n\r En_OB_result: %x \n\r", xcmp->u8[0]);
		
	}
	
	
}

void EnOB_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	
	mylog("\n\r En_OB Broadcast \n\r");
}



void FD_request_func(xcmp_fragment_t * xcmp)
{
	
	mylog("\n\r Forward Data Request \n\r");
	
	
}

void FD_reply_func(xcmp_fragment_t * xcmp)
{
	
	mylog("\n\r Forward Data Reply \n\r");
	
	
}

void FD_brdcst_func(xcmp_fragment_t * xcmp)
{
	
	
	mylog("\n\r Forward Data Broadcast \n\r");
	
}



static const volatile app_exec_t the_app_list[MAX_APP_FUNC]=
{
    /*XCMP_REQUEST,XCMP_REPLY,XCMP_BOARDCAST-*/
    {NULL, NULL, (void *)DeviceInitializationStatus_brdcst_func},// 0x400 -- Device Initialization Status
    {NULL, NULL, NULL},// 0x401 -- Display Text
    {NULL, NULL, NULL},// 0x402 -- Indicator Update
    {NULL, NULL, NULL},// 0x403 --
    {NULL, NULL, NULL},// 0x404 --
    {NULL, NULL, (void *)Phyuserinput_brdcst_func},// 0x405 -- Physical User Input Broadcast
    {NULL, (void *)Volume_reply_func, (void *)Volume_brdcst_func},// 0x406 -- Volume Control
    {NULL, (void *)spk_reply_func, (void *)spk_brdcst_func},// 0x407 -- Speaker Control
    {NULL, NULL, NULL},// 0x408 -- Transmit Power Level
    {NULL, (void *)ToneControl_reply_func, NULL},// 0x409 -- Tone Control
    {NULL, NULL, (void *)ShutDown_brdcst_func},// 0x40A -- Shut Down
    {NULL, NULL, NULL},// 0x40B --
    {NULL, NULL, NULL},// 0x40C -- Monitor Control
    {NULL, NULL, NULL},// 0x40D -- Channel Zone Selection
    {NULL, (void *)mic_reply_func, (void *)mic_brdcst_func},// 0x40E -- Microphone Control
    {NULL, NULL, NULL},// 0x40F -- Scan Control
    {NULL, NULL, (void *)BatteryLevel_brdcst_func},// 0x410 -- Battery Level
    {NULL, NULL, NULL},// 0x411 -- Brightness
    {NULL, (void *)ButtonConfig_reply_func, (void *)ButtonConfig_brdcst_func},// 0x412 -- Button Configuration
    {NULL, NULL, NULL},// 0x413 -- Emergency Control
    {NULL, (void *)AudioRoutingControl_reply_func, (void *)AudioRoutingControl_brdcst_func},// 0x414 -- Audio Routing Control
    {NULL, (void *)TransmitControl_reply_func, (void *)TransmitControl_brdcst_func},// 0x415 -- Transmit Control
    {NULL, NULL, NULL},// 0x416 --
    {NULL, NULL, NULL},// 0x417 --
    {NULL, NULL, NULL},// 0x418 --
    {NULL, NULL, NULL},// 0x419 --
    {NULL, NULL, NULL},// 0x41A --
    {NULL, NULL, (void *)SingleDetection_brdcst_func},// 0x41B -- Signal Detection Broadcast
    {NULL, NULL, NULL},// 0x41C -- Remote Radio Control
    {NULL, (void *)DataSession_reply_func, (void *)DataSession_brdcst_func},// 0x41D -- Data Session
    {NULL, NULL, (void *)CallControl_brdcst_func},// 0x41E -- Call Control
    {NULL, NULL, NULL},// 0x41F -- Menu or List Navigation
    {NULL, NULL, NULL},// 0x420 -- Menu Control
    {NULL, (void *)dcm_reply_func, (void *)dcm_brdcst_func},// 0x421 -- Device Control Mode
    {NULL, NULL, NULL},// 0x422 -- Display Mode Control
    {NULL, NULL, NULL},// 0x423 --
    {NULL, NULL, NULL},// 0x424 --
    {NULL, NULL, NULL},// 0x425 --
    {NULL, NULL, NULL},// 0x426 --
    {NULL, NULL, NULL},// 0x427 --
    {NULL, NULL, (void *)DeviceManagement_brdcst_func},// 0x428 -- Device Management
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
	{(void *)FD_request_func, (void *)FD_reply_func ,(void *)FD_brdcst_func},// 0x458 -- Forward Data
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
	{NULL, (void *)EnOB_reply_func, (void *)EnOB_brdcst_func},// 0x465 --Enhanced Option Board Mode
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
	xcmp_register_app_list((void *)the_app_list);
	
	//将app_payload_rx_proc更改为PCM加密功能
	payload_init( app_payload_rx_proc , app_payload_tx_proc );
	
	/* Create the mutex semaphore to guard a shared global_count.*/
	count_mutex = xSemaphoreCreateMutex();
	if (count_mutex == NULL)
	{
		mylog("Create the count_mutex semaphore failure\n");
	}
	
	static portBASE_TYPE res = 0;
	 res = xTaskCreate(
	app_cfg
	,  (const signed portCHAR *)"USER_P"
	,  900//1024//800//384
	,  NULL
	,  2
	,  NULL );
	
	 //res = xTaskCreate(
	 //send_message
	 //,  (const signed portCHAR *)"SEND_M"
	 //,  800
	 //,  NULL
	 //,  1
	 //,  NULL );
	
}

extern  char AudioData[];
extern U32 tc_tick;
extern volatile DateTime_t Current_time;
extern volatile  xTaskHandle save_handle; 
//extern portTickType water_value;
extern portTickType xnl_rx_water_value;
extern portTickType xcmp_rx_water_value;
extern portTickType xnl_tx_water_value;
extern volatile  portTickType usart1_task_water_value; 
//extern portTickType log_water_value;
static void send_message(void * pvParameters)
{

	static U16 message_count =0;
	U32 destination = DEST;
	static  portTickType xLastWakeTime;
	//const portTickType xFrequency = 4000;//2s,定时问题已经修正。2s x  2000hz = 4000
//	U16  * data_ptr;
	Message_Protocol_t *m_buff = (Message_Protocol_t *) pvPortMalloc(sizeof(Message_Protocol_t));
	static xgflash_status_t status = XG_ERROR;
	
	xLastWakeTime = xTaskGetTickCount();
//	static  portTickType water_value;
	/*clear xBinarySemaphore and wait Datasession broadcast reply*/
	xSemaphoreTake(xBinarySemaphore, portMAX_DELAY);
	
	for (;;)
	{
	
		message_count = xgflash_get_message_count();
		if( (message_count!=0) && (Battery_Flag == Battery_Okay) && (connect_flag))//有缓存且电量充足，需发送短信
		{
			mylog("Current_total_message_count: %d\n", message_count);
			if(m_buff==NULL)break;
			status = xgflash_get_message_data(message_count, m_buff, TRUE);
			if(status == XG_OK)
			{
				xcmp_data_session_req(m_buff, (sizeof(Message_Protocol_t)), destination);//send message
				
				//if(xSemaphoreTake(SendM_CountingSemaphore, (20000*2) / portTICK_RATE_MS) == pdTRUE)
				if(xSemaphoreTake(xBinarySemaphore, (20000*2) / portTICK_RATE_MS) == pdTRUE)
				{
					mylog("xSemaphoreTake okay!\n");
					vTaskDelay((2000*2) / portTICK_RATE_MS);
				}
				else//短信丢失，手台未响应，超时后默认再次重发
				{
					mylog("xSemaphoreTake failure!\n");
					xcmp_IdleTestTone(Tone_Start, MANDOWN_DISABLE_TONE);//set tone to indicate send-failure!!!
					status = xgflash_message_save((U8 *)m_buff, sizeof(Message_Protocol_t), TRUE);
					if(status == XG_OK)
					{
						mylog("save message-2 okay\n");
					}
					else
					{
						mylog("!!!save message err : %d\n", status);
					}
				
				}
			}
			else
			{
				mylog("get message err : %d\n", status);
			}
		
		}
		else if (Battery_Flag == Battery_Low)
		{
			mylog("The device battery level is low !\n");
		}
		
		//water_value = uxTaskGetStackHighWaterMark(NULL);
		//mylog("send-thread water_value: %d\n", water_value);
		
		vTaskDelayUntil(&xLastWakeTime, (5000*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
	
	}
}

static __app_Thread_(app_cfg)
{
//	static int coun=0;
	static int run_counter=0;
	static int rx_csbk_count=0;
	static  portTickType xLastWakeTime;
	static  portBASE_TYPE queue_ret = pdPASS;
//	const portTickType xFrequency = 4000;//2s,定时问题已经修正。2s x  2000hz = 4000
//	U8 Burst_ID = 0;
//	char card_id[4]={0};
	U16  * data_ptr;
	static	OB_States OB_State = OB_UNCONNECTEDWAITINGSTATUS;
	static xgflash_status_t status = XG_ERROR;
	xLastWakeTime = xTaskGetTickCount();
	static  portTickType water_value;
	int i =0;
	int k= 0;
		
	for(;;)
	{
		water_value = uxTaskGetStackHighWaterMark(NULL);
		switch(OB_State)
		{
			case OB_UNCONNECTEDWAITINGSTATUS:
			
				if (0x00000003 == (bunchofrandomstatusflags & 0x00000003) && (!connect_flag))//确认连接成功了，再发送请求
				{
					connect_flag=1;
					xcmp_IdleTestTone(Tone_Start, Priority_Beep);//set tone to indicate connection success!!!
					xcmp_IdleTestTone(Tone_Start, Priority_Beep);//set tone to indicate connection success!!!
					OB_State = OB_CONNECTEDWAITTINGSYNTIME;
					mylog("connect OB okay!\n");
					mylog("XCMP_Version: %d.%d.%d.%d\n", XCMP_Version[0],  XCMP_Version[1],
					XCMP_Version[2],  XCMP_Version[3]);
					mylog("OB_Firmware_Version: %d.%d.%d\n", OB_Firmware_Version[0],  OB_Firmware_Version[1], OB_Firmware_Version[2]);

					
				}
				else
				{
					//package_usartdata_to_csbkdata(test, sizeof(test));
					nop();
					nop();
					nop();
					//xcmp_IdleTestTone(Tone_Start, Bad_Key_Chirp);//set tone to indicate connection failure!!!
					mylog("connecting...\n");
					mylog("Current time is :20%d:%2d:%2d, %2d:%2d:%2d\n",
					Current_time.Year, Current_time.Month, Current_time.Day,
					Current_time.Hour, Current_time.Minute, Current_time.Second);
					/*send device_master_query to connect radio*/
					xnl_send_device_master_query();
				}
								
			break;
			case OB_CONNECTEDWAITTINGSYNTIME:
			
						if(get_time_okay){
							
							OB_State = OB_WAITINGAPPTASK;
							mylog("get time okay!\n");
							//vTaskResume(save_handle);
						}
						else
						{						
							xcmp_data_session_req(0x00, sizeof(Message_Protocol_t), DEST);//request to get system time						
						}
			break;
			case OB_WAITINGAPPTASK:
														
					run_counter++;			
					nop();
					//if(run_counter == 2)新增代码，可能需要增大栈空间分配值
					//{
						////mylog("send test csbk data...\n");
						////package_usartdata_to_csbkdata(test, sizeof(test));
					//}
					mylog("app task run:%d\n", run_counter);
					if(0x00000003 != (bunchofrandomstatusflags & 0x00000003))//可能断开
					{
						connect_flag =0;
						OB_State = OB_UNCONNECTEDWAITINGSTATUS;
					}
				
			break;
			default:
			break;
				
		} //End of switch on OB_State.
		
			//mylog("app-thread water_value: %d\n", water_value);
			//mylog("usart1_task water_value: %d\n", usart1_task_water_value);
			//mylog("xnl rx water_value: %d\n", xnl_rx_water_value);
			//mylog("xnl tx water_value: %d\n", xnl_tx_water_value);
			//mylog("xcmp rx water_value: %d\n", xcmp_rx_water_value);
		
		//vTaskDelay(300*2 / portTICK_RATE_MS);//延迟300ms
		//mylog("\n\r ulIdleCycleCount: %d \n\r", ulIdleCycleCount);
		vTaskDelayUntil( &xLastWakeTime, (500*2) / portTICK_RATE_MS  );//精确的以1000ms为周期执行。
	}
	mylog("app exit:err\n");
}



static void app_payload_rx_proc(void  * payload)
{
	static  U8 times_counter = 0;
	
	times_counter++;
	if (times_counter == 3)
	{
		times_counter = 0 ;
		mylog("\n\r w: \n\r");
	}
	//mylog("\n\r w: \n\r");
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
  mylog("R");
  
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


//void package_usartdata_to_csbkdata(U8 *usart_payload, U32 payload_len)
//{
	//
//#if 1
//
	//if(payload_len == 0)//需要拆分为多个datasession指令来发送数据
	//{
		//mylog("payload_len empty!!!\n");
		//return;
	//}
	//
	//U32 q= payload_len/8;
	//U32 r= payload_len%8;
	//U32 counts=0;
	//if(r!=0)
	//{
		//counts =q+2;//第一包为协议信息包，另外如不满8bytes数据需要不全
	//}
	//else
	//{
		//counts =q+1;//整8个数据包
	//}
	//if(counts>MAX_CSBK_UNIT)//需要拆分为多个datasession指令来发送数据
	//{
		//mylog("usart_payload beyond MAX_CSBK_UNIT\n");
		//return;
	//}
	//
	//CSBK_Pro_t * csbk_t_array_ptr=(CSBK_Pro_t*)pvPortMalloc(counts*sizeof(CSBK_Pro_t));//动态分配堆空间数据
	//if(csbk_t_array_ptr==NULL)
	//{
		//mylog("pvPortMalloc failure!!!\n");
	//}
	//else	
		//memset(csbk_t_array_ptr, 0x00, counts*sizeof(CSBK_Pro_t));
		//
	//U32 remaining_len =payload_len;
	//U32 idx =0;
	//U32 data_ptr_index=0;
//
	////打包CSBK数据
	////第一包数据放置协议信息
	//csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
	//csbk_t_array_ptr[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
	//csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	//csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
	////考虑放入校验等数据，未填充字段默认设置为0；
	//csbk_t_array_ptr[idx].csbk_data[0] = payload_len & 0xff;//数据长度低字节
	//csbk_t_array_ptr[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////数据长度高字节，且默认数据长度双字节最大65535
	//
	//
	//do//将负载数据打包到CSBK数据的中间包数据和最后一包数据
	//{
			//idx++;
			//csbk_t_array_ptr[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
			//csbk_t_array_ptr[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
			//csbk_t_array_ptr[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	//
		//if(remaining_len < CSBK_Payload_Length)//不超过8个字节
		//{
			//csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			//memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//拷贝CSBK数据
			//remaining_len =0;//清零剩余数据长度，并退出循环
		//}
		//else//整包
		//{
			//memcpy(csbk_t_array_ptr[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//拷贝CSBK数据
			//data_ptr_index+=CSBK_Payload_Length;//地址指针偏移
			//remaining_len-=CSBK_Payload_Length;//剩余数据长度
			//if(remaining_len == 0)
			//{
				//csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			//}
			//else
			//{
				//csbk_t_array_ptr[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			//}
		//}
	//
	//} while (remaining_len!=0);
	//
	//mylog("send csbk_ptr data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	//
	//xcmp_data_session_csbk_raw_req(csbk_t_array_ptr, sizeof(CSBK_Pro_t)*(idx+1));//最多一次只能发送22个csbk数据包
//
	//vPortFree(csbk_t_array_ptr);
	//csbk_t_array_ptr=NULL;
//
//#else	
//
//
	//CSBK_Pro_t csbk_t_array[100];//一个xcmp指令最大可以拥有100个csbk—pakage
	//memset(csbk_t_array, 0x00, sizeof(csbk_t_array));
	//U32 remaining_len =payload_len;
	//U32 idx =0;
	//U32 data_ptr_index=0;
//
	////打包CSBK数据
	////第一包数据放置协议信息
	//csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
	//csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
	//csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
	//csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
	////考虑放入校验等数据，未填充字段默认设置为0；
	//csbk_t_array[idx].csbk_data[0] = payload_len & 0xff;//数据长度低字节
	//csbk_t_array[idx].csbk_data[1] = ((payload_len >> 8) &0x00ff);////数据长度高字节，且默认数据长度双字节最大65535
	//
//
	//do//将负载数据打包到CSBK数据的中间包数据和最后一包数据 
	//{
		//idx++;	
		//csbk_t_array[idx].csbk_header.csbk_PF = CSBK_PF_FALSE;//fixed value
		//csbk_t_array[idx].csbk_header.csbk_opcode =CSBK_Opcade;//fixed value
		//csbk_t_array[idx].csbk_manufacturing_id = CSBK_Third_PARTY;//fixed value
		//
		//if(remaining_len < CSBK_Payload_Length)//不超过8个字节
		//{				
			//csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			//memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), remaining_len);//拷贝CSBK数据
			//remaining_len =0;//清零剩余数据长度，并退出循环
		//}
		//else//整包
		//{
			//memcpy(csbk_t_array[idx].csbk_data, (usart_payload+data_ptr_index), CSBK_Payload_Length);//拷贝CSBK数据
			//data_ptr_index+=CSBK_Payload_Length;//地址指针偏移
			//remaining_len-=CSBK_Payload_Length;//剩余数据长度
			//if(remaining_len == 0)
			//{
				//csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_TRUE;//负载数据的最后一包
			//}
			//else
			//{
				//csbk_t_array[idx].csbk_header.csbk_LB = CSBK_LB_FALSE;
			//}
		//}
		//
	//} while (remaining_len!=0);
//
	 //mylog("send csbk_t data len:%d\n", sizeof(CSBK_Pro_t)*(idx+1));
	 //
	 //xcmp_data_session_csbk_raw_req(csbk_t_array, sizeof(CSBK_Pro_t)*(idx+1));
	//
//#endif	
//}
