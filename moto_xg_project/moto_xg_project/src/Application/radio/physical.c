/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: physical.c
Author: wxj
Version: 1.0.0.02
Date: 2016/3/25 12:02:25

Description:
History:
*/
#include <string.h>
/*include files are used to FreeRtos*/
//#include "AMBE.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "ambe.h"
#include "ssc.h"
#include "physical.h"


#include "../log/log.h"

volatile phy_fragment_t xnl_store[MAX_XNL_STORE];
volatile xQueueHandle xnl_store_idle = NULL;

/*define the queue are used to send/receive xnl packet*/
volatile xQueueHandle phy_xnl_frame_tx = NULL;
volatile xQueueHandle phy_xnl_frame_rx = NULL;

/*Defines the callback function is used to rend/receive SSC data*/
static void phy_tx_func(void * ssc);
static void phy_rx_func(void * ssc);

/*build and send xnl frame */
static void phy_xnl_tx(xnl_channel_t * xnl_tx_channel);

/*receive and parsing xnl frame*/
static void phy_xnl_rx(xnl_channel_t * xnl_rx_channel);

/*if enable send/receive payload(media), defined in physical.h*/
#if ENABLE == PAYLOAD_ENABLE
volatile U16 payload_store[MAX_PAYLOAD_STORE][MAX_PAYLOAD_BUFF_SIZE];
volatile xQueueHandle payload_store_idle = NULL;

/*define the queue are used to send/receive payload(media) packet*/
volatile xQueueHandle phy_payload_frame_tx = NULL;
volatile xQueueHandle phy_payload_frame_rx = NULL;

/*build and send payload(media) frame */
static void phy_payload_tx(payload_channel_t * payload_tx_channel);

/*receive and parsing payload(media) frame*/
static void phy_payload_rx(payload_channel_t * payload_rx_channel);
#endif /*end if*/


volatile static RxAMBEBurstType m_RxBurstType = VOICE_WATING;
volatile static U32 AMBE_HT[2];

//static U8 AMBE_Per_Burst_Flag = 0;

volatile xQueueHandle test_tx = NULL;
extern U32 tc_tick;
extern void xnl_send_device_master_query(void);
/**
Function: phy_init
Description: initialize the SSC;
    register the func to send/receive ssc packet;
	initialize the queue
Calls:   
    ssc_init -- ssc.c
    register_rx_tx_func -- ssc.c
	xQueueCreate -- freertos
Called By: xnl_init -- xnl.c
*/
void phy_init( void )
{
    /*register the func to send/receive ssc packet*/
    register_rx_tx_func(phy_rx_func, phy_tx_func);	
	
	/*initialize the SSC*/
	ssc_init();
	
	/*send device_master_query to connect radio*/
	//xnl_send_device_master_query();
	
}

/**
Function: phy_tx
Parameters: phy_fragment_t *
Description: separate xnl and payload(media), then push packet to the queue
Calls:   
	xQueueSend -- freertos
Called By: xnl_tx -- xnl.c
*/
void phy_tx(phy_fragment_t * phy)
{	
    Bool res = FALSE;
	
	U16 phy_ctrl = phy->xnl_fragment.phy_header.phy_control;
	
	//phy_fragment_t * phy_ptr = malloc(sizeof(phy_fragment_t));
	//memcpy(phy_ptr, phy, sizeof(phy_fragment_t));
	//mylog("\n\r T_xcmp:%4x \n\r", phy->xnl_fragment.xnl_payload.xnl_content_data_msg.xcmp_opcode);
	//mylog("T_xnl-opcode:%4x", xnl->xnl_header.opcode);//log:R_xnl指令	
	if(XCMPXNL_DATA == (phy_ctrl & 0xF000))
	{
		/*push the xnl packet to queue to send */
		if(NULL != phy_xnl_frame_tx)
		{
			if( pdTRUE == xQueueSend(phy_xnl_frame_tx, &phy, 0))
			{
				res = TRUE;
			}
		}
	}
	
	/*if enable send/receive payload(media), defined in physical.h*/
	#if ENABLE == PAYLOAD_ENABLE		
	else if((SPEAKER_DATA == (phy_ctrl & 0xF000))
	    || (MIC_DATA  == (phy_ctrl & 0xF000))
	    || (PAYLOAD_DATA_RX == (phy_ctrl & 0xF000))
	    || (PAYLOAD_DATA_TX == (phy_ctrl & 0xF000))
	)
	{		
		if(NULL == phy_payload_frame_tx)
		{
		}
	}	
	#endif /*end if*/
	
	if(res != TRUE)
	{
		//vPortFree(phy_ptr);
	}
}

/**
Function: phy_rx
Parameters: phy_fragment_t *
Description: xnl and payload(media), respectively, will send to the corre-
    sponding queue
Calls:   
	xQueueSendFromISR -- freertos
Called By: phy_xnl_rx
    phy_payload_rx
*/
static void phy_rx(phy_fragment_t * phy_ptr)
{
    /*variables are used to store the push result in interrupt*/
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	//phy_fragment_t * xx = pvPortMalloc(sizeof(phy_fragment_t));
	//memcpy(xx, phy_ptr, sizeof(phy_fragment_t));
	//set_phy_idle_isr(&phy_ptr, &xHigherPriorityTaskWoken);
	
	Bool res = FALSE;
	
	if(NULL == phy_ptr)
	{
		return;
	}
	
    U16 phy_ctrl = phy_ptr->xnl_fragment.phy_header.phy_control;

    if (XCMPXNL_DATA == (phy_ctrl & 0xF000))
    {
        /*push the xnl packet to queue*/
        if( pdTRUE == xQueueSendFromISR(
              phy_xnl_frame_rx
            , &phy_ptr
            , &xHigherPriorityTaskWoken
        ))	
		{
			///* 被队列读写操作解除阻塞的任务，其优先级是否高于当前任务？如果是，则进行任务上下文切换 */
			//if (xHigherPriorityTaskWoken == pdTRUE)
			//{
				//taskYIELD();
			//}
			res = TRUE;
		}	

    }
		
 
}

/**
Function: phy_tx_func
Parameters: void *
Description: send ssc data
Calls:   
    phy_xnl_tx
    phy_payload_tx
Called By: phy_xnl_rx
    phy_init(register_rx_tx_func)
*/
static void phy_tx_func( void * ssc)
{
    if(NULL != phy_xnl_frame_tx)
    {
  	  	/*send ssc data in xnl frame*/
  	  	phy_xnl_tx(&(((ssc_fragment_t * )ssc)->xnl_channel));		
    }
	
    /*if enable send/receive payload(media)and enable playback function, defined in physical.h*/
    //#if (ENABLE == PAYLOAD_ENABLE) 
    ////if(NULL != phy_payload_frame_tx)
    //{
		////启用回放功能
		////if (ENABLE == PLAYBACK_ENABLE)
		//{
			///*send ssc data in payload(media) frame*/
			//phy_payload_tx(&(((ssc_fragment_t * )ssc)->payload_channel));	
			//
		//}
    //}
	//#else
	///*send idle frame*/	
	//((ssc_fragment_t * )ssc)->payload_channel.dword[0] = PAYLOADIDLE0;
	//((ssc_fragment_t * )ssc)->payload_channel.dword[1] = PAYLOADIDLE1;
	//#endif /*end if*/
	((ssc_fragment_t * )ssc)->payload_channel.dword[0] = PAYLOADIDLE0;
	((ssc_fragment_t * )ssc)->payload_channel.dword[1] = PAYLOADIDLE1;
}


static void payload_rx(void * payload);
/**
Function: phy_rx_func
Parameters: void *
Description: receive ssc data
Calls:   
    phy_xnl_rx
    phy_payload_tx
Called By: phy_xnl_rx
    phy_init(register_rx_tx_func)
*/
static void phy_rx_func( void * ssc)
{    
		
	if(NULL != phy_xnl_frame_rx)
	{
		/*receive ssc data in xnl frame*/
		phy_xnl_rx(&(((ssc_fragment_t * )ssc)->xnl_channel));
	}	
	
	/*if enable send/receive payload(media), defined in physical.h*/
	#if ENABLE == PAYLOAD_ENABLE
	//if(NULL != phy_payload_frame_rx)
	{
		/*receive ssc data in payload frame*/
		phy_payload_rx(&(((ssc_fragment_t * )ssc)->payload_channel));
	}
	#endif /*end if*/
	
	
}

/**
Function: phy_xnl_tx
Parameters: xnl_channel_t * 
Description: send xnl packet
Calls:   
    xQueueReceiveFromISR -- freertos
Called By:phy_tx_func
*/
static void phy_xnl_tx(xnl_channel_t * xnl_tx_channel)
{
	/*variables are used to store the push result in interrupt*/
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	/*To store the elements in the queue*/
	//static phy_fragment_t phy_frame;
	
	static phy_fragment_t  * phy_ptr;
	
	/*Analytical status*/
	static phy_tx_state_t phy_tx_state;
	
	static S16 phy_tx_expexted_length = 0;
	static U8 phy_tx_index = 0;
	
	/*his is the code for handling any outgoing XNL Phy message*/
	switch(phy_tx_state)
	{
		/*Waiting for the send xnl packet*/
		case WAITING_FOR_PHY_TX:			
			if( pdTRUE == xQueueReceiveFromISR(
				  phy_xnl_frame_tx
				, &phy_ptr
				, &xHigherPriorityTaskWoken 
			))
			{								
				phy_tx_expexted_length = 
				     phy_ptr->xnl_fragment.phy_header.phy_control & 0x000000FF;
				
				/*
				Handle to first fragment. Assume index to a valid fragment block.
                Points to first hWord in fragment block.
				*/
				xnl_tx_channel->dword = 
				                   phy_ptr->xnl_fragment.phy_header.phy_control;
				
				/*Transmit 0xABCD0000 | Type/Length.*/
				xnl_tx_channel->dword |= PHYHEADER32;
				
				phy_tx_index = 1;
				phy_tx_state = WRITE_NEXT_DWORD;	
				
				/*The new transmission has started.*/
			}
			else
			{
				/*Nothing new to transmit, send an idle frame*/
				xnl_tx_channel->dword = XNL_IDLE;
			}			
			break;
		
		case WRITE_NEXT_DWORD:
			xnl_tx_channel->dword = 
							    phy_ptr->fragment_element[phy_tx_index++] << 16;

			phy_tx_expexted_length -= 2;
			
			/*have written all the bytes (including 16-bit pad)*/
			if (phy_tx_expexted_length <= 0)
			{
				/*Must immediately send 0x00BA in Slot 4.*/
				xnl_tx_channel->dword |= PHYTERMRIGHT;
				
				/*Go back to waiting.*/
				phy_tx_state = WAITING_FOR_PHY_TX;
				break;
			}

			/*Have not broken. Transmit 2nd hWord.*/
			xnl_tx_channel->dword |=  phy_ptr->fragment_element[phy_tx_index++];

			phy_tx_expexted_length -= 2;
			
			/*Have written all the bytes (including 16-bit pad)*/
			if (phy_tx_expexted_length <= 0)
			{
				/*Must send 0x00BA0000 next interrupt in Slot 3&4*/
				phy_tx_state = SEND_TAILED;
			}
			break;

		case SEND_TAILED:
			/*send 0x00BA0000*/
			xnl_tx_channel->dword = PHYTERMLEFT;
			
			/*Go back to waiting.*/		
			phy_tx_state = WAITING_FOR_PHY_TX;		
			break;
			/*This fragment finished.*/
			
		default:
			break;
	}
}

/**
Function: phy_xnl_rx
Parameters: xnl_channel_t * 
Description: send xnl packet
Calls:   
    phy_rx
Called By: phy_rx_func
*/
static void phy_xnl_rx(xnl_channel_t * xnl_rx_channel)
{
//	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	static phy_fragment_t * phy_frame_ptr = NULL;
	
	static S16 phy_rx_expexted_length = 0;
	static S16 phy_rx_length = 0;
	static U16 phy_check_sum = 0;

	static phy_rx_state_t  phy_rx_state = WAITING_FOR_HEADER;
	static U32 phy_rx_count = 0;

	U32 phy_dword = xnl_rx_channel->dword;	

	phy_rx_count++;
	
	/*This is the code for parsing the incoming physical message.*/
	switch (phy_rx_state)
	{
		/*
		Note that all segments must align with a 32-bit boundary and beginning
		of each XCMP/XNL payload frame must start on slot 3 Thus, segments of
		odd length must append a 0x0000 at the end (slot 4) to ensure 
		alignment. [9.1.3]
		*/	

        /*Waiting for something. Most frequent visit.*/		
		case WAITING_FOR_HEADER:
		
			/*Ignore Idles.*/
			if (0xABCD5A5A == phy_dword)
			{
				break;
			}	

			/*Skip until Header.*/		
			if (0xABCD != (phy_dword >> 16))
			{			
				break;
			}
		
			/*Length excluding CSUM.*/
			phy_rx_expexted_length = (phy_dword & 0x000000FF) - 2;
			
			/*Discard degenerate message.*/
			if (phy_rx_expexted_length <= 0)
			{
				break;
			}
		
			phy_rx_length = 0;

			phy_frame_ptr = get_xnl_idle_isr();
			if(NULL == phy_frame_ptr)
			{
				break;
			}
					
			phy_frame_ptr->fragment_element[phy_rx_length++] = phy_dword;
			
			/*time stamp*/
			phy_frame_ptr->fragment_element[phy_rx_length++] = 
													(phy_rx_count) & 0x0000FFFF;
			
			/*This switch tests the fragment type, and adjusts receiver state
			accordingly.*/
			/*Check frag type*/
			switch (phy_dword & 0x00000F00)
			{
				case SINGLE_FRAGMENT:  //Only Fragment.
				case FIRST_FRAGMENT:   //First of Multi-fragment.
				case MIDDLE_FRAGMENT:  //Continuing Multi-fragment.
				case LAST_FRAGMENT:    //Last Multi-fragment.
					phy_rx_state = WAITING_CHECK_SUM;
				break;
				default:
					set_xnl_idle_isr(phy_frame_ptr);			
				break;
			}	
			break;

		/*
		Gets here on CSUM. Expect at least one hWord payload. Gets here once 
		on every fragment.*/	
		case WAITING_CHECK_SUM:
		
			/*Stores CSUM*/
			phy_check_sum  = (phy_dword & 0xFFFF0000) >> 16;
			
			/*sums in first hWord*/
			phy_check_sum += (phy_dword & 0x0000FFFF);		
			phy_frame_ptr->fragment_element[phy_rx_length++] =  
														 phy_dword & 0x0000FFFF;
					
			phy_rx_expexted_length -= 2;
			
			/*Normal case for greater than one byte payloads.*/
			if (phy_rx_expexted_length > 0)
			{					  
				phy_rx_state = READING_FRAGMENT;
			}
			
			/*
			Sort of strange, one byte payload. Should not happen, but follow
			protocol.*/
			else
			{
				/*
				Expect next word 0x00BA0000.
	    		Note that all segments must align with a 32-bit boundary and
				beginning of each XCMP/XNL payload frame must start on slot 3
				Thus, segments of odd length must append a 0x0000 at the end
				(slot 4) to ensure alignment. [9.1.3]
				*/
				phy_rx_state = WAITING_LAST_TERM;
			}			
		
			break;

		case READING_FRAGMENT:
			phy_check_sum += (phy_dword & 0xFFFF0000) >> 16;
	
			phy_frame_ptr->fragment_element[phy_rx_length++] = 
												 (phy_dword & 0xFFFF0000) >> 16;
	
			phy_rx_expexted_length -= 2;
			if (phy_rx_expexted_length <= 0)
			{
				/*
				All read in.
				Terminator should be in 2nd hWord.
				Shaoqun says useful bits not used. The packet will always end
				with $00BA. [9.1.2.8]	
				*/
				
				if ((0x000000BA == (phy_dword  & 0x0000FFFF)) 
					&& (phy_check_sum == 0))
				{
					phy_rx(phy_frame_ptr);
				}
				else
				{
					set_xnl_idle_isr(phy_frame_ptr);
				}

				phy_rx_state = WAITING_FOR_HEADER;
				break;
			}
		
			/*Have not broken. 2nd hWord contains payload.*/
			phy_check_sum += (phy_dword & 0x0000FFFF);
		
			phy_frame_ptr->fragment_element[phy_rx_length++] =  
													   (phy_dword & 0x0000FFFF);
		
			phy_rx_expexted_length -= 2;
			if (phy_rx_expexted_length <= 0)
			{
				/*All read in. Next Word should be 0x00BA0000.*/
				phy_rx_state = WAITING_LAST_TERM;
			}/*else, next Word contains more payload.*/
			break;

		/*Expecting last terminator 0x00BA0000.*/	
		case WAITING_LAST_TERM:			
			if ((0x00BA0000 == (phy_dword  & 0x00FF0000)) /*Expected found.*/
				&& (phy_check_sum == 0))/*Good checksum*/
			{
				phy_rx(phy_frame_ptr);

			}
			else
			{
				set_xnl_idle_isr(phy_frame_ptr);
			}
				
			phy_rx_state = WAITING_FOR_HEADER;
			break;
		default:
		break;
	}/*End of phy_rx_state switch.*/
}

//extern char AMBE_AudioData[];
extern char AudioData[];
extern U16 Public_AMBEkey[];
extern U8 Speaker_is_unmute;
extern U8 Silent_flag;
extern U8 Terminator_Flag;
extern U8 AMBE_rx_flag;
extern U8 AMBE_tx_flag;
extern U8 AMBE_Media;
extern U8 VF_SN ;
extern U8 Burst_ID;

extern U8 Radio_Transmit_State;// in standby or receive mode
extern U8 Mic_is_Enabled;

/*if enable send/receive payload(media), defined in physical.h*/
#if ENABLE == PAYLOAD_ENABLE
/**
Function: phy_payload_tx
Parameters: payload_channel_t * 
Description: send payload(media) packet
Calls: 
Called By:phy_tx_func
*/

static void phy_payload_tx(payload_channel_t * payload_tx_channel)
{
//	static U32 counter = 0;
	// 0:idle;1:header; 2:send, 3:end frame
	static AMBEPayloadTxStates AMBEpayload_tx_state = AMBE_IDLE;
	
//	static U8 payload_tx_state = 0;
//	static U8 frame_number = 0; // 10frame;
//	static S16 expexted_length = 0;
//	static Bool last_frame = FALSE;
	
//	static U32 index = 0;
	static U32 A_index = 0;
	static U32 Soft_index = 0;
	
//	static U32 send_num = 0;
	
//	static U32 i = 0;
	
	//static U8 frame_5_end = 0;
	//static U16 pay[256];
	
	//Send-AMBE-data
	if ((AMBE_Media == TRUE))
	{
		A_index = (A_index >=1456) ? 0 : A_index;
	  //AMBE_flag
		switch(AMBEpayload_tx_state)
		{
			case AMBE_IDLE:
		
				if ((m_RxBurstType == VOICE_WATING) || (m_RxBurstType == VOICETERMINATOR)  || (m_RxBurstType == VOICEHEADER))
				{
							
					payload_tx_channel->dword[0] = PAYLOADIDLE0;
					payload_tx_channel->dword[1] = PAYLOADIDLE1;
			
				}
				else if((m_RxBurstType == UNSUREDATA))
				{
					payload_tx_channel->dword[0] = AMBE_HT[0];
					payload_tx_channel->dword[1] = AMBE_HT[1];
				}
				else//加密和解密的激励类型
				{
					
					//Radio Internal Parameter + Vocoder Bits Stream Parameter + soft decision value + 1st 20ms AMBE bits of Voice Burst A 
					//assemble 4 items into 1 Command. Send it to Radio.
					if (m_RxBurstType == RADIOINTERNAL)//解密数据
					{
						//0xABCDCO32
						payload_tx_channel->dword[0] = DE_OB_PAYLOAD;//50bytes
						//0x847F
						payload_tx_channel->word[2] = RIP_PAYLOAD;
						//0xxxxx
						payload_tx_channel->word[3] = Radio_Internal_Data[0];
						
						AMBEpayload_tx_state = AMBE_DE_FIRST;
					}
					//Vocoder Bits Stream Parameter + 1st 20ms AMBE bits of Voice Burst A 
					//assemble 2 items into 1 Command. Send it to Radio.
					else//加密数据
					{
	
						//0xABCDCOOE
						payload_tx_channel->dword[0] = EN_OB_PAYLOAD;//49bits
						//0x8212
						payload_tx_channel->word[2] = VBSP_data[0];
						//0xF00x
						payload_tx_channel->word[3] = VBSP_data[1];
						
						AMBEpayload_tx_state = AMBE_EN_FIRST;
					}

				}
				
				break;
			
			case AMBE_EN_FIRST:
			
			
				//0x88F2
				payload_tx_channel->word[0] = ENCODER_PAYLOAD;//49bits
				
				//payload_tx_channel->word[1] = ((AMBE_AudioData[A_index]<<8) + AMBE_AudioData[A_index+1]) ;
				//A_index+=2;
				//
				//payload_tx_channel->word[2] = ((AMBE_AudioData[A_index]<<8) + AMBE_AudioData[A_index+1]) ;
				//A_index+=2;
				//
				//payload_tx_channel->word[3] = ((AMBE_AudioData[A_index]<<8) + AMBE_AudioData[A_index+1]) ;
				//A_index+=2;
				//
				//AMBEpayload_tx_state = AMBE_EN_LAST;
				
			
				switch (m_RxBurstType)//在发送函数中去做加密处理
				{
					case VOICEBURST_A:
							if (VF_SN == 1)
							{	
								
								
								//Place public key
								payload_tx_channel->word[1] = Public_AMBEkey[0];
								payload_tx_channel->word[2] = Public_AMBEkey[1];
								payload_tx_channel->word[3] = Public_AMBEkey[2];
							
								//payload_tx_channel->word[1] = AMBEBurst_rawdata[0];
								//payload_tx_channel->word[2] = AMBEBurst_rawdata[1];
								//payload_tx_channel->word[3] = AMBEBurst_rawdata[2];				
								//logFromISR("\n\r MMQ \n\r");
						
							}
							else//VF_SN==2/3
							{
								
								
						
								//Encrypted AMBE data(XOR)
								payload_tx_channel->word[1] = ((Public_AMBEkey[0]) ^ (AMBEBurst_rawdata[0])) ;
								payload_tx_channel->word[2] = ((Public_AMBEkey[1]) ^ (AMBEBurst_rawdata[1])) ;
								payload_tx_channel->word[3] = ((Public_AMBEkey[2]) ^ (AMBEBurst_rawdata[2])) ;
							
								//payload_tx_channel->word[1] = AMBEBurst_rawdata[0];
								//payload_tx_channel->word[2] = AMBEBurst_rawdata[1];
								//payload_tx_channel->word[3] = AMBEBurst_rawdata[2];
							
						
							}
					
							AMBEpayload_tx_state = AMBE_EN_LAST;
					
						break;
					
					case VOICEBURST_B:
					case VOICEBURST_C:	
					case VOICEBURST_D:	
					case VOICEBURST_E:
					case VOICEBURST_F:
						
							//Encrypted AMBE data(XOR)
						
							//payload_tx_channel->word[1] = AMBEBurst_rawdata[0];
							//payload_tx_channel->word[2] = AMBEBurst_rawdata[1];
							//payload_tx_channel->word[3] = AMBEBurst_rawdata[2];
							payload_tx_channel->word[1] = ((Public_AMBEkey[0]) ^ (AMBEBurst_rawdata[0])) ;
							payload_tx_channel->word[2] = ((Public_AMBEkey[1]) ^ (AMBEBurst_rawdata[1])) ;
							payload_tx_channel->word[3] = ((Public_AMBEkey[2]) ^ (AMBEBurst_rawdata[2])) ;
						
							AMBEpayload_tx_state = AMBE_EN_LAST;
				
						break;
					default://This shouldn't happen, but must check;
					
							payload_tx_channel->dword[0] = PAYLOADIDLE0;
							payload_tx_channel->dword[1] = PAYLOADIDLE1;
							
							AMBEpayload_tx_state = AMBE_IDLE;
					
						break;
				}
		
		
		
		
				break;
			
			case AMBE_EN_LAST:
					
				
					//Encrypted AMBE data(XOR)
					//NOTE:
					/*
						There are 2 places we recommend to steal and insert the 3rd party inter-application data
						  1:VF1 of first Burst A after last Voice Header (49 bits).
						  2:Last 4 bits (bit 45 to bit 48) of every 49 bits AMBE Voice Frame. (72 bits@Super Frame).
					
					*/
					
					payload_tx_channel->word[0] = AMBEBurst_rawdata[3];
				
					//if ((m_RxBurstType == VOICEBURST_A) && (VF_SN == 1))
					//{
						//payload_tx_channel->word[0]	= (Public_AMBEkey[3]) ;
					//}
					//else{
						//
						//payload_tx_channel->word[0]	= ((Public_AMBEkey[3]) ^ (AMBEBurst_rawdata[3])) ;
						//
					//}
					
					
					//payload_tx_channel->word[0] = ((AMBE_AudioData[A_index]<<8) + 0x00) ;//需要补充Pad_bits位
					//A_index+=1;
					
					payload_tx_channel->word[1]	= 0x00BA ; 
					payload_tx_channel->word[2]	= 0x0000 ;
					payload_tx_channel->word[3]	= 0x0000 ;
				
					AMBEpayload_tx_state = AMBE_IDLE;
				
				break;
				
			case AMBE_DE_FIRST:
				
					//0xxxxx
					payload_tx_channel->word[0] = Radio_Internal_Data[1];
					//0x8212
					payload_tx_channel->word[1] = VBSP_data[0];
					
					if ((m_RxBurstType == VOICEBURST_A) && (VF_SN == 1) )
					{
						// Vocoder Bits Stream Parameter("E(bit8)" flag = 1)
						//0xF0(8+)x
						payload_tx_channel->word[2] = ((VBSP_data[1]) | (0x0100));
					}
					else
						//0xF00x
						payload_tx_channel->word[2] = (VBSP_data[1]) ;
					
					//0x9A13
					payload_tx_channel->word[3] = SDV_PAYLOAD;
		
					AMBEpayload_tx_state = AMBE_DE_SECOND;
					
				break;
			
			case AMBE_DE_SECOND:
					//Post back soft decision value data

						payload_tx_channel->word[0] = Soft_Decision_Data[Soft_index] ;
						payload_tx_channel->word[1] = Soft_Decision_Data[Soft_index+1] ;
						payload_tx_channel->word[2] = Soft_Decision_Data[Soft_index+2] ;
						payload_tx_channel->word[3] = Soft_Decision_Data[Soft_index+3] ;
						Soft_index +=4;
						
						if (Soft_index == 12)
						{
							AMBEpayload_tx_state = AMBE_DE_THIRD;
							//Soft_index = 0 ;
							
						}
						else
						AMBEpayload_tx_state = AMBE_DE_SECOND;
			
				break;	
				
			case AMBE_DE_THIRD:
					
					//0x xxxx
					payload_tx_channel->word[0] = Soft_Decision_Data[Soft_index] ;
					Soft_index = 0 ;//索引清零
					
					//0x8003 隔离符号？
					payload_tx_channel->word[1] = 0x8003;
					//0x88F3
					payload_tx_channel->word[2] = DECODER_PAYLOAD;
				
					switch (m_RxBurstType)//在发送函数中去做解密处理
					{
						case VOICEBURST_A:
								if (VF_SN == 1)
								{	
									//if(AMBEBurst_rawdata[0] == Public_AMBEkey[0])//0X1AC3
									{
										
									
										//Pick up public key
										AMBE_DecryptionKey[0] = AMBEBurst_rawdata[0];
									
										//Post back data
										payload_tx_channel->word[3] = AMBEBurst_rawdata[0] ;
									}
									//else//干扰数据
									//{
										////logFromISR("\n\r MMQ：%x \n\r", AMBEBurst_rawdata[0]);
										////Pick up public key
										//AMBE_DecryptionKey[0] = Public_AMBEkey[0];
										//
										////Post back data
										//payload_tx_channel->word[3] = Public_AMBEkey[0] ;
										//
									//}
						
								}
								else//VF_SN==2/3
								{	
									//Decrypt AMBE data(XOR) 
									//Recover data

									payload_tx_channel->word[3] = ((AMBE_DecryptionKey[0]) ^ (AMBEBurst_rawdata[0])) ;
									
									//payload_tx_channel->word[3] = ((AMBEBurst_rawdata[0])) ;
						
								}
					
								AMBEpayload_tx_state = AMBE_DE_LAST;
					
							break;
					
						case VOICEBURST_B:
						case VOICEBURST_C:	
						case VOICEBURST_D:	
						case VOICEBURST_E:
						case VOICEBURST_F:
						
								//Encrypted AMBE data(XOR)
						
								//Decrypt AMBE data(XOR)
								//Recover data
								payload_tx_channel->word[3] = ((AMBE_DecryptionKey[0]) ^ (AMBEBurst_rawdata[0])) ;
								//payload_tx_channel->word[3] = ((AMBEBurst_rawdata[0])) ;
						
								AMBEpayload_tx_state = AMBE_DE_LAST;
				
							break;
						default://This shouldn't happen, but must check;
					
								payload_tx_channel->dword[0] = PAYLOADIDLE0;
								payload_tx_channel->dword[1] = PAYLOADIDLE1;
								AMBEpayload_tx_state = AMBE_IDLE;
					
							break;
					}
		
					break;
				
			case AMBE_DE_LAST:
				
					switch (m_RxBurstType)//在发送函数中去做解密处理
					{
						case VOICEBURST_A:
							if (VF_SN == 1)
							{
								
								//if(AMBEBurst_rawdata[1] == Public_AMBEkey[1])//0X1840
								{
								
									//Pick up public key
							
									AMBE_DecryptionKey[1] = AMBEBurst_rawdata[1];
									AMBE_DecryptionKey[2] = AMBEBurst_rawdata[2];
									AMBE_DecryptionKey[3] = AMBEBurst_rawdata[3];
							
									//Post back data
									payload_tx_channel->word[0] = AMBEBurst_rawdata[1] ;
									payload_tx_channel->word[1] = AMBEBurst_rawdata[2] ;
									payload_tx_channel->word[2] = AMBEBurst_rawdata[3] ;
									payload_tx_channel->word[3] = 0x00BA ;
								}
								//else
								//{
									////Pick up public key
									//
									//AMBE_DecryptionKey[1] = Public_AMBEkey[1];
									//AMBE_DecryptionKey[2] = Public_AMBEkey[2];
									//AMBE_DecryptionKey[3] = Public_AMBEkey[3];
									//
									////Post back data
									//payload_tx_channel->word[0] = Public_AMBEkey[1] ;
									//payload_tx_channel->word[1] = Public_AMBEkey[2] ;
									//payload_tx_channel->word[2] = Public_AMBEkey[3] ;
									//payload_tx_channel->word[3] = 0x00BA ;
									//
									//
									//
									//
								//}
								//logFromISR("\n\r MMQ \n\r");
							
							}
							else//VF_SN==2/3
							{
							
								//Decrypt AMBE data(XOR)
								//Recover data

									payload_tx_channel->word[0] = (AMBEBurst_rawdata[1] ^ AMBE_DecryptionKey[1]);
									payload_tx_channel->word[1] = (AMBEBurst_rawdata[2] ^ AMBE_DecryptionKey[2]);
									payload_tx_channel->word[2] = (AMBEBurst_rawdata[3]);
									//payload_tx_channel->word[2] = (AMBEBurst_rawdata[3] ^ AMBE_DecryptionKey[3]);
									
									//payload_tx_channel->word[0] = (AMBEBurst_rawdata[1]);
									//payload_tx_channel->word[1] = (AMBEBurst_rawdata[2]);
									//payload_tx_channel->word[2] = (AMBEBurst_rawdata[3]);
									
									payload_tx_channel->word[3] = 0x00BA ;
							
							
							}
						
							AMBEpayload_tx_state = AMBE_IDLE;
						
						break;
						
						case VOICEBURST_B:
						case VOICEBURST_C:
						case VOICEBURST_D:
						case VOICEBURST_E:
						case VOICEBURST_F:
						
							//Encrypted AMBE data(XOR)
						
							//Decrypt AMBE data(XOR)
							//Recover data
							payload_tx_channel->word[0] = (AMBEBurst_rawdata[1] ^ AMBE_DecryptionKey[1]);
							payload_tx_channel->word[1] = (AMBEBurst_rawdata[2] ^ AMBE_DecryptionKey[2]);
							payload_tx_channel->word[2] = (AMBEBurst_rawdata[3]);
							//payload_tx_channel->word[2] = (AMBEBurst_rawdata[3] ^ AMBE_DecryptionKey[3]);
							
							//payload_tx_channel->word[0] = (AMBEBurst_rawdata[1]);
							//payload_tx_channel->word[1] = (AMBEBurst_rawdata[2]);
							//payload_tx_channel->word[2] = (AMBEBurst_rawdata[3]);
							
							payload_tx_channel->word[3] = 0x00BA ;
						
							AMBEpayload_tx_state = AMBE_IDLE;
						
							break;
						default://This shouldn't happen, but must check;
						
							payload_tx_channel->dword[0] = PAYLOADIDLE0;
							payload_tx_channel->dword[1] = PAYLOADIDLE1;
							AMBEpayload_tx_state = AMBE_IDLE;
							
							break;
					}

				break;
		
			default:
			
					payload_tx_channel->dword[0] = PAYLOADIDLE0;
					payload_tx_channel->dword[1] = PAYLOADIDLE1;
					
					AMBEpayload_tx_state = AMBE_IDLE;
					break;
			
				
		
				}

	}//end of Send-AMBE-data

	else
	{
		payload_tx_channel->dword[0] = PAYLOADIDLE0;
		payload_tx_channel->dword[1] = PAYLOADIDLE1;
	}
#if 0
else//Send-PCM-data（注意测试回放时：模拟信道码流为40bytes/2.5ms.）
{
	
	index = (index >=30240) ? 0 : index;
	
	if(Speaker_is_unmute == 1)counter++;
	
	switch(payload_tx_state)
	{
		case 0:
		
			payload_tx_channel->dword[0] = PAYLOADIDLE0;
			payload_tx_channel->dword[1] = PAYLOADIDLE1;
		
			if(((counter % 20 )== 0) && (counter != 0) &&(Speaker_is_unmute == 1))//20*125us = 2.5ms.
			{
				payload_tx_state = 1;
				//payload_tx_state = 0;
				frame_number = 0;
	
			}
		
			send_num++;
		
			break;
		
		case 1:
		
	
			payload_tx_channel->word[0] = 0xABCD; 
		
			if(frame_number == 0)
			{
				if (Silent_flag == 1)
				{
					//即是44betes-4bytes= 40bytes
					expexted_length = 0x2C;//One Descriptor Indicator
					payload_tx_channel->word[1] = 0x102C;//44betes-4bytes= 40bytes
				}
				
				else
				{
					//first frame
					expexted_length = 0x2A;
					payload_tx_channel->word[1] = 0x102A;//42-2= 40 bytes;	
				}
				
					last_frame = TRUE;
			}
			
		
			//if //(Terminator_Flag == 1)
			//if((send_num == 1) && (last_frame == FALSE) )
			//if((send_num == 15) && (last_frame == FALSE) )
			//{
			////Speaker_is_unmute = 0;//提示mute,并退出
			//if (Silent_flag == 1)//发送静音指令
			//{
			//
			//payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
			//payload_tx_channel->word[3] =  0x0004;//Silent Descriptor Indicator
			//
			////payload_tx_channel->word[2] = 0x0002;//Array Descriptor Length
			//
			////payload_tx_channel->word[3] =  0x0003;//Stream Terminator Indicator
			//}
			//else
			//{
			//payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
			//
			//payload_tx_channel->word[3] =  0x0003;//Stream Terminator Indicator
			//
			//
			//}
			//
			////send_num = 0;//满320bytes则重新计数
			//
			////index+=2;
			//expexted_length -= 4;
			//
			//
			//}
			//else//不执行
			
			if (Silent_flag == 1)//发送静音指令
			{
			
				payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
				payload_tx_channel->word[3] =  0x0004;//Silent Descriptor Indicator

			}
			else
			{
			
				payload_tx_channel->word[2] = 0x0000;
				payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
				index+=2;
			
			}
		
			expexted_length -= 4;
		
			//frame_number++;
			payload_tx_state = 2;
		
			i = 0;
		
			break;
		
		case 2:
		
			if(expexted_length <= 0)
			{
				//last word 0x00BA
				payload_tx_state = last_frame ? 0 : 1;
				payload_tx_channel->word[0] = 0x00BA;
				payload_tx_channel->word[1] = 0x0000;
				payload_tx_channel->word[2] = 0x0000;
				payload_tx_channel->word[3] = 0x0000;
				break;
			}
		
			if(Silent_flag == 1)
			{
				payload_tx_channel->word[0] =  0x0000;
			}
			else
			{
				//payload_tx_channel->word[0] = AudioData[index++] + (AudioData[index++] << 8);
				payload_tx_channel->word[0] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
				index+=2;
			
			}

			expexted_length -= 2;
			if(expexted_length <= 0)
			{
				//last word 0x00BA
				payload_tx_state = last_frame ? 0 : 1;
				payload_tx_channel->word[1] = 0x00BA;
				payload_tx_channel->word[2] = 0x0000;
				payload_tx_channel->word[3] = 0x0000;
				break;
			}
		
			if(Silent_flag == 1)
			{
				payload_tx_channel->word[1] =  0x0000;
			}
			else
			{
			
				payload_tx_channel->word[1] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
				index+=2;
			}
			//payload_tx_channel->word[1] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
		
			expexted_length -= 2;
			if(expexted_length <= 0)
			{
				//last word 0x00BA
				payload_tx_state = last_frame ? 0 : 1;
				payload_tx_channel->word[2] = 0x00BA;
				payload_tx_channel->word[3] = 0x0000;
				break;
			}
		
			if(Silent_flag == 1)
			{
				payload_tx_channel->word[2] =  0x0000;
			}
			else
			{
			
				payload_tx_channel->word[2] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
				index+=2;
			}
			//payload_tx_channel->word[2] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
		
			expexted_length -= 2;
			if(expexted_length <= 0)
			{
				//last word 0x00BA
				payload_tx_state = last_frame ? 0 : 1;
				payload_tx_channel->word[3] = 0x00BA;
				break;
			}
		
			if(Silent_flag == 1)
			{
				payload_tx_channel->word[3] =  0x0000;
			}
			else
			{
			
				payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
				index+=2;
			}
		
			//payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
		
			expexted_length -= 2;
		
			break;
		
		default:
			payload_tx_state = 0;
			break;
		
	}

}//end of Send-PCM-data

#endif

#if 0
	
	else//Send-PCM-data（注意测试回放时：数字信道码流为320bytes/20ms)
	{
		
		index = (index >=30240) ? 0 : index;
		
		if((Radio_Transmit_State == 1) && (Mic_is_Enabled == 1))counter++;
		
		switch(payload_tx_state)
		{
			case 0:
			
					payload_tx_channel->dword[0] = PAYLOADIDLE0;
					payload_tx_channel->dword[1] = PAYLOADIDLE1;
	
					if((counter % 160== 0) && (counter != 0) &&(Radio_Transmit_State == 1))//160*125us = 20ms; 
					{
						payload_tx_state = 1;
	
						frame_number = 0;
						//logFromISR("\n\r payload_tx_state: %d \n\r", payload_tx_state);
					}
					
					send_num++;
		
				break;
			
			case 1:
				
				//logFromISR("\n\r counter: %d \n\r", counter);
				payload_tx_channel->word[0] = 0xABCD; //254 bytes;
	
				if(frame_number == 0)
				{
					//first frame
					expexted_length = 0xFE;
					payload_tx_channel->word[1] = 0x21FE;//0x11FE; //254 bytes;		mic_data?
					last_frame = FALSE;
				}
				else if(frame_number + 1 >= 2) //2frame
				{
					//last frame
					//if (send_num == 15)
					//{							即是(254+70)324betes-4bytes= 320bytes
						//expexted_length = 0x46;//经测试发现，要获取正确的语音的（按320bytes/20ms纯语音码流），并且每一个样本都添加了一个Stream Terminator Indicator的话，那么总长度-描述符相关的字段=320bytes
							//payload_tx_channel->word[1] = 0x1346;
						//send_num = 0;//满320bytes则重新计数
					//}
					//else{
					if (Silent_flag == 1)
					{
						//即是(254+70)324betes-4bytes= 320bytes
						expexted_length = 0x46;//One Descriptor Indicator
						payload_tx_channel->word[1] = 0x2346;//70bytes
					}
		
					else
					{
						///即是(254+68)322betes-2bytes(no Descriptor Indicator)= 320bytes
						expexted_length = 0x44;//no Descriptor Indicator
						payload_tx_channel->word[1] = 0x2344;//0x1344;//0x2344;// 0x1344; //68bytes
					}
					//}
		
					last_frame = TRUE;
					//frame_5_end = 1;
					//logFromISR("\n\r time: %d \n\r", tc_tick);
				}
				else
				{
					//middle frame
					expexted_length = 0xFE;
					payload_tx_channel->word[1] = 0x22FE;//0x22FE;//0x12FE;
					last_frame = FALSE;
				}
	
				//if //(Terminator_Flag == 1)
				//if((send_num == 1) && (last_frame == FALSE) )
				//if((send_num == 15) && (last_frame == FALSE) )
				//{
				////Speaker_is_unmute = 0;//提示mute,并退出
				//if (Silent_flag == 1)//发送静音指令
				//{
				//
				//payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
				//payload_tx_channel->word[3] =  0x0004;//Silent Descriptor Indicator
				//
				////payload_tx_channel->word[2] = 0x0002;//Array Descriptor Length
				//
				////payload_tx_channel->word[3] =  0x0003;//Stream Terminator Indicator
				//}
				//else
				//{
					//payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
					//
					//payload_tx_channel->word[3] =  0x0003;//Stream Terminator Indicator
					//
					//
				//}
				//
				////send_num = 0;//满320bytes则重新计数
				//
				////index+=2;
				//expexted_length -= 4;
				//
				//
				//}
				//else//不执行
				{
		
					if (Silent_flag == 1)//发送静音指令
					{
					
						payload_tx_channel->word[2] = 0x0001;//Array Descriptor Length
						payload_tx_channel->word[3] =  0x0004;//Silent Descriptor Indicator
					
			
					}
					else
					{
						payload_tx_channel->word[2] = 0x0000;
						payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
						index+=2;
						
					}
					
					expexted_length -= 4;
					
					
			
				}
	
				//index+=2;
				//expexted_length -= 4;
		
				frame_number++;
				payload_tx_state = 2;
	
				i = 0;
	
				break;
			
			case 2:
		
				if(expexted_length <= 0)
				{
					//last word 0x00BA
					payload_tx_state = last_frame ? 0 : 1;
					payload_tx_channel->word[0] = 0x00BA;
					payload_tx_channel->word[1] = 0x0000;
					payload_tx_channel->word[2] = 0x0000;
					payload_tx_channel->word[3] = 0x0000;
					break;
				}
	
				if(Silent_flag == 1)
				{
					payload_tx_channel->word[0] =  0x0000;
				}
				else
				{
					//payload_tx_channel->word[0] = AudioData[index++] + (AudioData[index++] << 8);
					payload_tx_channel->word[0] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
					index+=2;
		
				}

				expexted_length -= 2;
				if(expexted_length <= 0)
				{
					//last word 0x00BA
					payload_tx_state = last_frame ? 0 : 1;
					payload_tx_channel->word[1] = 0x00BA;
					payload_tx_channel->word[2] = 0x0000;
					payload_tx_channel->word[3] = 0x0000;
					break;
				}
	
				if(Silent_flag == 1)
				{
					payload_tx_channel->word[1] =  0x0000;
				}	
				else
				{
		
					payload_tx_channel->word[1] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
					index+=2;
				}
				//payload_tx_channel->word[1] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
	
				expexted_length -= 2;
				if(expexted_length <= 0)
				{
					//last word 0x00BA
					payload_tx_state = last_frame ? 0 : 1;
					payload_tx_channel->word[2] = 0x00BA;
					payload_tx_channel->word[3] = 0x0000;
					break;
				}
	
				if(Silent_flag == 1)
				{
					payload_tx_channel->word[2] =  0x0000;
				}
				else
				{
		
					payload_tx_channel->word[2] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
					index+=2;
				}
				//payload_tx_channel->word[2] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
	
				expexted_length -= 2;
				if(expexted_length <= 0)
				{
					//last word 0x00BA
					payload_tx_state = last_frame ? 0 : 1;
					payload_tx_channel->word[3] = 0x00BA;
					break;
				}
	
				if(Silent_flag == 1)
				{
					payload_tx_channel->word[3] =  0x0000;
				}
				else
				{
		
					payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
					index+=2;
				}
	
				//payload_tx_channel->word[3] =  ((AudioData[index]<<8 )+ AudioData[index+1] );
	
				expexted_length -= 2;
		
				break;
		
			default:
				payload_tx_state = 0;
				break;
	
		}		

	}//end of Send-PCM-data
	
#endif

}

static void payload_tx(void * payload)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	if(NULL == phy_payload_frame_tx)
	{
		phy_payload_frame_tx = xQueueCreate(TX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t *));
	}

	if(errQUEUE_FULL == xQueueSendFromISR(phy_payload_frame_tx, &payload, &xHigherPriorityTaskWoken))
	{
		set_payload_idle_isr(payload);
		//logFromISR("mm");
	}
	else
	{
		//set_payload_idle_isr(payload);
		//logFromISR("ss");
	}
}

static void payload_rx(void * payload)
{
    set_payload_idle_isr(payload);
	
	#if 0
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	if(NULL == phy_payload_frame_rx)
	{
		phy_payload_frame_rx = xQueueCreate(RX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t *));		
	}

	if(errQUEUE_FULL == xQueueSendFromISR(phy_payload_frame_rx, &payload, &xHigherPriorityTaskWoken))
	{
		
		set_payload_idle_isr(payload);
		logFromISR("mm");
	}
	else
	{
		//logFromISR("ss");
	}
	#endif
}
/**
Function: phy_payload_rx
Parameters: payload_channel_t * 
Description: receive payload(mdia) packet
Calls: 
Called By:phy_rx_func
*/
static void phy_payload_rx(payload_channel_t * payload_rx_channel)
{
//	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	static RxMediaStates RxMediaState = WAITINGABAB;
	static U32  RxMedia_IsFillingNext16 = 0;
	static U32  RxAMBE_IsFillingNext8 = 0;
	
	static U32  RxData_IsFillingNext16 = 0;
	
	
	static U32 RxBytesWaiting = 0;
	static U32 ArrayDiscLength = 0;
	
	static U16 * payload_ptr = NULL;
	static U8 * AMBE_payload_ptr = NULL;
	
	static Bool is_first = FALSE;
	volatile static U8 Item_ID = 0;
	volatile static U32	Item_Length = 0;
	
//	static U8  HT_index = 0;
	static U8 SDV_Index = 0;
	
//	static U8 _flag = 1;//0xABCDC014时，_flag为0；
						//0xABCDC010时，_flag为1；
			
	//payload_ptr_t *AMBE_payload_ptr;		
	
	if(is_first == FALSE)
	{
		payload_ptr = get_payload_idle_isr();
		AMBE_payload_ptr = get_payload_idle_isr();
		is_first = TRUE;
	}	
	
	//This is the RxMedia Phy Handler.
	switch (RxMediaState)
	{
		case WAITINGABAB:

			
			if (payload_rx_channel->dword[0] == 0xABCD5A5A)//Ignore Idles.
			{
				m_RxBurstType = VOICE_WATING;
				//Upon receiving the idle frame, the m Rx Burst Type into an idle state in order to transmit the synchronization wait
				 break; 
			}
            
			if ((payload_rx_channel->dword[0]  & 0xFFFF0000) != 0xABCD0000)break; //Skip until Header. 
				
#if 0
			//if (((payload_rx_channel->dword[0]  & 0x0000F000) == 0x00002000))break; 
			//if (((payload_rx_channel->dword[0]  & 0x0000F000) != 0x00001000) //media data from mic or speaker
			//&&((payload_rx_channel->dword[0]  & 0x0000F000) != 0x00002000))
			//break;   //Skip on non-DATA.
			//mylog("mic data:");
			//PAYLOAD_DATA_ENH (0x0c)
			//—Data routed to Option Board or to radio's main board, the data contents depend on Item field contained in payload bits.
			//if (((payload_rx_channel->dword[0]  & 0x0000F000) == 0x00005000)
			//|| ((payload_rx_channel->dword[0]  & 0x0000F000) == 0x00006000))
			//{
				//
				//logFromISR("\n\r Payload-Data_R/T \n\r");//测试收到的数据
				//
			//}
			//if (payload_rx_channel->word[0] == 0xABCD)
				{
			//if ((payload_rx_channel->dword[0] & 0xFFFFF000 ) == 0xABCDc000)//PAYLOAD_DATA_ENH (0x0c)
			//{
				//AMBE_flag = 1;
			//}
			//else
			//{
				//AMBE_flag = 0;
			//}
		
			//if (payload_rx_channel->dword[1] == 0x00010003)
			//{
				//Terminator_Flag = 1;
				////logFromISR("\n\r Stream Terminator Descriptor \n\r");
				////logFromISR("\n\r P: %X \n\r", payload_rx_channel->dword[0]);//测试收到的数据
				////logFromISR("\n\r P: %X \n\r", payload_rx_channel->dword[1]);//测试收到的数据
			//}
			//else
			//{
				//Terminator_Flag = 0;
			//}
			}
			//logFromISR("\n\r P: %X \n\r", payload_rx_channel->word[1]);//测试收到的数据
			
#endif
			
			RxBytesWaiting = payload_rx_channel->dword[0] & 0x000000FF;
			/****Note AMBE stream protocol frame structure and the PCM frame structure is different*****/
					
			if ((payload_rx_channel->dword[0] & 0x0000F000 ) == PAYLOAD_DATA_ENH )//PAYLOAD_DATA_ENH (0x0c))
			{
				AMBE_Media = 1;	
				
				if (NULL== AMBE_payload_ptr){
					logFromISR("\n\r xxxxx_QQ_xxxxx \n\r");//测试是否有这种情况出现
					break;
				}
																	
				Item_ID = payload_rx_channel->byte[5];
				
				Item_Length = (payload_rx_channel->byte[4] & 0x7F);//7bits
								
				switch(Item_ID)
				{
					//The OB know the Call begin and discard the Voice Header
					//The OB know the Call end and discard the Voice  Terminator
					case Raw_Tx_Data_HT://0xF0
					case Raw_Rx_Data_HT://0xF1
					
							if ((payload_rx_channel->byte[6] & 0xF0 )== 0x10)//header
							{
								m_RxBurstType = VOICEHEADER;

							}
							else if ((payload_rx_channel->byte[6] & 0xF0) == 0x20)//Terminator
							{
								m_RxBurstType = VOICETERMINATOR;
								//In order to complete the save data AMBE stream to SDcard.
								//AMBE-data and PCM-data is not the same. AMBE is compressed data,
								//if there was a missing portion, a clear voice is difficult to extract the data.
								//It must ensure that all the data received AMBE.
								
								//注意！！！考虑是否需要把剩余的空间置0。
								memset((AMBE_payload_ptr+ (RxAMBE_IsFillingNext8 +1)), 0x00, (512-(RxAMBE_IsFillingNext8 + 1)));
								
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								//logFromISR("\n\r QQ1 \n\r");
								
							}
							else//error voice
							{
								m_RxBurstType = VOICE_WATING;
							}
							
						break;//WAITINGABAB.
					
		
					case Vocoder_Bit_Stream_Parameter://0x12
							
							if ((RxBytesWaiting -= 4) <= 0) break;
					
							//Vocoder Bits Stream Parameter
							VF_SN = (payload_rx_channel->byte[7] & 0x1F);//This parameter is very important to the loop back Radio, as a reference.
							
							VBSP_data[0] = payload_rx_channel->word[2];
							VBSP_data[1] = payload_rx_channel->word[3];
							
							m_RxBurstType = CalculateBurst(VF_SN);
							
							RxMediaState = READING_AMBE_MEDIA;//Jump
							
					
						break;//Jump to READING_AMBE_MEDIA
							
					//Soft decision bits are not require to route back to Radio, the OB can either route it back radio
					// or discard it to reduce payload size.	
					//This shouldn't happen, but must check.	
					case Soft_Decision_Value://0x13
						m_RxBurstType = VOICE_WATING;
						
						break;//WAITINGABAB.
							
								
					case Radio_Internal_Parameter://0x7F
							
							//logFromISR("\n\r Item_Length:%d\n\r", Item_Length);
							
							if ((RxBytesWaiting -= 4) <= 0) break;
							//Radio Internal Parameter
							Radio_Internal_Data[RxData_IsFillingNext16] = payload_rx_channel->word[3];
							RxData_IsFillingNext16 += 1; 
							Item_Length -= 2 ;
							//....................................................
							m_RxBurstType = RADIOINTERNAL;

							RxMediaState = READING_AMBE_AUX;//Jump
							
					
						break;//Jump to READING_AMBE_AUX
									
					//case Post_Voice_Encoder_Data://0xF2
						//break;
									//
					//case Pre_Voice_Decoder_Data://0xF3
						//break;
									
					default://0x03,0x04
						
						//((Item_ID == 0x04) || (Item_ID == 0x03) )//Unknown type data directly back hair
							//break;
							m_RxBurstType = UNSUREDATA;
							AMBE_HT[0] = payload_rx_channel->dword[0];
							AMBE_HT[1] = payload_rx_channel->dword[1];
							
							if (RxBytesWaiting == 0x00000014)
							{
								RxBytesWaiting = 0x18;//24 Reassigned
								//_flag =0;
								
							}
							if (RxBytesWaiting == 0x00000010)
							{
								RxBytesWaiting = 0x10;//16 Reassigned
								//_flag =1;
							}
					
							RxMediaState = READING_AMBE_MEDIA;//Jump
					
						break;//Jump to READING_AMBE_MEDIA
									
			
				}
				break;
			}
			
			//else
			//{
				//AMBE_Media = 0;	
				//break;
			//}
			

//#if 0
			else//PCM-media-data
			{	
				//logFromISR("\n\r RX:%x \n\r", payload_rx_channel->dword[0]);
				//SPEAKER_DATA or  //MIC_DATA
				if (((payload_rx_channel->dword[0] & 0x0000F000 ) != SPEAKER_DATA ) 
					&& ((payload_rx_channel->dword[0] & 0x0000F000 ) != MIC_DATA ))break;
				
				AMBE_tx_flag = 0;
				AMBE_rx_flag = 0;
				
				Item_ID = 0;//To make sure your save PCM data.
			
				if (NULL== payload_ptr){
					logFromISR("\n\r xxxxx_QQ_xxxxx \n\r");//测试是否有这种情况出现
					break;
				}
				
				
				if ((payload_rx_channel->dword[0]  & 0x00000F00) <= 1){  //Flag type must process Array Descriptor.
				//The first word of the media access payload must be the Array descriptor length. And the
				//unit of the length is in word (16-bit). The length field itself does not count into the length.
				//When there is no array descriptor, the length must be set to zero.[9.1.4.1]
					if ((RxBytesWaiting -= 4) <= 0) break;          //Nothing beyond this Phy buffer. Keep looking for Header
					ArrayDiscLength = payload_rx_channel->word[2];
					switch (ArrayDiscLength){
						case 0:          //The usual case. Remaining word in Phy buffer is Audio.
								
							payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
							RxMedia_IsFillingNext16 += 1;
							if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
							{
								RxMedia_IsFillingNext16 = 0;	
								payload_rx(payload_ptr);	
								payload_ptr = get_payload_idle_isr();
								if(NULL == payload_ptr){
									break;
								}				
							}
							RxMediaState = READINGMEDIA;
						break;
				
						case 1: //The next usual case.
								//In general case, add code to process single word Array descriptor.
								if (payload_rx_channel->word[3] == 0x0003)//Stream Terminator
								{
									Terminator_Flag = 1;

								}
								else if(payload_rx_channel->word[3] == 0x0004)//Silent Descriptor
								{
							
									//Silent_flag = 1;
								}
								else if (payload_rx_channel->word[3] == 0x1026)//Tone Descriptor
								{
									//Tone_flag = 1;
							
								}
								else
								{
									//Terminator_Flag = 0;
								}
								
								RxMediaState = READINGMEDIA;
						break;
				
						default: //So far, can't happen, but need to code anyway.
								//In general, add code to process multi-word array descriptor.
								//ArrayDiscLength -= 1;
								//RxMediaState = READINGARRAYDISCRPT;
						break;
					}
				break;
			}
		
				//Code gets here on Middle or last Fragment. No Array descriptor.
				if (RxBytesWaiting < 2) break;//This shouldn't happen, but must check.
				payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->dword[1] & 0x0000FFFF;
				RxMedia_IsFillingNext16 += 1;
				if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
				{
					RxMedia_IsFillingNext16 = 0;
					payload_rx(payload_ptr);
					payload_ptr = get_payload_idle_isr();
					if(NULL == payload_ptr)
					{
						break;
					}
				}
				if ((RxBytesWaiting -= 2) <= 0) break;  //This shouldn't happen, but must check;
				RxMediaState = READINGMEDIA;
			}	
		
			break; //End of WAITINGABAB.

		case READINGMEDIA:
				
			/***PCM-media-data ****/
			{
				
				payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[0];
				RxMedia_IsFillingNext16 += 1;	
				if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
				{
					RxMedia_IsFillingNext16 = 0;
					payload_rx(payload_ptr);
					payload_ptr = get_payload_idle_isr();
					if(NULL == payload_ptr)
					{
						RxMediaState = WAITINGABAB;
						break;
					}
				}
				if ((RxBytesWaiting -= 2) <= 0)
				{
					RxMediaState = WAITINGABAB;
					break;
				}
		
				payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[1];
				RxMedia_IsFillingNext16 += 1;
				if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
				{
					RxMedia_IsFillingNext16 = 0;
					payload_rx(payload_ptr);
					payload_ptr = get_payload_idle_isr();
					if(NULL == payload_ptr)
					{
						RxMediaState = WAITINGABAB;
						break;
					}
				}
				if ((RxBytesWaiting -= 2) <= 0){
					RxMediaState = WAITINGABAB;
					break;
				}

				payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[2];
				RxMedia_IsFillingNext16 += 1;
				if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
				{
					RxMedia_IsFillingNext16 = 0;
					payload_rx(payload_ptr);
					payload_ptr = get_payload_idle_isr();
					if(NULL == payload_ptr)
					{
						RxMediaState = WAITINGABAB;
						break;
					}
				}
				if ((RxBytesWaiting -= 2) <= 0){
					RxMediaState = WAITINGABAB;
					break;
				}
		
				payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
				RxMedia_IsFillingNext16 += 1;
				if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
				{
					RxMedia_IsFillingNext16 = 0;
					payload_rx(payload_ptr);
					payload_ptr = get_payload_idle_isr();
					if(NULL == payload_ptr)
					{
						RxMediaState = WAITINGABAB;
						break;
					}
				}
				if ((RxBytesWaiting -= 2) <= 0){
					RxMediaState = WAITINGABAB;
					break;
				}
			}
		break; //End of READINGMEDIA.
//#endif

		case READING_AMBE_MEDIA:
		
			/****AMBE-media-data ****/
		
					if ((Item_ID == Vocoder_Bit_Stream_Parameter))//PAYLOAD_DATA_ENH (0x0c))
					{
						Item_ID = payload_rx_channel->byte[1];
						if (Item_ID == Post_Voice_Encoder_Data)//发送方的压缩类型数据
						{
							AMBE_tx_flag = 1;
							
							RxBytesWaiting = ((payload_rx_channel->dword[0] & 0x7F000000) >>24);//Test calculations are correct; 8
							
							//For looping back to Radio
							AMBEBurst_rawdata[0] = payload_rx_channel->word[1];
							AMBEBurst_rawdata[1] = payload_rx_channel->word[2];
							AMBEBurst_rawdata[2] = payload_rx_channel->word[3];
							
							//To be tested. Also locally stored RAW-AMBER-DATA
							
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[2];//1
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}			
							
							
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[3];//2
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}		
						
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[4];//3
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}
							
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[5];//4
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}
							
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[6];//5
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}
							
							AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[7];//6
							RxAMBE_IsFillingNext8 += 1;
							
							if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
							{
								RxAMBE_IsFillingNext8 = 0;
								payload_rx(AMBE_payload_ptr);
								AMBE_payload_ptr = get_payload_idle_isr();
								if(NULL == AMBE_payload_ptr)
								{
									RxMediaState = WAITINGABAB;
									break;
								}
							}
							if ((RxBytesWaiting -= 1) <= 0){
								RxMediaState = WAITINGABAB;
								break;
							}
							
							
							
							//payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[1];
							//RxMedia_IsFillingNext16 += 1;
							
							//if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(payload_ptr);
								//payload_ptr = get_payload_idle_isr();
								//if(NULL == payload_ptr)
								//{
									//RxMediaState = WAITINGABAB;
									//break;
								//}
							//}
							//if ((RxBytesWaiting -= 2) <= 0){
								//RxMediaState = WAITINGABAB;
								//break;
							//}
							
							//payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[2];
							//RxMedia_IsFillingNext16 += 1;
							//if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(payload_ptr);
								//payload_ptr = get_payload_idle_isr();
								//if(NULL == payload_ptr)
								//{
									//RxMediaState = WAITINGABAB;
									//break;
								//}
							//}
							//if ((RxBytesWaiting -= 2) <= 0){
								//RxMediaState = WAITINGABAB;
								//break;
							//}
							//
							//payload_ptr[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
							//RxMedia_IsFillingNext16 += 1;
							//if (RxMedia_IsFillingNext16 >= MAX_PAYLOAD_BUFF_SIZE)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(payload_ptr);
								//payload_ptr = get_payload_idle_isr();
								//if(NULL == payload_ptr)
								//{
									//RxMediaState = WAITINGABAB;
									//break;
								//}
							//}
							//if ((RxBytesWaiting -= 2) <= 0){
								//RxMediaState = WAITINGABAB;
								//break;
							//}
							
						}
						else//error
						{
							RxMediaState = WAITINGABAB;
							break;
						}
					}
					else if (Item_ID == Post_Voice_Encoder_Data)//(bit48~Pad-bits)
					{
					
						AMBEBurst_rawdata[3] = payload_rx_channel->word[0];//(bit48~Pad-bits)
						//AMBE_Per_Burst_Flag = 1;
						
						//根据MOTO.bit文件数据分布，建议测试pad-bits的最后一个字节不保存。
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[0];//7
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}	
						
						if ((RxBytesWaiting -= 1) <= 0){//此处丢弃AMBE Vocoder Bits Stream(bit59~63),不予保存
							RxMediaState = WAITINGABAB;
							break;
						}	
						
								
					}
					else if (Item_ID == Pre_Voice_Decoder_Data)//bit16~bit63
					{
						//接收方的解压类型数据
						//For looping back to Radio
						AMBEBurst_rawdata[1] = payload_rx_channel->word[0];
						AMBEBurst_rawdata[2] = payload_rx_channel->word[1];
						AMBEBurst_rawdata[3] = payload_rx_channel->word[2];
						
						//if((m_RxBurstType == VOICEBURST_A) && (VF_SN == 1))
						//logFromISR("\n\r Pre_rx:%X\n\r", payload_rx_channel->word[2]);
						
						//To be tested. Also locally stored RAW-AMBER-DATA
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[0];//3
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}
						
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[1];//4
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[2];//5
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[3];//6
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}
						
						AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[4];//7
						RxAMBE_IsFillingNext8 += 1;
						
						if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
						{
							RxAMBE_IsFillingNext8 = 0;
							payload_rx(AMBE_payload_ptr);
							AMBE_payload_ptr = get_payload_idle_isr();
							if(NULL == AMBE_payload_ptr)
							{
								RxMediaState = WAITINGABAB;
								break;
							}
						}
						if ((RxBytesWaiting -= 1) <= 0){
							RxMediaState = WAITINGABAB;
							break;
						}
						
						//此处丢弃AMBE Vocoder Bits Stream(bit59~63),不予保存
						if ((RxBytesWaiting -= 1) <= 0){
							
							//Soft decision bits are not require to route back to Radio, the OB can either route it back radio
							//or discard it to reduce payload size.
							
							RxMediaState = WAITINGABAB;
							break;
						}	
						
					}
					else if ((Item_ID == 0x04)  ||  (Item_ID == 0x03))//Unknown type data directly back hair
					{
						AMBE_HT[0] = payload_rx_channel->dword[0];
						AMBE_HT[1] = payload_rx_channel->dword[1];				
						
						if ((RxBytesWaiting -= 8) <= 0)//Consider two cases 0xABCDC014 and 0xABCDC010
						{
					
							RxBytesWaiting = 0;
							RxMediaState = WAITINGABAB;
							break;
			
						}
						break;

					}
					else if (Item_ID == Soft_Decision_Value)
					{							
						if (SDV_Index == 12)
						{
							Soft_Decision_Data[SDV_Index]= payload_rx_channel->word[0];
							
							SDV_Index = 0;
								
							if (payload_rx_channel->byte[5] == Pre_Voice_Decoder_Data )//0xF3
							{
									Item_ID = Pre_Voice_Decoder_Data;
									RxBytesWaiting = (payload_rx_channel->byte[4] & 0x7F);//Test calculations are correct; 8
									AMBE_rx_flag = 1;//本地作为AMBE数据的解密方
		
									AMBEBurst_rawdata[0] = payload_rx_channel->word[3];//用于解密的中间数据变量
									
									AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[6];//1
									RxAMBE_IsFillingNext8 += 1;
									
									if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
									{
										RxAMBE_IsFillingNext8 = 0;
										payload_rx(AMBE_payload_ptr);
										AMBE_payload_ptr = get_payload_idle_isr();
										if(NULL == AMBE_payload_ptr)
										{
											RxMediaState = WAITINGABAB;
											break;
										}
									}
									if ((RxBytesWaiting -= 1) <= 0){
										RxMediaState = WAITINGABAB;
										break;
									}
									
									AMBE_payload_ptr[RxAMBE_IsFillingNext8] = payload_rx_channel->byte[7];//2
									RxAMBE_IsFillingNext8 += 1;
									
									if (RxAMBE_IsFillingNext8 >= (2*MAX_PAYLOAD_BUFF_SIZE))
									{
										RxAMBE_IsFillingNext8 = 0;
										payload_rx(AMBE_payload_ptr);
										AMBE_payload_ptr = get_payload_idle_isr();
										if(NULL == AMBE_payload_ptr)
										{
											RxMediaState = WAITINGABAB;
											break;
										}
									}
									if ((RxBytesWaiting -= 1) <= 0){
										RxMediaState = WAITINGABAB;
										break;
									}
									
							}
							else
							RxMediaState = WAITINGABAB;
								
						}
						else
						{
									
							Soft_Decision_Data[SDV_Index]	  = payload_rx_channel->word[0];
							Soft_Decision_Data[SDV_Index+1]	  = payload_rx_channel->word[1];
							Soft_Decision_Data[SDV_Index+2]	  = payload_rx_channel->word[2];
							Soft_Decision_Data[SDV_Index+3]	  = payload_rx_channel->word[3];
							SDV_Index +=4;
									
						}
							
							break;
						
					}
					
					else
					{
						
						RxMediaState  = WAITINGABAB;
						break;
					}
			
			
		break;//End of READING_AMBE_MEDIA.

        case READING_AMBE_AUX:
			
				Radio_Internal_Data[RxData_IsFillingNext16] = payload_rx_channel->word[0];
				RxData_IsFillingNext16 += 1;
			
				if ((Item_Length -= 2) <= 0)
				{
					RxData_IsFillingNext16 = 0;
				
					if (payload_rx_channel->word[1] == 0x8212 )
					{
						//Item_ID = Vocoder_Bit_Stream_Parameter;
						//Vocoder Bits Stream Parameter
						//Its value ranges from 1 to 18 for one voice superframe contains 6 voice bursts
						// and each burst contains 3 VFs.
						VF_SN = (payload_rx_channel->byte[5] & 0x1F);//This parameter is very important to the loop back Radio, as a reference.
							
						VBSP_data[0] = payload_rx_channel->word[1];
						VBSP_data[1] = payload_rx_channel->word[2];
							
						m_RxBurstType = CalculateBurst(VF_SN);
						
						if (payload_rx_channel->byte[7] == Soft_Decision_Value)//0x13
						{
							Item_ID = Soft_Decision_Value;	
							RxBytesWaiting = ((payload_rx_channel->dword[1] & 0x00007F00) >>24);//Test calculations are correct; 26
						}
						else
						{//error
								RxMediaState = WAITINGABAB;//Jump
						}
							
						RxMediaState = READING_AMBE_MEDIA;//Jump
					}
					else
					{
						RxMediaState = WAITINGABAB;//Jump
					}

				}
				else
				{//测试看看是几个字节数据
					logFromISR("\n\r Radio Internal parameter length error \n\r");
					RxMediaState = WAITINGABAB;//Jump
				}

		 
		break;//End of READING_AMBE_AUX.
		 
#if 0

		 
		//case READINGARRAYDISCRPT:  //So far, this cannot happen, but needed for forward compatibility.
		////Array descriptorLength is greater than 0 on entry here.
		//if(ArrayDiscLength > 4){ //All 4 words are still array discriptor.
			////For now, continue discarding.
			//ArrayDiscLength -= 4;
			//RxBytesWaiting  -= 8;
			//if (RxBytesWaiting <= 0)RxMediaState = WAITINGABAB;
			//break;
		//}
//
		//switch (ArrayDiscLength){ //1,2, or 3
			//case 3:
			//if ((RxBytesWaiting -= 6) <= 0) break;  //Throw away 3 hWords.
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//RxBytesWaiting -= 2;
			//break;
			//case 2:
			//if ((RxBytesWaiting -= 4) <= 0) break;  //Throw away 2 hWords.
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[2];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//if ((RxBytesWaiting -= 2) <= 0)break;
//
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//RxBytesWaiting -= 2;
			//break;;
			//case 1:
			//if ((RxBytesWaiting -= 2) <= 0) break;  //Throw away 1 hWords.
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[1];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//if ((RxBytesWaiting -= 2) <= 0)break;
//
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[2];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//if ((RxBytesWaiting -= 2) <= 0)break;
//
			//phy_frame.fragment_element[RxMedia_IsFillingNext16] = payload_rx_channel->word[3];
			//RxMedia_IsFillingNext16 += 1;
			//if (RxMedia_IsFillingNext16 >= 128)
							//{
								//RxMedia_IsFillingNext16 = 0;
								//payload_rx(phy_frame.fragment_element);
							//}
			//RxBytesWaiting -= 2;
			//break;
		//}
		//if (RxBytesWaiting <= 0){
			//RxMediaState = WAITINGABAB;
			//}else{
			//RxMediaState = READINGMEDIA;
		//}
		//break;  //End of READINGARRAYDISCRPT.


#endif
		case BGFORCERESET: //Do nothing.
		break;
	}//End of RxMedia Phy Handler.
}
#endif /*end if*/

void * get_idle_store(xQueueHandle store)
{
	void * ptr = NULL;

	if(pdTRUE == xQueueReceive(store, &ptr, 0))
	{
		return ptr;
	}
	else
	{
		return NULL;
	}
}

void * get_idle_store_isr(xQueueHandle store)
{
	void * ptr = NULL;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	if(pdTRUE == xQueueReceiveFromISR(store, &ptr, &xHigherPriorityTaskWoken))
	{
		return ptr;
	}
	else
	{
		return NULL;
	}
}


void set_idle_store(xQueueHandle store, void * ptr)
{
	xQueueSend(store, &ptr, 0);
}

void set_idle_store_isr(xQueueHandle store, void * ptr)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(store, &ptr, &xHigherPriorityTaskWoken);
}





