/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: xnl.c
Author: wxj
Version: 1.0.0.01
Date: 2016/3/15 14:02:54

Description:
History:
*/

#include <string.h>

/*include files are used to FreeRtos*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

#include "physical.h"
#include "xnl.h"

/*encipher,to used connect (auth)*/
#include "key.h"

#include "../log/log.h"
#include "ssc.h"
#include "xcmp.h"

/*information of xnl*/
volatile xnl_information_t xnl_information;

/*Defines the callback function is used to receive XCMP*/
void ( *xcmp_exec)(xnl_content_data_msg_t) = NULL;

/*send/receive XNL in task*/
static void xnl_tx_process(void * pvParameters);
static void xnl_rx_process(void * pvParameters);

/*queue is used to send XNL message*/
static volatile xQueueHandle xnl_frame_tx = NULL;

/*semaphore is used to wait reply message of XNL*/
volatile xSemaphoreHandle xnl_timeout_semphr = NULL;

extern void xcmp_audio_route_speaker(void);

/**
Function: encipher
Parameters: U32 *const v, U32 *const w, const U32 *const k
Description: encipher the key to authentication and connect radio
Calls:   
Called By: xnl_device_auth_reply_func
Output: U32 *const w
*/
static void encipher(U32 *const v, U32 *const w, const U32 *const k)
{
	register U32 y=v[0], z=v[1], sum=0;
	register U32 a=k[0], b=k[1], c=k[2], d=k[3];
	register U32 n=32;

	while(n-->0)
	{
		sum += authDelta;		
		y += ((z << 4)+a) ^ (z+sum) ^ ((z >> 5)+b);
		z += ((y << 4)+c) ^ (y+sum) ^ ((y >> 5)+d);
	}

	w[0]=y; w[1]=z;
}

/**
Function: encipher
Parameters: xnl_fragment_t *
Description: For transmission or reception of XCMPXNL data, checksum is
	qrequired for all fragments of XCMPXNL payload.
Calls:   
Called By: xnl_tx
Return:U16
*/
static U16 check_sum (xnl_fragment_t * xnl)
{
	/*
	For transmission or reception of XCMPXNL data, checksum is required for 
	all fragments of XCMPXNL payload. Checksum field is used for ensuring the 
	correct XCMPXNL payload transmission. For all other data type, the 
	checksum field shall not be inserted in the fragments. 
	
	The checksum is computed by summing all XCMPXNL payload words (16 bits) in 
	each fragment excluding the Checksum word , taking lower 16 bits of the 
	results and performing 2 complement of that result. Each 16-bit payload 
	word is used as an unsigned integer in the checksum computation. The 
	checksum computation must include any stuffed bits.
	*/	
	U16  sumScratch;
	U32  indextohWord;
	S32  hWordswithinFrag;
	
	sumScratch = 0;
	hWordswithinFrag = ((xnl->phy_header.phy_control) & 0x00FF) - 2;
	hWordswithinFrag =  (hWordswithinFrag + (hWordswithinFrag & 0x0001)) >> 1; 
	indextohWord = 2;
	while (hWordswithinFrag > 0)
	{
		sumScratch += *((U16*)xnl + indextohWord);
		indextohWord     += 1;
		hWordswithinFrag -= 1;
	}
	
	return -sumScratch;//反码
}

/**
Function: xnl_send_device_master_query
Description: send device_master_query to connect radio.
Calls:xnl_tx
Call By:xnl_init
*/
void xnl_send_device_master_query(void)
{
	/*
	All non-master devices must wait until a MASTER_STATUS_BRDCST is received. 
	If no MASTER_STATUS_BRDCST message is received within 500 ms, then the 
	non-master device should send a DEVICE_MASTER_QUERY message. If the master 
	is present, then it will respond with a MASTER_STATUS_BRDCST message. If a 
	master is not present, then the device shall re-send the 
	DEVICE_MASTER_QUERY message. This will handle the differences in power up 
	times, as well as the case when a device is connected after the rest of 
	the system has already powered up. 
	5.2.1 XCMP/XNL Development Guide. 
	This code provides for this retry by creating an intance of the 
	DEVICE_MASTER_QUERY message, but not sending it. Should the 
	MASTER_STATUS_BRDCST not be received before the retry timout, the 
	DEVICE_MASTER_QUERY will then be sent.
	*/
		
	/*XNL frame will be send*/	
	xnl_fragment_t xnl_frame;

	/*
	Data Type 0x4000
	Fragment Type:0
	Length :12 + 2(xnl length(12) + checksum(2)) 
	*/
	xnl_frame.phy_header.phy_control = ((0x4000 | 12) + 2);
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.phy_header.check_sum = DEFAULT_VALUE;
	
	/*Insert opcode*/
	xnl_frame.xnl_header.opcode = XNL_DEVICE_MASTER_QUERY;
	
	/*The initial value*/
	xnl_frame.xnl_header.protocol_id = XNL_PROTO_XNL_CTRL;
	
	xnl_frame.xnl_header.xnl_flags = 0;
	
	/*XNL address of the master device, if known;otherwise 0x0000*/
	xnl_frame.xnl_header.destination = 0;
	
	/*XNL address, if assigned; otherwise 0x0000*/
	xnl_frame.xnl_header.source = 0;
	
	/*No transaction ID required for this message*/
	xnl_frame.xnl_header.transaction_id = 0;
	
	/*This message contains no payload*/
	xnl_frame.xnl_header.payload_length = 0;

	/*send XNL message*/
	xnl_tx(&xnl_frame);
}

/**
Function: xnl_master_status_brdcst_func
Description: process while receive master status boardcast.
Calls:xSemaphoreGive--freestos,xnl_tx
Register:xnl_proc_list exec in xnl_rx_process
*/
static void xnl_master_status_brdcst_func(xnl_fragment_t * xnl)
{	
	/*XNL frame will be send*/	
	xnl_fragment_t xnl_frame;
	//log_debug("rx master status brdcast.");
	//log_debug("T_xnl-opcode:%4x", xnl->xnl_header.opcode);
	
	/*
	The XNL_MASTER_STATUS_BRDCST message is sent out by the master device to 
	indicate that the master has been determined and that non-master devices 
	can now connect. The data payload for this message will contain the XNL 
	version as well as the logical device identifier for the master device. 
	The last field in the payload contains a flag that indicates whether or 
	not an XNL_DATA_MSG has been sent out. This will indicate to a connecting 
	device that it has missed messages. The XNL header will contain the 
	master?XNL address. 5.4.1
	*/
	
	if(xnl_information.is_connected)
	{
		return;	
	}
		
	/*No timeout*/	
	//xSemaphoreGive(xnl_timeout_semphr);	

	//log_debug("xnl-ma:%4x", xnl->xnl_header.source);
	/*get the master adderss from this message*/	
	xnl_information.master_address = xnl->xnl_header.source;	
	
	/*
	Data Type 0x4000
	Fragment Type:0
	Length :12 + 2(xnl length(12) + checksum(2)) 
	*/
	xnl_frame.phy_header.phy_control = ((0x4000 | 12) + 2);
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.phy_header.check_sum = DEFAULT_VALUE;
	
	/*Insert opcode*/
	xnl_frame.xnl_header.opcode = XNL_DEVICE_AUTH_KEY_REQUEST;
		
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/	
	xnl_frame.xnl_header.protocol_id = xnl->xnl_header.protocol_id;
	
	xnl_frame.xnl_header.xnl_flags = xnl->xnl_header.xnl_flags;
	
	/*Use actual Master address.*/
	xnl_frame.xnl_header.destination = xnl_information.master_address;
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.xnl_header.source = DEFAULT_VALUE;
	xnl_frame.xnl_header.transaction_id = DEFAULT_VALUE;
		
	xnl_frame.xnl_header.payload_length = 0;
	
	/*send XNL message*/	
	xnl_tx(&xnl_frame);
}


/**
Function: xnl_device_auth_reply_func
Description: process while receive device auth reply.
Calls:xSemaphoreGive--freestos,xnl_tx
Register:xnl_proc_list exec in xnl_rx_process
*/
static void xnl_device_auth_reply_func(xnl_fragment_t * xnl)
{
	U32 v_vector[2], w_vector[2];
	
	/*XNL frame will be send*/	
	xnl_fragment_t xnl_frame;
	
	/*
	The payload for XNL_DEVICE_AUTH_KEY_REPLY message is a temporary XNL 
	address to use during the connection process and an unencrypted 8 byte 
	random number generated by the master. This number should be encrypted by 
	the receiving device and will be used to authenticate the connection 
	request. 5.4.4
	*/
	//log_debug("R_xnl-opcode:%4x", xnl->xnl_header.opcode);
	//log_debug("rx auth key reply.");
	if(xnl_information.is_connected)
	{
		return;		
	}
	
	/*No timeout*/		
	xSemaphoreGive(xnl_timeout_semphr);

	
	/*Temporarily use temporary device address*/
	xnl_information.device_address = 
	    xnl->xnl_payload.xnl_content_device_auth_key_reply.temporary_xnl_address;
	
	/*Get Array of values to be encrypted into an aligned 2X32bits.*/
	v_vector[0] =
	  (xnl->xnl_payload.xnl_content_device_auth_key_reply
	  .unencrypted_authentication_value[0])<<24	  
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	  .unencrypted_authentication_value[1])<<16	  
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	  .unencrypted_authentication_value[2])<<8	  
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	  .unencrypted_authentication_value[3]);
	
	v_vector[1] =
	  (xnl->xnl_payload.xnl_content_device_auth_key_reply
	  .unencrypted_authentication_value[4])<<24
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	   .unencrypted_authentication_value[5])<<16
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	   .unencrypted_authentication_value[6])<<8
	| (xnl->xnl_payload.xnl_content_device_auth_key_reply
	   .unencrypted_authentication_value[7]);
	
	encipher(&v_vector[0], &w_vector[0], &authKey[0]);
	
	/*
	This message is sent by all non-master devices in order to establish a 
	logical connection with the master. If a particular XNL address is desired 
	(for fixed address systems), a preferred address field should be used. 
	Otherwise a value of 0x0000 should be used. For systems that contain both 
	fixed address and dynamic address assignments, the preferred address 
	cannot be guaranteed. The payload for this message also includes a device 
	type value, authentication index, and the encrypted authentication value. 
	XCMP/XNL Development Guide 5.4.5
	*/
	
	/*
	Data Type 0x4000
	Fragment Type:0
	Length :24 + 2(xnl length(24) + checksum(2)) 
	*/
	xnl_frame.phy_header.phy_control = ((0x4000 | 24) + 2);
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.phy_header.check_sum = DEFAULT_VALUE;
	
	/*Insert opcode*/
	xnl_frame.xnl_header.opcode = XNL_DEVICE_CONN_REQUEST;
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.xnl_header.protocol_id = xnl->xnl_header.protocol_id;
	
	xnl_frame.xnl_header.xnl_flags = 0;//enable data ack
	/*
		XNL_DEVICE_CONN_REQUEST:
		
		For XCMP v0.0.0.4 and Previous
		Value = 0x00
		
		For XCMP v0.0.0.5 and Later
		0x00 – Device requests that XNL_ACKs are
		enabled for bidirectional XNL_DATA_MSGs
		between the device and the radio.
		0x08 – Device requests that XNL_ACKs are
		disabled for bidirectional XNL_DATA_MSGs
		between the device and the radio.
	
	*/
	
	
	/*Use actual Master address.*/
	xnl_frame.xnl_header.destination = xnl_information.master_address;
	
	/*Use Temporary address.*/
	/*
  	There is a possible flaw in the XNL protocol; several Option Cards may 
	power up at about the same time, XNL_DEVICE_AUTH_KEY_REPLY. Thus their 
	Temporary Address will also be the same. So, they will be sending the same 
	message here, and all will receive the same DEVICE_CONN_REPLY. Not real 
	sure what's going to happen with multiple conrol heads, etc. One suspects 
	the Rocket Scientists will eventually figure this out, and demand a 
	transaction ID based on Device Type in the XNL_DEVICE_AUTH_KEY_REQUEST.
	*/
	xnl_frame.xnl_header.source = xnl_information.device_address;
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.xnl_header.transaction_id = DEFAULT_VALUE;
	
    /*his message contains 12 payload bytes*/	
	xnl_frame.xnl_header.payload_length = 0x000C;
	
	/*No Preferred XNL Address*/
	xnl_frame.xnl_payload.xnl_content_device_conn_req.preferred_xnl_address = 
																		 0x0000;
	
	/*
	XCMP/XNL Development Specification Section 4.5.3.2.1.
	Same as in MOTOTRBO?XCMP/XNL Development Specification?
	evice_type--Option Board(0x07)
	authentication_index--Option Board(0x02)
	*/	
	xnl_frame.xnl_payload.xnl_content_device_conn_req.device_type = 0x07;
	xnl_frame.xnl_payload.xnl_content_device_conn_req.authentication_index =
	                                                                       0x02;
		
	//We know encrypted array happens to be aligned to 32-bit boundary.
	*((U32 *)(&xnl_frame.xnl_payload.xnl_content_device_conn_req
	                         .encrypted_authentication_value[0])) = w_vector[0];
	*((U32 *)(&xnl_frame.xnl_payload.xnl_content_device_conn_req
	                         .encrypted_authentication_value[4])) = w_vector[1];
	
	/*send XNL message*/
	xnl_tx(&xnl_frame);
}

/**
Function: xnl_device_conn_reply_func
Description: process while receive device conn reply.
Calls:xSemaphoreGive--freestos
Register:xnl_proc_list exec in xnl_rx_process
*/
static void xnl_device_conn_reply_func(xnl_fragment_t * xnl)
{
	/*Bool MasterAuthenticate;
	The reply to the Device Connection Request contains a result code 
	(connection successful or not) and the XNL address and device logical 
	address to use for all future communications. In addition, the reply will 
	contain a unique 8-bit value that should be used as the upper byte of the 
	transaction ID and an 8-byte encrypted value that the device can use to 
	authenticate the master. XCMP/XNL Development Guide 5.4.6
	*/
	
	/*No timeout*/	
	xSemaphoreGive(xnl_timeout_semphr);
	
	/*Test result code*/
	if((xnl->xnl_payload.xnl_content_device_conn_reply.result_base & 0x0000FF00)
		!= 0x00000100)
	{
		/*
		Rejected. The device must retry the authentication process at this 
		point by sending out a new AUTH_KEY_REQUEST message. XCMP/XNL 
		Development Guide Section 5.2.3
		*/
		log_debug("xnl connection failure!");
		xnl_master_status_brdcst_func(xnl);			
	}
	else
	{
		/*connection accepted*/		
		/*Record Transaction ID Base*/
		xnl_information.transaction_id =  
					(xnl->xnl_payload.xnl_content_device_conn_reply.result_base
					& 0x000000FF) << 8;
		
		/*Record Device Logical Address*/
		xnl_information.logical_address = 
		          xnl->xnl_payload.xnl_content_device_conn_reply.logical_address;
		
		/*Record permanent device address*/
		xnl_information.device_address = 
				      xnl->xnl_payload.xnl_content_device_conn_reply.xnl_address;
		
		/*connect finish*/
		xnl_information.is_connected = TRUE;
		
		log_debug("xnl connection success.");
	}
	
	
}


static void xnl_device_sysmap_brdcst_func(xnl_fragment_t * xnl)
{
	log_debug("ob rx sysmap:");
	//log_debug("map_array: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.sizeof_sysmap_array);
	//log_debug("logical ident[0]: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.u8[0]);
	//log_debug("logical ident[1]: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.u8[1]);
	//log_debug("xnl addr[0]: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.u8[2]);
	//log_debug("xnl addr[1]: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.u8[3]);
	//log_debug("authen index: %d", xnl->xnl_payload.xnl_content_device_sysmap_brdcst.u8[4]);
	//
}

/**
Function: xnl_send_msg_ack
Description: send data message ack.
Calls:xnl_tx
Register:xnl_data_msg_func
*/
static void xnl_send_msg_ack(xnl_header_t * hdr)
{	
	/*XNL frame will be send*/	
	xnl_fragment_t xnl_frame;

	/*
	Data Type 0x4000
	Fragment Type:0
	Length :12 + 2(xnl length(12) + checksum(2)) 
	*/
	xnl_frame.phy_header.phy_control = ((0x4000 | 12) + 2);
	
	/*If the value is DEFAULT_VALUE, then say the value will be modified in 
	the xnl_tx*/
	xnl_frame.phy_header.check_sum = DEFAULT_VALUE;
	
	/*Insert opcode*/
	xnl_frame.xnl_header.opcode = XNL_DATA_MSG_ACK;
	
	/*Turn around Flags.*/	
	//xnl_frame.xnl_header.flags = hdr->flags;
	
	xnl_frame.xnl_header.protocol_id = hdr->protocol_id;
	
	xnl_frame.xnl_header.xnl_flags = hdr->xnl_flags;
	
	/*ACK Destination Address is Source of XNL_Message.*/
	xnl_frame.xnl_header.destination = hdr->source;
	
	/*ACK Source Address is my address.*/
	xnl_frame.xnl_header.source = hdr->destination;
	
	/*Turn around Transaction ID.*/
	xnl_frame.xnl_header.transaction_id = hdr->transaction_id;
	xnl_frame.xnl_header.payload_length = 0;

	/*send XNL message*/
	xnl_tx(&xnl_frame);
}

/**
Function: xnl_data_msg_func
Description: process while receive data message.
Calls:xnl_send_msg_ack, xcmp_exec(function in xcmp)
Register:xnl_proc_list exec in xnl_rx_process
*/
extern volatile  U16 session_addr;
static void xnl_data_msg_func(xnl_fragment_t * xnl)
{
	/*
	Try to schedule ACK.
	If cannot schedule ACK, just leave without processing message; 
	XNL will retry again, and hopefully our Tx resources will then be free. If 
	ACK has been scheduled. It most likely is already owned by the 
	transmitter, but possibly is waiting in Queue with immediate timeout.
	 */
	xnl_send_msg_ack(&xnl->xnl_header);
	if((xnl->xnl_payload.xnl_content_data_msg.xcmp_opcode &0x0FFF) == DATA_SESSION)
	{
		session_addr = xnl->xnl_header.source;
	}

	/*exec xcmp function*/
	xcmp_exec(xnl->xnl_payload.xnl_content_data_msg);//xcmp_rx =>> xcmp_rx_process
}

/**
Function: xnl_get_msg_ack_func
Description: process while receive msg ack.
Calls:xSemaphoreGive--freertos
Register:xnl_proc_list exec in xnl_rx_process
*/
static void xnl_get_msg_ack_func(xnl_fragment_t * xnl)
{
	U16 DestinationAddress;
	U16 TransactionID;
	
	DestinationAddress = xnl->xnl_header.destination;
	/*No timeout*/		
	 //One relationship is controlled by a state machine, only the current MSG received the ACK, 
	//then the program will clear the current MSG sending address and out of standby and will jump to the sending state.
	if (DestinationAddress == xnl_information.device_address )
	{
		//The ack is for me.	
		TransactionID = xnl->xnl_header.transaction_id;
		xSemaphoreGive(xnl_timeout_semphr);	
		    
	}
}

/**
Function: xnl_register_xcmp_func
Description: register the xcmp function(callback function)
*/
void xnl_register_xcmp_func( void ( *func)(xnl_content_data_msg_t))
{
	 xcmp_exec = func;
}

/*
Define a function list, and register the corresponding function is used to 
deal with XNL instructions
*/
static const volatile xnl_proc_list_t xnl_proc_list[20]={
	
	NULL,							/*-0x0-XNL_INVALID*/
	NULL,							/*-0x1-XNL_INVALID*/

	(void *)xnl_master_status_brdcst_func,	/*-0x2-XNL_MASTER_STATUS_BRDCST*/
	NULL,							/*-0x3-XNL_DEVICE_MASTER_QUERY*/
	NULL,							/*-0x4-XNL_DEVICE_AUTH_KEY_REQUEST*/
	(void *)xnl_device_auth_reply_func,		/*-0x5-XNL_DEVICE_AUTH_KEY_REPLY*/
	NULL,							/*-0x6-XNL_DEVICE_CONN_REQUEST*/
	(void *)xnl_device_conn_reply_func,		/*-0x7-XNL_DEVICE_CONN_REPLY*/
	NULL,							/*-0x8-XNL_DEVICE_SYSMAP_REQUEST*/
	(void *)xnl_device_sysmap_brdcst_func,/*-0x9-XNL_DEVICE_SYSMAP_BRDCST*/

	NULL,							/*-0xA-XNL_INVALID*/

	(void *)xnl_data_msg_func,				/*-0xB-XNL_DATA_MSG*/
	(void *)xnl_get_msg_ack_func			/*-0xC-XNL_DATA_MSG_ACK*/
};

/**
Function: xnl_tx
Parameters: xnl_fragment_t * 
Description: send xnl(resend when timeout)
Calls: xQueueSend--freertos  
Called By: ...
*/
void xnl_tx(xnl_fragment_t * xnl)
{
	/*variables are used to store the push result in interrupt*/
	//portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	/*
	Value ranging from 0-7. This value is incremented for each new data 
	message that is sent. The same value should be used for all retries.
	*/
	static U8 flags = 0;
	
	//if(DEFAULT_VALUE == xnl->xnl_header.flags)
	//{
	//xnl->xnl_header.flags =  0x0100 | ((++flags) & 0x07);
	//}
	if((xnl->xnl_header.protocol_id == XNL_PROTO_XCMP) 
		&&(xnl->xnl_header.opcode == XNL_DATA_MSG))//xcmp data
	{
		xnl->xnl_header.xnl_flags =  ((++flags) & 0x07);
	}
	else
	{
		//xnl,xcmp_ack: This is the same value contained in the XNL_DATA_MSG.
	}
	
	
	if(DEFAULT_VALUE == xnl->xnl_header.destination)
	{		
		/*Use actual Master address.*/
		xnl->xnl_header.destination = xnl_information.master_address;
	}
	
	if(DEFAULT_VALUE == xnl->xnl_header.source)
	{
		/*Use Temporary address.*/	
		xnl->xnl_header.source = xnl_information.device_address;
	}
	
	if(DEFAULT_VALUE == xnl->xnl_header.transaction_id)
	{		
		xnl->xnl_header.transaction_id = 
		  ((++xnl_information.transaction_id) & 0xFF) 
		| (xnl_information.transaction_id & 0xFF00);
	}
	
	/*count check sum */
	xnl->phy_header.check_sum = check_sum( xnl );
	
	xnl_fragment_t * ptr = get_xnl_idle();
	if(NULL != ptr)
	{
		memcpy(ptr, xnl, sizeof(xnl_fragment_t));		
		xQueueSend(xnl_frame_tx, &ptr, 0);//如果队列已满，则立即返回
	}
	else
	{
		log_debug("ptr - failure\n");
	}
}

/**
Function: xnl_tx_process
Parameters: void * 
Description: send xnl(resend when timeout)
Calls: xQueueReceive--freertos 
	xSemaphoreTake--freertos 
	phy_tx -- physical.c
Called By: task
*/
portTickType xnl_tx_water_value =0;
static void xnl_tx_process(void * pvParameters)
{
	/*To store the elements in the queue*/
//	static xnl_fragment_t xnl_frame;
	
	/*send status*/
	static xnl_tx_state_t xnl_tx_state = WAITING_FOR_TX;
	
	/*the times of timeout*/
	static U32 xnl_send_times = 0;
	
	//static xnl_fragment_t * xnl_ptr;
	static  xnl_fragment_t * ptr = NULL;//是否可以修缮为静态变量？请关注
	
	sync_ssi();	//同步SSC信号
	for(;;)
	{		
		switch(xnl_tx_state)
		{
			case WAITING_FOR_TX:
				if(pdTRUE == xQueueReceive( xnl_frame_tx, &ptr
					, portMAX_DELAY ))
				{				
					if(NULL == ptr)
					{
						break;
					}
					
					if(ptr->xnl_header.opcode == 0x0000)
					{
						/*invalid XNL opcode*/
						set_xnl_idle(ptr);
						break;
					}
					
					
					/*send physical data*/
					phy_tx((phy_fragment_t *)ptr);
					//log_debug("send xnl[%d]:0x%x \n\r", 
					//((ptr->xnl_header.protocol_id<<8)+ptr->xnl_header.xnl_flags), 
					//ptr->xnl_header.opcode);
					
					//if( ptr->xnl_header.opcode == XNL_DATA_MSG)
					//{
						//占用更多的的内存开销，谨慎调用
						//log_debug("T_xcmp:0x%x \n\r", ptr->xnl_payload.xnl_content_data_msg.xcmp_opcode);
					//}
					xnl_send_times = 1;				
					//if(ptr->xnl_header.opcode != XNL_DATA_MSG_ACK)
					{
						/*clear timeout semaphore and wait XNL reply*/
						xSemaphoreTake( xnl_timeout_semphr, ( portTickType )0);
						xnl_tx_state = WAITING_FOR_REPLY;
					}
						
					xnl_tx_water_value = uxTaskGetStackHighWaterMark(NULL);
				}
				break;
			
			/*wait XNL reply*/			
			case WAITING_FOR_REPLY://直到回答ACK才再发送下一条data/control
				if(pdPASS == xSemaphoreTake( xnl_timeout_semphr
					, ( portTickType )(500)/ portTICK_RATE_MS))//按ADK文档中提示500ms一次超时,但是实际情况下程序中启用freertos的任务延时不够精准。因此根据经验需要降低延时等待的时间
				{
					/*No timeout*/
					set_xnl_idle(ptr);			
					xnl_tx_state = WAITING_FOR_TX;
				}
				else
				{
					log_debug("send_xnl: timeout! \n\r");				
					/*time out*/
					if(xnl_send_times <= MIN_RESEND_TIMES)
					{
						/*If times smaller than A supermarket, then resend the 
						instructions*/
						phy_tx((phy_fragment_t *)ptr);
						xnl_send_times++;
					}
					else
					{
						//can not send data, disconnected						
						//vPortFree(ptr);	
						//log_debug("note:send xnl timeout! \n\r");
						set_xnl_idle(ptr);									
						xnl_tx_state = WAITING_FOR_TX;									
					}
				}
				xnl_tx_water_value = uxTaskGetStackHighWaterMark(NULL);
				break;
			default:
				break;
		}
	}
}

/**
Function: xnl_rx
Parameters: xnl_fragment_t * 
Description: Receive the XNL and perform the corresponding functions
Calls: 
Called By:xnl_rx_process
*/
static void xnl_rx(xnl_fragment_t * xnl)
{
	if(xnl->xnl_header.opcode > 0x0C)	
	{
		return;
	}

	if(NULL != xnl_proc_list[xnl->xnl_header.opcode].xnl_rx_exec)
	{
		//log_debug("R_xnl[%d]:0x%x \n\r", 
		//((xnl->xnl_header.protocol_id<<8)+xnl->xnl_header.xnl_flags),
		//xnl->xnl_header.opcode);//log:R_xnl指令
		/*execute the function in list*/
		xnl_proc_list[xnl->xnl_header.opcode].xnl_rx_exec(xnl);
	}
}

/**
Function: xnl_rx_process
Parameters: void * 
Description: Receive the XNL
Calls: 
Called By:task
*/
portTickType xnl_rx_water_value =0;
static void xnl_rx_process(void * pvParameters)
{
	/*To ptr the elements in the queue*/
	xnl_fragment_t * xnl_ptr;
	//static  portTickType water_value;
		
	for(;;)
	{
		if(pdTRUE ==xQueueReceive( phy_xnl_frame_rx, &xnl_ptr
		, portMAX_DELAY ))
		{				
			/*Receive the XNL and perform the corresponding functions*/			
			if(NULL != xnl_ptr)
			{
				xnl_rx(xnl_ptr);
				xnl_rx_water_value = uxTaskGetStackHighWaterMark(NULL);
				set_xnl_idle(xnl_ptr);//用完再处理							
			}			
		}
		
	}
}

/**
Function: xnl_init
Parameters: void 
Description: 
	intialize the physical layer;
	intialize the queue;
	Create the corresponding task;
	send device_master_query to connect radio
Calls: phy_init -- physical.c
	vSemaphoreCreateBinary--freertos
	xQueueCreate--freertos
	xTaskCreate--freertos
	xnl_send_device_master_query
Called By:xcmp_init -- xcmp.c
*/
void xnl_init(void)
{
	
	xnl_information.is_connected = FALSE;

	/*initialize the semaphore and queue*/
	vSemaphoreCreateBinary(xnl_timeout_semphr);		
	
	//作为一个中间变量，为什么分配那么大的空间？
	xnl_frame_tx = xQueueCreate(TX_XNL_QUEUE_DEEP, sizeof(xnl_fragment_t *)); //扩大xnl_frame_tx的队列深度
		
	xnl_store_idle = xQueueCreate(MAX_XNL_STORE, sizeof(phy_fragment_t *));
	for(int i= 0; i < MAX_XNL_STORE; i++ )
	{
		set_xnl_idle(&xnl_store[i]);//xnl_frame_tx，phy_xnl_frame_tx，phy_xnl_frame_rx这三个实际上都是依赖xnl_store_idle实体：二维数组
	}
	
	/*initialize the queue to send/receive xnl packet */
	phy_xnl_frame_tx = xQueueCreate(TX_XNL_QUEUE_DEEP, sizeof(phy_fragment_t *));
	phy_xnl_frame_rx = xQueueCreate(RX_XNL_QUEUE_DEEP, sizeof(phy_fragment_t *));
	
	
	/*if enable send/receive payload(media), defined in physical.h*/
	#if ENABLE == PAYLOAD_ENABLE
	payload_store_idle = xQueueCreate(MAX_PAYLOAD_STORE, sizeof(void *));
	for(int i= 0; i < MAX_PAYLOAD_STORE; i++ )
	{
		set_payload_idle(payload_store[i]);
	}
	
	/*initialize the queue to send/receive payload packet */
	//phy_payload_frame_tx =
	//xQueueCreate(TX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t));
	
	phy_payload_frame_rx = xQueueCreate(RX_PAYLOAD_QUEUE_DEEP, sizeof(phy_fragment_t *));
	
	#endif /*end if*/
		
	/*create task*/	
	/*this task is used to receive xnl message*/
	xTaskCreate(
	xnl_rx_process
	,  (const signed portCHAR *)"XNL_RX"
	,  800//512,220*4=880bytes
	,  NULL
	,  tskXNL_PRIORITY //+ 1
	,  NULL
	);
	
	/*this task is used to send xnl message*/
	xTaskCreate(
	xnl_tx_process
	,  (const signed portCHAR *)"XNL_TX"
	,  200//800,130*4=520bytes
	,  NULL
	,  tskXNL_PRIORITY//+1
	,  NULL
	);
	
	/*initialize the physical layer*/
	phy_init();
}









