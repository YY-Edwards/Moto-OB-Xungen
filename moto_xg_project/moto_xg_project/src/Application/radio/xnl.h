/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: xnl.h
Author: wxj
Version: 1.0.0.01
Date: 2016/3/15 14:03:54

Description:
History:
*/

#ifndef XNL_H_
#define XNL_H_

#include "compiler.h"

/*information of xnl */
typedef struct
{	
	/*xnl status*/
	U8 is_connected;
	
	/*XNL address*/
	U16 master_address;
	U16 device_address;
	
	/*The upper byte contains the device type. The lower byte contains the 
	device number (1, 2, 3etc).*/
	U16 logical_address;
	
	/*Transaction ID*/
	U16 transaction_id;
}xnl_information_t;

typedef struct {
	
	/*Data Type << 12 | Fragment Type << 8 | Length*/
	/*
	Date Type:
	There are five possible data types. These data types are described below. 
	Diagram 3 490 illustrates the definition of input connection and output 
	connection which are used in the 491 data type definition.
	Fragment Type:
	The fragment type has four possible values.
	Length:
	The length field is in the unit of byte.
	*/	
	U16            phy_control; 
	
	/*
	For transmission or reception of XCMPXNL data, checksum is required for 
	all fragments of XCMPXNL payload. Checksum field is used for ensuring the 
	correct XCMPXNL payload transmission. For all other data type, the 
	checksum field shall not be inserted in the fragments.
	
	The checksum is computed by summing all XCMPXNL payload words (16 bits) in 
	each fragment excluding the Checksum word , taking lower 16 bits of the 
	results and performing 2’s complement of that result. Each 16-bit payload 
	word is used as an unsigned integer in the checksum computation. The 
	checksum computation must include any stuffed bits.
	*/
	U16            check_sum;
} phy_header_t;

//Option Board ADK Development Guide [9.1.2.4].
//The 0xABCD Header is a constant, so need not be stored.
typedef struct {   //XCMP/XNL Development Guide Section 5.1
	U16            opcode;
	
	/*Protocol ID << 8 | XNL Flags*/
	/*
	Protocol ID:
	XCMP command message 
	XNL Flags:
	Value ranging from 0-7. This value is incremented for each new data message
	that is sent. The same value should be used for all retries.
	*/
	U16            flags;
	
	/*XNL address*/
	U16            destination;
	U16            source;
	
	/*Transaction ID*/
	U16            transaction_id;
	
	/*Number of bytes of payload*/
	U16            payload_length;
}xnl_header_t;

/**invalid XNL opcode*/
#define XNL_INVALID					 0x0000

/*MASTER_STATUS_BRDCST*/
/*
This message is sent out by the master device to indicate that the master has 
been determined and that non-master devices can now connect. The data payload 
for this message will contain the XNL version as well as the logical device 
identifier for the master device. The last field in the payload contains a 
flag that indicates whether or not an XNL_DATA_MSG has been sent out. This 
will indicate to a connecting device that it has missed messages. The XNL 
header will contain the master’s XNL address.
*/
#define XNL_MASTER_STATUS_BRDCST     0x0002

typedef struct 
{
	/*Minor XNL Protocol Version Number*/
	U16   minor_xnl_version_number;
	
	/*Major XNL Protocol Version Number*/
	U16   major_xnl_version_number;
	
	/*The upper byte contains the device type. The lower byte contains the 
	device number (1, 2, 3etc).*/
	U16   master_logical_identifier;
	
	/*Indicates to late connecting devices that XNL Data Message traffic has 
	already occurred.
	0x00 – No XNL Data Message traffic has occurred
	0x01 – XNL Data Message traffic has occurred*/
	U8    data_message_sent;
	
	/*Pad.*/
	U8    u8;                        
	U16   u16[3];
} xnl_content_master_status_brdcst_t;

/*DEVICE_MASTER_QUERY*/
/*
This message is sent by a non-master device in order to query the system to 
determine which device is master. The master replies with the 
MASTER_STATUS_BRDCST message no matter the XNL connection is established or 
not with this non-master device. This message allows for timing differences in 
devices. If the master takes longer than the device to power up, the device 
can continue to send out this query until the master is determined. In the 
same way, if a device is attached after the master has already been 
determined, it provides a way for the device to find out which device is 
master. This message does not contain any payload data. 

The non-slave device can use either the XNL address of the master device or 
0x0000 to send the DEVICE_MASTER_QUERY to the master device. However, for all 
other XNL control messages: DEVICE_AUTH_KEY_REQUEST, DEVICE_CONN_REQUEST, and 
DEVICE_SYSMAP_REQUEST, the non-slave device should use the master XNL address 
as the destination address. The master XNL address can be gotten from the 
source field of the MASTER_STATUS_BRDCST message.
*/
#define XNL_DEVICE_MASTER_QUERY      0x0003

typedef struct 
{
	/*This message contains no payload.*/
	U16   u16[7];
} xnl_content_device_mastaer_req_t;

/*DEVICE_AUTH_KEY_REQUEST*/
/*XCMP/XNL Development Guide Section 5.4.3*/
/*
This message is sent by all non-master devices in order to get the 
authentication key to be used when establishing a connection. This message 
contains no payload data.
*/
#define XNL_DEVICE_AUTH_KEY_REQUEST  0x0004

typedef struct 
{ 
	/*This message contains no payload.*/
	U16   u16[7]; 
} xnl_content_device_auth_key_req_t;

/*DEVICE_AUTH_KEY_REPLY*/
/*
The payload for this message is a temporary XNL address to use during the 
connection process and an unencrypted 8 byte random number generated by the 
master. This number should be encrypted by the receiving device and will be 
used to authenticate the connection request.
*/
#define XNL_DEVICE_AUTH_KEY_REPLY    0x0005

typedef struct 
{  
	/*Temporary XNL address to use until authentication is complete.*/			
	U16   temporary_xnl_address;
	
	/*Array of values to be encrypted*/
	U8    unencrypted_authentication_value[8];
	
	U16   u16[2];
} xnl_content_device_auth_key_reply_t;

/*DEVICE_CONN_REQUEST*/
/*
This message is sent by all non-master devices in order to establish a 
logical connection with the master. If a particular XNL address is desired 
(for fixed address systems), a preferred address field should be used. 
Otherwise a value of 0x0000 should be used. For systems that contain both 
fixed address and dynamic address assignments, the preferred address cannot be 
guaranteed. The payload for this message also includes a device type value, 
authentication index, and the encrypted authentication value
*/
#define XNL_DEVICE_CONN_REQUEST      0x0006

typedef struct 
{ 
	/*0x0000 if no address is requested, otherwise the desired XNL address.*/
	U16   preferred_xnl_address;
	
	/*Device Type identifier. See definition in Device Initialization Status 
	message in xcmp.h*/
	U8    device_type;
	
	/*Authentication index
	0x01 – IP Capable Peripheral,
		   Non-IP Capable Peripheral
	0x02 – Option Board*/
	U8    authentication_index;
	
	/*8 byte value encrypted*/
	U8    encrypted_authentication_value[8];
	
	U16   u16;
} xnl_content_device_conn_req_t;

/*DEVICE_CONN_REPLY*/
/*
The reply to the Device Connection Request contains a result code (connection
successful or not) and the XNL address and device logical address to use for 
all future communications. In addition, the reply will contain a unique 8-bit 
value that should be used as the upper byte of the transaction ID and an 
8-byte encrypted value that the device can use to authenticate the master.
*/
#define XNL_DEVICE_CONN_REPLY        0x0007

typedef struct 
{  
    /*
	(Result Code << 8) | Transaction ID Base;
	Result Code:Result of the connection request.XNL_FAILURE=0,XNL_SUCCESS=1
	Transaction ID Base:A unique identifier that is used as the upper byte of 
	the transaction ID for subsequent requests.
	*/    
	U16   result_base;
	
	/*Newly assigned address to be used in all subsequent messaging.*/
	U16   xnl_address;
	
	/*
	Logical address that has been assigned to the device. The logical address
	consists of the device type in the upper byte (i.e. byte 16) and the device
	ID in the lower byte (i.e. byte 17)
	*/
	U16   logical_address;
	
	/*8-byte encrypted value created using the Authentication Value contained 
	in the Device Connect Request message.*/
	U8    encrypted_authentication_value[8];
} xnl_content_device_conn_reply_t;

/*DEVICE_SYSMAP_REQUEST*/
/*
This message allows a device to request that the Master send out the 
XNL_DEVICE_SYSMAP_BRDCST message. This message does not contain any payload 
data.*/
#define XNL_DEVICE_SYSMAP_REQUEST    0x0008

typedef struct 
{ 
	/*This message contains no payload.*/
	U16   u16[7];
} xnl_content_device_sysmap_req_t;

/*DEVICE_SYSMAP_BRDCST*/
#define XNL_DEVICE_SYSMAP_BRDCST     0x0009
/*
The master device sends out this message to all XNL devices after each new 
connection is established. This message contains a list of the XNL addresses 
for all devices as well as a logical identifier for the device based on its 
device type (i.e. Control Head 2, Radio 1, etc.). If a device connects after 
the system map has already been broadcast (the device powers up late), the 
master device will resend the system map which will include all previously 
connected devices as well as the new device. If this message is sent out in 
response to a DEVICE_SYSMAP_REQUEST, it will only be sent to the requesting 
device.
*/

typedef struct 
{ 
	/*The number of elements (N) in the following SysMap array.*/
	U16   sizeof_sysmap_array;
	
	/*Packed bytes.*/
	U8    u8[10]; 
	
	U16   u16;
} xnl_content_device_sysmap_brdcst_t;

/*DATA_MSG*/
/*
The XNL_DATA_MSG opcode merely carries the raw data. The protocol field will 
indicate the type of raw data.*/
#define XNL_DATA_MSG                 0x000B

typedef struct 
{
	U16  xcmp_opcode;
	//U8   u8[200];
	U8   u8[238];
} xnl_content_data_msg_t;

/*DATA_MSG_ACK*/
/*
XNL_DATA_MSG_ACK contains no payload data. The opcode will indicate that the 
message was received correctly.
*/
#define XNL_DATA_MSG_ACK             0x000C

typedef struct 
{                 //XCMP/XNL Development Guide Section 5.4.10
	U16   u16[7];                    //This message contains no payload.
} xnl_content_data_msg_ack_t;

#define DEFAULT_VALUE	0xFFFF

/*payload*/
typedef union {
	xnl_content_master_status_brdcst_t		xnl_content_master_status_brdcst;
	xnl_content_device_mastaer_req_t		xnl_content_device_mastaer_req;
	xnl_content_device_auth_key_req_t		xnl_content_device_auth_key_req;
	xnl_content_device_auth_key_reply_t		xnl_content_device_auth_key_reply;
	xnl_content_device_conn_req_t			xnl_content_device_conn_req;
	xnl_content_device_conn_reply_t			xnl_content_device_conn_reply;
	xnl_content_device_sysmap_req_t			xnl_content_device_sysmap_req;
	xnl_content_device_sysmap_brdcst_t		xnl_content_device_sysmap_brdcst;
	xnl_content_data_msg_t					xnl_content_data_msg;
	xnl_content_data_msg_ack_t				xnl_content_data_msg_ack;
} xnl_payload_t;


//Fixed size 256 byte Rx fragment.
typedef struct {
	phy_header_t     phy_header;  // 2  Words
	xnl_header_t     xnl_header;  // 6  Words
	xnl_payload_t    xnl_payload; // 7  ,10hWords (most common messages),=>238bytes,at most
	//U16            rest[19];
} xnl_fragment_t;

/*f times smaller than A supermarket, then resend the instructions*/
#define MIN_RESEND_TIMES	3//通过超时判定重发太单一。。。

/*send/receive XNL packet status*/
typedef enum
{
	WAITING_FOR_TX,
	WAITING_FOR_REPLY,
}xnl_tx_state_t;

/*Define the type of XNL command handler*/
typedef struct 
{
	void (* xnl_rx_exec)(void *);
}xnl_proc_list_t;

/*
initialize the physical
initialize the queue
send device_master_query to connect radio
*/
void xnl_init(void);

/*register the xcmp function(callback function)*/
void xnl_register_xcmp_func( void ( *func)(xnl_content_data_msg_t));

void xnl_send_device_master_query(void);
/*send xnl(resend when timeout)*/
void xnl_tx(xnl_fragment_t * xnl);

#endif /*XNL_H_*/
