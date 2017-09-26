/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: physical.h
Author: wxj
Version: 1.0.0.01
Date: 2016/3/25 12:02:34

Description:
History:
*/

#ifndef PHYSICAL_H_
#define PHYSICAL_H_

#include "xnl.h"
#include "payload.h"

#define MAX_XNL_STORE	 30

/*the queue depth is used to create xnl queue*/
#define TX_XNL_QUEUE_DEEP	20
#define RX_XNL_QUEUE_DEEP	10

/*Enable payload(media) or disable*/
#define PAYLOAD_ENABLE	ENABLE	

/*Enable playback(media) or disable*/
#define PLAYBACK_ENABLE ENABLE

#define MAX_PAYLOAD_BUFF_SIZE 256
#define MAX_PAYLOAD_STORE 10
/*the queue depth is used to create the payload(media) queue*/
#define TX_PAYLOAD_QUEUE_DEEP	5
#define RX_PAYLOAD_QUEUE_DEEP	5

/*
16-bit raw PCM audio data sampled at 8 kHz routed to the radio¡¯s main speaker
from the option board when the option board makes audio output connection, and
routed to the option board from the radio¡¯s main speaker when the option board
makes audio inputconnection.
*/
#define SPEAKER_DATA 0x00001000

/*
16-bit raw PCM audio data sampled at 8 kHz routed to the radio¡¯s microphone
from the option board when the option board makesaudio output connection, and
routed to the option board from the radio¡¯s microphone when the option board
makes audio input connection.
 */
#define MIC_DATA 0x00002000

/*
XCMP / XNL data packet sent to or received from the option board.
*/
#define XCMPXNL_DATA 0x00004000

/*
the 16-bit raw PCM de-modulated audio, or voice-band audio sampled at 8 kHz or
20 kHz when radio is in receive mode.It is routed to the radio¡¯s main board
from the option board when the option board makes audio output connection, and
routed to the option board from the radio when the option board makes audio 
input connection.
*/
#define PAYLOAD_DATA_RX 0x00005000


/*
PAYLOAD_DATA_ENH (0x0c)¡ªData routed to Option Board or to radio's main
board, the data contents depend on Item field contained in payload bits.

*/


#define PAYLOAD_DATA_ENH 0x0000C000


/*
the 16-bit raw PCM de-modulated audio, or oice-band audio sampled at 8 kHz 
when radio is in transmission mode. It is routed to the radio¡¯s main board 
from the option board when the option board makes audio output connection, and 
routed to the option board from the radio when the option board makes audio 
input connection.
 */
#define PAYLOAD_DATA_TX 0x00006000

/*
Used when the total data length is less than or  equal to the MTU size (254 
bytes). Indicates there is only one transfer unit in the sequence. For XNL 
Channel, the 2-byte checksum is counted in the MTU  size, therefore the 
maximum bytes for the payload in one fragment is MTU ¨C 2.
*/
#define SINGLE_FRAGMENT  0x00000000

/*
Used when the total data length is greater than the MTU size (254 bytes). 
For transfer unit 0, indicates the first transfer unit in the sequence.
*/
#define FIRST_FRAGMENT 0x00000100

/*
Used when the total data length is greater than 2 x MTU size (508 bytes).
For transfer units 1 to M ¨C 2 (where M >= 3),indicates the middle transfer 
units in the sequence.
 */
#define MIDDLE_FRAGMENT 0x00000200

/*
Used when the total data length is greater than the MTU size (254 bytes)
and the remaining data length of the last transfer unit is less than or equal to the MTU size (254 bytes).
For transfer unit M-1 (where M >= 2), indicates the last transfer unit in the sequence.*/
#define LAST_FRAGMENT	0x00000300

//Item type Aux

#define Vocoder_Bit_Stream_Parameter	0x12
#define Soft_Decision_Value	0x13
#define Radio_Internal_Parameter	0x7f

//Item type Data
#define Raw_Tx_Data_HT	0xF0
#define Raw_Rx_Data_HT	0xF1
#define Post_Voice_Encoder_Data	0xF2
#define Pre_Voice_Decoder_Data	0xF3


/*send/receive SSC packet status*/
typedef enum
{
	WAITING_FOR_PHY_TX, 
	WRITE_NEXT_DWORD,
	SEND_TAILED,
}phy_tx_state_t;

typedef enum {
	
	/*Waiting for something. Most frequent visit.*/	
	WAITING_FOR_HEADER,
	READING_FRAGMENT,
	
	/*
	Gets here on CSUM. Expect at least one hWord payload. Gets here once 
	on every fragment.*/	
	WAITING_CHECK_SUM,
	
	/*Expecting last terminator 0x00BA0000.*/	
	WAITING_LAST_TERM
} phy_rx_state_t;


//typedef enum {
	//
	//WAITING_FOR_ABAB,
	//
	//READING_ARRAY_DISCRPT,
	//
	//READING_MEDIA,
	//
	//BG_FORCE_RESET
//} media_rx_state_t;  //enums are 32 bits.

  typedef enum {
	  WAITINGABAB,
	  READINGARRAYDISCRPT,
	  READINGMEDIA,
	  READING_AMBE_AUX,
	  READING_AMBE_MEDIA,
	  BGFORCERESET,
  } RxMediaStates;  //enums are 32 bits.
  
  typedef enum {
	  
	  AMBE_IDLE,
	  AMBE_EN_FIRST,
	  AMBE_EN_LAST,
	  AMBE_DE_FIRST,
	  AMBE_DE_SECOND,
	  AMBE_DE_THIRD,
	  AMBE_DE_LAST,  
	   
   }AMBEPayloadTxStates; //enum are 32 bits.

  //typedef enum {
	  //
	  //VOICEHEADER,
	  //VOICEBURST_A,
	  //VOICEBURST_B,
	  //VOICEBURST_C,
	  //VOICEBURST_D,
	  //VOICEBURST_E,
	  //VOICEBURST_F,
	  //VOICETERMINATOR
	  //
  //} RxAMBEBurstType;  //enum are 32 bits.


typedef union {
	xnl_fragment_t	xnl_fragment;
	payload_fragment_t payload_fragment;
	U16				fragment_element[128];
} phy_fragment_t;


void * get_idle_store(xQueueHandle store);
void * get_idle_store_isr(xQueueHandle store);
void set_idle_store(xQueueHandle store, void * ptr);
void set_idle_store_isr(xQueueHandle store, void * ptr);

//#define get_xnl_idle_isr(ptr, woken)  xQueueReceiveFromISR(xnl_store_idle, ptr,woken)
//#define set_xnl_idle_isr(ptr, woken)  xQueueSendFromISR(xnl_store_idle, ptr, woken)
//#define get_xnl_idle(ptr)	xQueueReceive(xnl_store_idle, ptr, 0)
//#define set_xnl_idle(ptr)   xQueueSend(xnl_store_idle, ptr, 0)

#define get_xnl_idle_isr()  get_idle_store_isr(xnl_store_idle)
#define get_xnl_idle()	get_idle_store(xnl_store_idle)
#define set_xnl_idle_isr(ptr)  set_idle_store_isr(xnl_store_idle, ptr)
#define set_xnl_idle(ptr)   set_idle_store(xnl_store_idle, ptr)

extern volatile phy_fragment_t xnl_store[MAX_XNL_STORE];
extern volatile xQueueHandle xnl_store_idle;

/*queue is used to receive xnl*/
extern volatile xQueueHandle phy_xnl_frame_rx;
extern volatile xQueueHandle phy_xnl_frame_tx;
/*if enable send/receive payload(media)*/
#if ENABLE == PAYLOAD_ENABLE

//#define get_payload_idle_isr(ptr, woken)  xQueueReceiveFromISR(payload_store_idle, ptr,woken)
//#define set_payload_idle_isr(ptr, woken)  xQueueSendFromISR(payload_store_idle, ptr, woken)
//#define get_payload_idle(ptr)	xQueueReceive(payload_store_idle, ptr, 0)
//#define set_payload_idle(ptr)   xQueueSend(payload_store_idle, ptr, 0)

#define get_payload_idle_isr()  get_idle_store_isr(payload_store_idle)
#define get_payload_idle()	get_idle_store(payload_store_idle)
#define set_payload_idle_isr(ptr)  set_idle_store_isr(payload_store_idle, ptr)
#define set_payload_idle(ptr)   set_idle_store(payload_store_idle, ptr)


extern volatile U16 payload_store[MAX_PAYLOAD_STORE][MAX_PAYLOAD_BUFF_SIZE];
extern volatile xQueueHandle payload_store_idle;

/*queue is used to receive payload(media)*/
extern volatile xQueueHandle phy_payload_frame_rx;
extern volatile xQueueHandle phy_payload_frame_tx;

#endif /*end if*/

/*
initialize the SSC;
register the func to send/receive ssc packet;
initialize the queue
*/
void phy_init( void );

/*separate XNL and payload(media), then push packet to the queue*/
void phy_tx(phy_fragment_t * phy);


#endif /* PHYSICAL_H_ */