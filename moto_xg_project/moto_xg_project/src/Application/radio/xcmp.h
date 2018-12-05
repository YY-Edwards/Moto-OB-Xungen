/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: xcmp.h
Author: wxj
Version: 1.0.0.01
Date: 2016/3/29 9:48:40

Description:
History:
*/

#ifndef XCMP_H_
#define XCMP_H_

#include "xnl.h"

typedef xnl_content_data_msg_t xcmp_fragment_t;

#define MAX_APP_FUNC 0x006A
/*For the XNL Channel, the total data length includes the checksum bytes in
each fragment while for the Payload Channel the checksum bytes do not exist. Note
that the total data length does not include the payload padding byte for both XNL
Channel and Payload Channel*/
#define MAX_TRANSFER_UNIT 254
#define MAX_XCMP_DATA_LENGTH (MAX_TRANSFER_UNIT - sizeof(xnl_header_t) -2) //254-2-12=240,除去校验2bytes
#define MAX_CSBK_UNIT 22//单包xnl协议下最多只能实现22个csbk数据包的传输。

typedef struct
{
	/*request function*/
	void (* xcmp_rx_req)(void *);
	
	/*reply function*/
	void (* xcmp_rx_reply)(void *);
	
	/*board cast function*/
	void (* xcmp_rx_brdcst)(void *);
}app_exec_t;

/*The message types can now be rigorously defined.*/
/*
a message sent to a single endpoint with the expectation that the
recipient will generate a protocol-mandated reply
*/ 
#define XCMP_REQUEST		0

/*
a message sent to a single endpoint in response to a request initiated 
from that endpoint. 
*/
#define XCMP_REPLY			0x8000

/* 
a message sent to one or more endpoints; protocol-mandated  responses are 
disallowed.
*/
#define XCMP_BRDCAST		0xB000


/*
Reply messages contain a result code (in some cases, the reply message may 
consist solely of the opcode and the result code; in other cases additional 
fields will be present). The result code indicates the success or failure of 
the request being replied to, possibly with some indication of the reason if 
the result is failure. 
The standard result codes have the following fixed meanings:
*/

/*The command was completed successfully.*/
#define xcmp_Res_Success 0x00

/*The command failed (use only when no other result code applies).*/
#define xcmp_Res_Failure 0x01

/*The radio is not in the mode required (e.g. receive mode etc.) for the 
requested operation.*/
#define xcmp_Res_Incorrect_Mode 0x02

/*The radio does not support this opcode.*/
#define xcmp_Res_Opcode_Not_Supported 0x03

/*One or more of the message parameters are missing, out of range, or conflict
with other parameters.*/
#define xcmp_Res_Invalid_Parameter 0x04

/*The reply message was too big to be sent over the transport layer.*/
#define xcmp_Res_Reply_Too_Big 0x05

/*The security unlock mechanism was required and not invoked.*/
#define xcmp_Res_Security_Locked 0x06

/*The requested function is not supported or enabled.*/
#define xcmp_Res_Function_Not_Available 0x07

/*Feature capability enable is required via CPS.*/
#define xcmp_Res_Feature_Disabled 0x10

/*Radio is not in the idle state.*/
#define xcmp_Res_Radio_Busy 0x11

/*The Target address is invalid*/
#define xcmp_Res_Invalid_Target_Address 0x12

/*Resource is unavailable or channel is busy*/
#define xcmp_Res_System_Busy 0x13

/*
RADIO_SATUS
This message is used to interrogate the status of several conditions of the 
radio.
*/
#define RADIO_STATUS			0x00E



/*
CSBK_SATUS
This message is used to interrogate the status of data session of the
radio.
*/

#define CSBK_DATA_RX_Suc			0x24

#define DATA_SESSION_TX_Suc			0x03
#define DATA_SESSION_TX_Fail		0x04



/*
Condition
This field is optional. When result is Invalid Parameter, the reply message does
not use this field.
*/

/*Read RSSI*/
#define RSSI (unsigned char)0x02

/*Read Model Number*/
#define Product_Model_Number (unsigned char)0x07 

/*Read Serial Number*/
#define Product_Serial_Number (unsigned char)0x08 

/*Read Electronic Serial Number (ESN)*/
#define Product_ESN (unsigned char)0x09

/*Read Current Signaling*/
#define Signaling (unsigned char)0x0D

/*Read Radio ID*/
#define Radio_ID (unsigned char)0x0E

/*Select 5 Radio ID*/
#define Select_5_Radio_ID (unsigned char)0x16 

/*Bluetooth hardware chip status*/
#define Bluetooth_Chip_Status (unsigned char)0x1B

/*Read IME Status*/
#define IME_Status (unsigned char)0x1C

/*Read ARTS Status*/
#define ARTS_Status (unsigned char)0x1F

/*Bluetooth Active Connection*/
#define Bluetooth_Active_Connection (unsigned char)0x23

/*Bluetooth Status (On/Off)*/
#define Bluetooth_Status (unsigned char)0x24

typedef struct
{
	unsigned char  Condition;
}RadioStatus_req_t;

#define MAX_RADIO_STATUS_VALUE_SIZE 160
typedef struct
{
	unsigned char  Result;
	unsigned char  Condition;
	
	/*This field is optional. When result is Invalid Parameter, the reply 
	message does not use this field.*/
	unsigned char  StatusValue[MAX_RADIO_STATUS_VALUE_SIZE];
}RadioStatus_reply_t;

/*
Version Information
This message is used to read version information from the device.
*/
#define VERSION_INFORMATION 0x00F 

/*Type*/
/*Software version of the host device (radio) firmware.*/
#define Host_Software 0x00 

/*DSP software version*/
#define DSP_SW 0x10

/*Radio Regional information The reply will be 1 byte value.*/
#define Regional_Information 0x47 

/*Radio frequency band of the radio device.*/
#define RF_Band 0x63 

/*Transmit output power of the radio device (mobile only).*/
#define Power_Level 0x65

/*Flash Size of the radio*/
#define Flash_Size 0x6D

/*Control head sw version*/
#define Control_Head_1_Application 0x70 

/*Version number of the option board firmware.*/
#define Option_Board_Firmware_Version 0x81

/*Version number of the option board hardware.*/
#define Option_Board_Hardware_Version 0x80 //

typedef struct
{
	unsigned char  Type;
}VersionInfromation_req_t;

#define MAX_VERSION_INFO_VALUE_SIZE 160
typedef struct
{
	unsigned char  Result;
	unsigned char  Value[MAX_VERSION_INFO_VALUE_SIZE];
}VersionInfromation_reply_t;

/*
Language Pack Information
This message is used to query the language pack information stored in the radio.
The broadcast message type is only supported for function Query Default 
Language 790 Pack↑.
*/
#define LANGUAGE_PACK_INFORMATION 0x02C

/*
Function
*/
/*This operation is used to query the language pack information on a specific 
language pack. The Language Pack ID field is required for this operation.
*/
#define Query_Single_Language_Pack_Information 0x00

/*This operation is used to query information on all language packs currently 
supported in the radio.
*/
#define Query_All_Language_Pack_Information 0x01

/*This operation is used to query the default language pack information stored 
in the Radio. The Language Pack ID field is not required for this operation. 
In reply, available space shall have value 0.
*/
#define Query_Default_Language_Pack_Information 0x02

typedef struct
{
	unsigned char  Function;
	
	/*
	This is a fixed size 16-byte UCS-2 string. The string shall be null 
	terminated and any extra bytes shall also be padded with the null 
	character. 
	In general, the format of the string includes 2 characters for the 
	language (4 bytes) followed by a ?hyphenˉ (2 bytes) followed by 2 
	characters identifying the country (4 bytes). Space for 2 extra characters 
	has been provided for further uniqueness to be 813 provided if necessary.
	*/
	unsigned char  Language_Pack_ID[16];
}Language_Pack_Information_req_t;

#define MAX_LANG_PACK_DEF_NAME_SIZE  160 // >30

typedef struct 
{
	unsigned char Structure_Size;
	unsigned char Language_Pack_ID[16];
	unsigned char Major_version[4];
	unsigned char Minor_Version[4];
	unsigned char Size[4];
	unsigned char Name[MAX_LANG_PACK_DEF_NAME_SIZE];
}Language_Pack_Definition_t; 

typedef struct
{
	unsigned char  Result;
	
	/*This field indicates the available free space remaining in Bytes on the 
	Radio for adding Language Packs.*/
	unsigned char  Available_Space[4];
	
	/*This field indicates the number of Language Pack Definition structures*/
	unsigned char  Number_of_Definitions;
	Language_Pack_Definition_t  Language_Pack_Definition;
}Language_Pack_Information_reply_t;

typedef struct
{
	unsigned char  Function;
	/*This field indicates the number of Language Pack Definition structures*/
	unsigned char  Number_of_Definitions;
	Language_Pack_Definition_t  Language_Pack_Definition;
}Language_Pack_Information_brdcst_t;


/*
Automatic Frequency Correction Control

Automatic Frequency Correction (AFC) is a method the radio uses to adjust its 
reference oscillator to align its receive frequency with the signal being 
received. By default, AFC is enabled in the radio. This message is used to 
toggle this feature on a per personality basis until channel change or 
power-on/reset.

AFC may also be disabled on a per-personality basis via Clone Write. The radio 
will enable or disable AFC based on the AFC control mechanism that is last 
executed (i.e. AFXCON or CLONEWR with Channel Change). 

This opcode is supported in Analog RF Mode operation only.
*/
#define AUTOMATIC_FREQUENCY_CORRECTION_CONTROL 0x01C

/*
Function
*/

/*Enables AFC*/
#define AFC_On 0x00

/*Disables AFC*/
#define AFC_Off 0x01

typedef struct
{
	unsigned char  Function;
}Automatic_Frequency_Correction_Control_req_t;

typedef struct
{
	unsigned char  Result;
	unsigned char  Function;
}Automatic_Frequency_Correction_Control_reply_t;

/*
Clone Write

This message is used by the master radio to insert cloning specific 
information into the slave radio during the cloning operation. It is also used 
by attached devices to write codeplug data into the radio. This message allows 
one or more fields to be transferred simultaneously (up to the maximum packet 
size allowed by the transport layer).

The CLONEWR command is subject to the following restrictions:
	Radio must be in Device Control Mode for all XCMP Versions except V2.0.0.4; 
	The device sending the CLONEWR command must have its authentication level 
	set to 2 for all XCMP Versions except V2.0.0.4. XCMP V2.0.0.4 must have 
	authentication level set to 1; 
	CLONEWR can only be used the two virtual personalities (i.e. Zone Number: 
	0xFFFF, Channel Number: 0x01 / 0x02);
	
All Field IDs in the Clone Write message which are recognized by the radio and 
have valid values will be committed to the Virtual Personality of the radio by 
the time that the Clone Write Reply message is sent. Out of range values or 
unrecognized fields will not be committed to the Virtual Personality of the 
radio.

NOTE: Capacity Plus System Channel does not support CLONEWR. Sending a clone 
write request in this channel will cause unexpected behavior.
*/
#define CLONE_WRITE 0x109

/*
Clone Read

This message is used by attached devices to obtain codeplug data from the 
repeater. This message allows one or more fields to be transferred 
simultaneously (up to the maximum packet size allowed by the transport layer). 

The CLONERD command is subject to the following restrictions:  
	Radio must be in Device Control Mode to read a virtual personality for all 
	XCMP Versions except V2.0.0.4;
	The device sending the CLONERD command must have its authentication level 
	set to 2 for all XCMP Versions except V2.0.0.4. For XCMP V2.0.0.4 must 
	haveauthentication level set to 1.
*/
#define CLONE_READ 0x10A

/*
Device Initialization Status

This message is used to announce that a device is present, as well as to 
provide information about the basic capabilities of that device. This message 
is also sent by the  master to indicate that device initialization is 
complete. This is an indication that all devices may begin their normal XCMP 
messaging. 

When a non-master device powers up, it must first wait for the master to send 
its Device Initialization Status broadcast before sending its own 
initialization message. However, the non-master device must send this message 
within 1 second of the masterˉs device initialization broadcast. Once power up 
is completed, the master will send out a Device Initialization Complete 
message. 

If a single physical device supports more than one device type, the device 
must send a separate power-up status message for each supported type.

XCMP does not limit the implementation of how to handle the message sequence, 
which is specific to the product and shall be defined by products. Here is a 
recommended solution:

After the master powers up, it broadcasts its Device Initialization Status 
message. Upon receiving the Device Initialization Status from the master, all 
devices broadcast their Device Initialization Status messages. There may be 
situations where a device powers up late or the master receives a device 
initialization after master sends out the Device Initialization Complete 
message. If the master detects that a new device powered on or it receives a 
Device Initialization Status message from a device after sending out Device 
Initialization Complete message then the Master sends out a Device 
Initialization Status message again to trigger all devices including the new 
device to resynchronize their status. Resending this message does not imply 
that devices should disconnect and reconnect.

Sending this message can cause the system configuration to change. The 463 
implementation of how to handle the device init message sequence is specific 
to the 464 product and shall be defined by products.
*/
#define DEVICE_INITIALIZATION_STATUS 0x400

/*
Device Init Type
*/
/*This is the message that the device sends at power up or after a reset. It 
will give the initial status and capabilities for the device.
*/
#define Device_Init_Status		0x00

/*This is the message that is sent when the Master has determined that power 
up is complete. Only the Master device should send this message.
*/
#define Device_Init_Complete	0x01

/*This message is sent when the device has an update to either its status or 
to its device descriptor. This can be used for dynamically attached hardware 
(e.g. keypad). The complete device descriptor array should be sent. This is 
supported for MOTOTRBO Enhanced Portfolio & SL Series subscribers
*/
#define Device_Status_Update	0x02

/*
Device Type
*/
/*Provides unknown capabilities*/
#define Unknown_Device		0x00

/*Provides RF capabilities*/
#define RF_Transceiver		0x01

/*Provides user inputs and/or outputs (display).*/
#define Control_Head		0x02

/*Provides Siren and/or Public Address capabilities.*/
#define Siren_PA			0x03

/*Provides RF capabilities in a repeater configuration.*/
#define VRS					0x04 

/*Provides system console interconnection.*/
#define TRC_Consolette		0x05

/*Provides RF and audio amplification.*/
#define Vehicular_Adapter	0x06

/*3rd party Option Board-based application.*/
#define Option_Board		0x07

/*Provides external microphone capabilities.*/
#define External_Mic		0x09

/*3rd party IP Peripheral-based application.*/
#define IP_Peripheral		0x0A

/*3rd party non-IP Peripheral-based application.*/
#define Non_IP_Peripheral	0x0B

/*
Device Status
*/
/*Device has powered up with no errors.*/
#define Power_Up_Success	0x0000

/*A fatal error has occurred in the device.*/
#define Fatal_Error			0x0080

#define MAX_DEVICE_DESC_SIZE 160
#pragma  pack(1)

/*
This is needed for Device Init Status and Device Status Update. It specifies 
the Status and Type for the device sending out its Initialization Status.
*/
typedef struct
{
	unsigned char  DeviceType;
	unsigned char  DeviceStatus[2];
	unsigned char  DeviceDescriptorSize;
	unsigned char  DeviceDescriptor[MAX_DEVICE_DESC_SIZE];
}DeviceStatusInfo_t;
#pragma  pack()

typedef struct
{
	unsigned char XCMPVersion[4];
	unsigned char DeviceInitType;
	DeviceStatusInfo_t DeviceStatusInfo;
}DeviceInitializationStatus_brdcst_t;


/*
Tone Control

This message is used to control logical tones. A logical tone is a predefined 
tone, defined by the handler of the request. A unique tone identifier will be 
associated with each tone. For logical tones, the same logical tone identifier 
is returned.

Momentary tones have higher priority than both continuous tones and received 
audio. Received audio has higher priority than continuous tones. It is 
possible that a logical continuous tone may get converted into a momentary 
tone if it is requested during received audio.

Momentary tones may be played while the radio is transmitting if the 
microphone is muted during the playing of the tone so that it is not 
transmitted with the transmitted audio. It is up to the tone-handling device 
to make this determination. Continuous tones will be suspended or queued 
during voice transmit mode.

Voice tones and signaling tones are not supported by this opcode.

Due to the finite and short duration, of a momentary tone, a Stopped state 
change may not be broadcast. This means that the device may only broadcast out 
the starting of a  momentary tone. 

The broadcast message type is disabled by default. A device requiring this 
message must send a Device Initialization Status with a Device Speaker Type 
attribute of XCMP_DEVICE_SPKR_PWR. Otherwise, the radio will not send out any 
broadcast message. See section 4.3 for more information. The broadcast message 
is delivered to all XCMP devices when it is enabled by any one of the XCMP 
devices.
*/
#define TONE_CONTROL 0x409

/*
Function
*/
/*Stops the specified tone.*/
#define Tone_Stop	0x00

/*Starts the specific tone.*/
#define Tone_Start	0x01

/*Disables the tone specified. This prevents the tone from being generated.*/
#define Disable		0x02

/*Enables the tone specified. This permits the tone to be generated.*/
#define Enable		0x03

/*
Tone Identifier
*/
#define Tone_Reserved 0x0000
#define Talk_Permit 0x0003
#define Good_Key_Chirp 0x0005
#define Bad_Key_Chirp 0x0006
#define Priority_Beep 0x000C
#define Power_Up_Beep 0X000D
#define No_ACK_Tone 0X0012
#define ACK_Received_Tone 0x0014
#define Dispatch_Busy 0x0023
#define Talk_Prohibit 0x0024
#define Accessory_Connected 0x0037
#define Accessory_Disconnected 0x0038
#define Negative_Indication_Tone 0x0039
#define First_Menu_Item_Tone 0x003A
#define Text_Message_Arrived_Tone 0x003B
#define Power_Up_Fail_Beep 0x003C
#define Ring_Style_Tone_1 0x003D
#define Ring_Style_Tone_2 0x003E
#define Ring_Style_Tone_3 0x003F
#define Ring_Style_Tone_4 0x0040
#define Ring_Style_Tone_5 0x0041
#define Ring_Style_Tone_6 0x0042
#define Ring_Style_Tone_7 0x0043
#define Ring_Style_Tone_8 0x0044
#define Ring_Style_Tone_9 0x0045
#define Ring_Style_Tone_10 0x0046
#define Volume_Feedback_Tone 0x0047
#define Test_Tone_1 0x0048
#define Test_Tone_2 0x0049
#define Test_Tone_3 0x004A
#define Test_Tone_4 0x004B
#define Low_Battery_1 0x004D
#define Low_Battery_2 0x004E
#define Low_Battery_3 0x004F
#define Emergency_Receive_Tone 0x0050
#define Repeating_Ring_Style_Tone_1 0x0052
#define Repeating_Ring_Style_Tone_2 0x0053
#define Repeating_Ring_Style_Tone_3 0x0054
#define Repeating_Ring_Style_Tone_4 0x0055
#define Repeating_Ring_Style_Tone_5 0x0056
#define Repeating_Ring_Style_Tone_6 0x0057
#define Repeating_Ring_Style_Tone_7 0x0058
#define Repeating_Ring_Style_Tone_8 0x0059
#define Repeating_Ring_Style_Tone_9 0x005A
#define Repeating_Ring_Style_Tone_10 0x005B

#define Emergency_Enter 0x005C
#define DTMF_Tone_0 0x005D
#define DTMF_Tone_1 0x005E
#define DTMF_Tone_2 0x005F
#define DTMF_Tone_3 0x0060
#define DTMF_Tone_4 0x0061
#define DTMF_Tone_5 0x0062
#define DTMF_Tone_6 0x0063
#define DTMF_Tone_7 0x0064
#define DTMF_Tone_8 0x0065
#define DTMF_Tone_9 0x0066
#define DTMF_Tone_Asterisk 0x0067
#define DTMF_Tone_Pound 0x0068

#define Privacy_Receive_Talk_Permit_Tone 0x006A
#define Group_Call_Talk Permit_Tone 0x006B
#define Private_Call_Receive_Tone 0x006C

#define Lone_Worker_Reminder_Tone 0x006D
#define Radio_Locked_Tone 0x006E
#define Input_Password_Tone 0x006F
#define Positive_Indicate_Tone 0x0070
#define Call_Ended_Alert_Tone 0x0071
#define TestMode_Tone 0x0072
#define Channel_Uninhibited_Tone 0x0076
#define Tone_Sidetone 0x001F
#define Ring_feedback_Tone 0x004C
#define BT_Connection_Success_Tone 0x0077
#define BT_Disconnecting_Success_Tone 0x0078
#define Speaker_Toggle_Tone 0x0079
#define QCII_Tone 0x0075
#define QCII_Individual_Call_Tone 0x007F
#define QCII_Group_Call_Tone 0x0080
#define Sig_Sidetone 0x0031
#define Vibrate_Style_1 0x0081
#define Vibrate_Style_Rep_1 0x008B

#define Ring_Style_with_Vibrate_1 0x0095
#define Ring_Style_with_Vibrate_2 0x0096
#define Ring_Style_with_Vibrate_3 0x0097
#define Ring_Style_with_Vibrate_4 0x0098
#define Ring_Style_with_Vibrate_5 0x0099
#define Ring_Style_with_Vibrate_6 0x009A
#define Ring_Style_with_Vibrate_7 0x009B
#define Ring_Style_with_Vibrate_8 0x009C
#define Ring_Style_with_Vibrate_9 0x009D
#define Ring_Style_with_Vibrate_10 0x009E

#define Repeating_Ring_Style_with_Vibrate_1 0x009F
#define Repeating_Ring_Style_with_Vibrate_2 0x00A0
#define Repeating_Ring_Style_with_Vibrate_3 0x00A1
#define Repeating_Ring_Style_with_Vibrate_4 0x00A2
#define Repeating_Ring_Style_with_Vibrate_5 0x00A3
#define Repeating_Ring_Style_with_Vibrate_6 0x00A4
#define Repeating_Ring_Style_with_Vibrate_7 0x00A5
#define Repeating_Ring_Style_with_Vibrate_8 0x00A6
#define Repeating_Ring_Style_with_Vibrate_9 0x00A7
#define Repeating_Ring_Style_with_Vibrate_10 0x00A8

#define Job_Tickets_Ring_Style 0x00A9
#define Job_Tickets_Ring_Style_Rep 0x00AA
#define Job_Tickets_Vibrate_Style 0x00AB
#define Job_Tickets_Vibrate_Style_Rep 0x00AC
#define Combined_Vol_Channel_Knob 0x00AD
#define ARTS_IN_RANGE_TONE 0x00AE
#define ARTS_OUT_RANGE_TONE 0x00AF
#define OTAP_RESTART_ATTENTION_TONE 0x00B2
#define OTAP_CALL_WAIT_TONE 0x00B3
#define MANDOWN_ALARM_TONE 0x00B5
#define MANDOWN_DISABLE_TONE 0x00B7
#define MANDOWN_FAILURE_TONE 0x00B8
#define All_Tones 0xFFFF

/*
Tone Volume Control
*/
/*The alert tone is played according to any rules for alert tones, given the 
current volume setting. Settings for this field are 8 to 255 (0x08 - 0xFF).*/
#define Current_Volume 0x00

/*The alert tone is played at the current volume setting, but all alert tone 
volume rules are ignored (minimum alert tone volume, alert tone offset, etc). 
NOTE: Supported for TONECTRLBRDCST only.*/
#define Voice_Volume 0x01

/*The alert tone is played at the maximum volume that the radio supports. 
NOTE: Supported for TONECTRLBRDCST only.*/
#define Max_Volume 0x02

/*The alert tone is played starting at a low volume and gradually increasing 
volume until Max volume is reached. The volume will increase linearly to the 
max volume. Applies only to continuous tones. NOTE: Supported for 
TONECTRLBRDCST only.*/
#define Escalate_Volume 0x03 

typedef struct
{
	unsigned char Function;
	unsigned char ToneIdentifier[2];
	unsigned char ToneVolumeControl;
	unsigned char Reserved[8];
}ToneControl_req_t;

typedef struct
{
	unsigned char Result;
	unsigned char ToneIdentifier[2];
	unsigned char Duration[2];
}ToneControl_reply_t;

typedef struct
{
	unsigned char Function;
	unsigned char ToneIdentifier[2];
	unsigned char ToneVolumeControl;
	unsigned char Reserved[8];
}ToneControl_brdcst_t;


/*
Volume Control
This message is used to request information about a device's volume control state, or to
request a change in volume control state. Because the actual device used as a volume
attenuator can be different in various devices, XCMP assumes an idealized volume
attenuator which provides 256 steps, with each step being an equal increment on an audio or logarithmic response.
In all cases, this request will result in a Volume Control Reply giving the status of the volume control.
In addition, if the volume attenuator setting is changed, a Volume Control State broadcast will also be sent.
The Volume Data for all functions is specified in terms of the 0-255 range of the 256
step attenuator. The value of 255 is interpreted as maximum volume (minimum attenuation), and 0 is interpreted as minimum volume (maximum attenuation). 
Note that a value of 0 may still result in some audio, the Speaker Control message should be used if it is desired to mute the speaker.


*/
#define VOLUME_CONTROL 0x406

//Function
#define   Enable_IntelligentAudio  0x10
#define   Disable_IntelligentAudio  0x11

//Attenuator Number

#define All_Speakers 0xFFFF //Selects all speakers
#define Primary_Speakers 0x0001
#define Secondary_Speakers 0x0002


//Volume Data




//Audio Parameter

typedef struct
{
	unsigned char Attenuator_Number[2];
	unsigned char Function;
	unsigned char Volume_Data;
	
}VolumeControl_req_t;

typedef struct
{
	unsigned char Result;
	unsigned char Attenuator_Number[2];
	unsigned char Function;
	unsigned char Volume_Data;
	
}VolumeControl_reply_t;

typedef struct
{
	unsigned char Attenuator_Number[2];

	unsigned char Volume_Data;
	unsigned char Audio_Parameter;
	
}VolumeControl_brdcst_t;

/*
 Enhanced Option Board Mode
This message is sent to request enter or exit Enhanced Option Board Mode.

*/
#define EN_OB_CONTROL 0x465

//Function
#define EN_OB_Enter 0x01
#define EN_OB_Exit  0x00

typedef struct
{
	
	unsigned char Function;
	
}En_OB_Control_req_t;

typedef struct
{
	unsigned char Result;

	unsigned char Function;

	
}En_OB_Control_reply_t;

typedef struct
{
	unsigned char Function;
	
}En_OB_Control_brdcst_t;



/*

 Shut Down
 
The request message type is used by one device to request a second device to shut down (or reset).
This reply message type is used by the second device to acknowledge whether the
request is accepted or denied. The second device should continue to process other
messages normally (or as best it can) up to the time it sends the shutdown broadcast.
The broadcast message type is used by the second device to indicate it is acting on the
request. This message must be sent when the device is actually about to shutdown.

*/


//Function
#define Shut_Down_Device 0x01
#define Reset_Device     0x02



/*

 Button Configuration
This message is used to report the characteristic or to change the logical functionality of
the radio’s programmable buttons. Changing the logical function of a button is known as
button re-task.

*/


#define BTN_CONFIG_REQUEST   0x412

//Function
#define Get_Info					0x00
#define Radio_Wide_Retask			0x01
#define Current_channel_Retask		0x02
#define Button_Reset				0x03
#define Button_Reset_All            0x04

//NumOfButtons
//ButtonInfoStructSize
#define Button_Info_StructSize     0x0A


//ButtonInfo

typedef struct
{
	U8 ButtonIdentifier[2];
	U8 ShortPressFeature[2];
	U8 Reserved1[2];
	U8 LongPressFeature[2];
	U8 Reserved2[2];
	
}ButtonInfo_t;



typedef struct
{
	U8 Function;
	U8 NumOfButtons;
	U8 ButtoInfoStructSize;
	ButtonInfo_t ButtonInfo[1];//*ButtonInfo;//ButtonInfo[1];
	
}ButtonConfig_req_t;


typedef struct
{
	U8 Result;
	U8 Function;

	
}ButtonConfig_reply_t;

typedef struct
{
	U8 Function;
	U8 NumOfButtons;
	U8 ButtoInfoStructSize;
	ButtonInfo_t ButtonInfo[19];
	
	
}ButtonConfig_brdcst_t;




//remote addr type
/*Identifies the local radio, master or a device in the XCMP network as the intended receiver.
 In this case the Remote Address Size will be 0 and no Remote Address field exists.*/
#define Rmt_Addr_Local_Device 0x00  
#define Rmt_Addr_Local_Device_Size 0x00 

/*Identifies the radio by its subscriber ID.*/
#define Rmt_Addr_MOTOTRBO_ID 0x01 
#define Rmt_Addr_MOTOTRBO_ID_Size 0x03

/*Identifies the radio by its IP Address on IPv4 type.*/
#define Rmt_Addr_IPv4_Address 0x02
#define Rmt_Addr_IPv4_Address_Size 0x04
 
/*Identifies the radio by its MDC ID.*/
#define Rmt_Addr_MDC_ID 0x05 
#define Rmt_Addr_MDC_ID_Size 0x02

/*Identifies the phone number(0-9, * ,# and P) of the call.*/ 
#define Rmt_Addr_Phone_Number 0x07

/*Identifies the radio by its QuikCall II (QCII) Tone Frequency*/
#define Rmt_Addr_Quik_Call_II 0x0B  
#define Rmt_Addr_Quik_Call_II_Size 0x04 

/*Identifies the radio by its Select 5 address*/
#define Rmt_Addr_Select_5_Address 0x0D  
#define Rmt_Addr_Select_5_Address_Size 0x09 
/*Identifies the access code/deaccess code which is requested during phone call operation*/
#define Rmt_Addr_Access_Deaccess_Code 0x0E

#define MAX_DATA_PAYLOAD_SIZE 1472

#pragma pack(1)
typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Address[14];
}RemoteAddress_Generic_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned short Port;
}RemoteAddress_LocalDevice_t;

typedef struct
{
	unsigned char Type;
	unsigned int Size : 8;
	unsigned int Address : 24;
	unsigned short Port;
}RemoteAddress_MOTOTRBOID_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Address[4];
	unsigned short Port;
}RemoteAddress_IPv4Address_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Address[2];
	unsigned short Port;
}RemoteAddress_MDCID_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Reserved[5];
	unsigned short Port;
}RemoteAddress_PhoneNumber_t;


typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Address[4];
	unsigned short Port;
}RemoteAddress_QuikCallII_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Address[9];
	unsigned short Port;
}RemoteAddress_Select5Address_t;

typedef struct
{
	unsigned char Type;
	unsigned char Size;
	unsigned char Reserved[8];
	unsigned short Port;
}RemoteAddress_AccessDeaccessCode_t;

typedef union{
	RemoteAddress_Generic_t Generic;
	RemoteAddress_LocalDevice_t LocalDevice;
	RemoteAddress_MOTOTRBOID_t MOTOTRBOID;
	RemoteAddress_IPv4Address_t IPv4Address;
	RemoteAddress_MDCID_t MDCID;
	RemoteAddress_PhoneNumber_t PhoneNumber;
	RemoteAddress_QuikCallII_t QuikCallII;
	RemoteAddress_Select5Address_t Select5Address;
	RemoteAddress_AccessDeaccessCode_t AccessDeaccessCode;
}RemoteAddress_t;

/*

 Data Session
This message is used to send a generic data message between two devices. 
XCMP does not decipher the data sent in the message. The applications on the devices using
this message are responsible for understanding the protocol used in the data. 
The message can be used to just send out data packets from one device to the other. 
The message can also be used to establish a session before initiating data transfer. 
An XCMP device can initiate multiple data sessions at one time.

*/

#define DATA_SESSION   0x41D

//Function
#define  Single_Data_Uint   (unsigned char)0x01	//Send a single data uint.
#define  Priority_Data_Uint   (unsigned char)0x02	//Send a single data uint.


#define  Data_Session_Start    (unsigned char)0x10	//Start a session of regular XCMP commands.
#define  Data_Session_End    (unsigned char)0x11	//End a session of regular XCMP commands.

//Data Protocol & Version

#define  LRRP_Ver_1_1         (unsigned char)0x00   //Location Request & Response Protocol.
#define  TMP_Ver_1_1         (unsigned char)0x20	//Text Messaging Protocol.
#define  Raw_Data         (unsigned char)0x50		//User-Defined Raw Data.
#define  CSBK_Raw_Data         (unsigned char)0x54		//User-Defined Raw Data in Broadcast CSBK.
#define  DMR_CSBK_Data         (unsigned char)0x70		//DMR Control Signal Block Data.
#define  DMR_Message_Data        (unsigned char)0x80		//Digital Mobile Radio Message Data.

//Destination Address

#define  Remote_Local_Address        (unsigned char)0x00  //Identifies the local radio, master or a device in the XCMP network as the intended receiver. 
														//In this case the Remote Address Size will be 0 and no Remote Address field exists.
#define  Remote_Local_Address_Size        (unsigned char)0x00	


#define  Remote_Mototrbo_Address        (unsigned char)0x01//Identifies the radio by its subscriber ID.
#define  Remote_Mototrbo_Address_Size        (unsigned char)0x03


#define  Remote_IPV4_Address        (unsigned char)0x02	//Identifies the radio by its IP Address on IPv4 type.
#define  Remote_IPV4_Address_Size        (unsigned char)0x04

//#define  Remote_Address    (U32)0x000002

#define  Remote_Port     (U16)0x0FA7//4007

//Data  payload
//Session ID

#define Session_ID  (unsigned char)0x00
#define DMR_Message_Type_Voice_header    (U16)0x0001
#define DMR_Message_Type_Terminator    (U16)0x0002
#define DMR_Message_Type_CSBK    (U16)0x0003
#define DMR_Message_Type_Data_header    (U16)0x0006
#define DMR_Message_Type_IDLE   (U16)0x0009


//#define MAX_ADDRESS_SIZE		10
//volatile U8  MAX_ADDRESS_SIZE = 0;

#define CSBK_PF_TRUE		0x1
#define CSBK_PF_FALSE		0x0
#define CSBK_LB_TRUE		0x1
#define CSBK_LB_FALSE		0x0
#define CSBK_Host_Opcode	0x3d
#define CSBK_Slave_Opcode	0x2b
#define CSBK_Third_PARTY	0x20
#define CSBK_Payload_Length	0x8


typedef struct
{
	U8 csbk_LB		:1;
	U8 csbk_PF		:1;
	U8 csbk_opcode	:6;
	
}CSBK_Header_t;


typedef struct
{
	CSBK_Header_t csbk_header;
	U8 csbk_manufacturing_id;
	U8 csbk_data[8];
	
}CSBK_Pro_t;

typedef struct
{
	U8 Remote_Address_Type;
	U8 Remote_Address_Size;
	U8 Remote_Address[4];
	U8 Remote_Port_Com[2];   
}Dest_Address_t;//8bytes


typedef struct
{
	U8 Data_Protocol_Version;
	Dest_Address_t Dest_Address;
}DataDefinition_t;//9bytes

typedef struct
{
	U8 Session_ID_Number;
	U8 DataPayload_Length[2];
	U8 DataPayload[1024];//若是单包，并且指针是指向xcmp.u8，那么最大为238bytes，也即是payload最大不能超过225bytes，否则会出现越界的bug
}DataPayload_t;//1027bytes

#pragma pack(1)
typedef struct
{
	U8 Function;
	DataDefinition_t DataDefinition;
	DataPayload_t DataPayload;
}DataSession_req_t;//1037bytes

typedef struct
{
	U16 xcmp_opcode;
	DataSession_req_t DataSession_req;
	
}xcmp_datasession_req_t;//1039bytes

#pragma pack()

typedef struct
{
	U8 Result;
	U8 Function;
	U8 Session_ID_Number;
	
}DataSession_reply_t;

typedef struct
{
	U8 State;
	DataPayload_t DataPayload;
	
}DataSession_brdcst_t;




/*
 Audio Routing Control
This message is used to control the audio routing path between audio inputs and audio
outputs on a device. A device’s default audio routing is defined by the device and can
be obtained by requesting the default configuration then monitoring the resulting broadcast
*/
#define AUDIO_ROUTING_CONTROL   0x414

//Function
#define Routing_Func_Default_Source (unsigned char)0x00 //Revert to the device’s default input source (platform specific)
#define Routing_Func_Update_Source (unsigned char)0x01 //Switch the devices input source to the Input Source specified in this message
#define Routing_Func_Query (unsigned char)0x04 //Request the current audio routing pairs and pre-defined routing type in use

//Routing Data---Audio Input----
//The device’s selected microphone.
#define IN_Microphone (unsigned char)0x01

//The device’s raw baseband demodulated data in receive mode.
//This data is the output of the demodulated mode which has the combined high speed audio and low speed data (e.g. MDC, TPL, DPL and Squelch).
//NOTE: Applicable for analog mode only.
#define IN_Raw_received_audio_demodulated (unsigned char)0x07

//The device’s raw baseband audio data in receive mode.
//In analog mode this data is the sub-band filtered audio data (raw high speed audio).
//NOTE: Applicable for analog mode only.
#define IN_Raw_received_audio_PCM (unsigned char)0x08

//The device’s raw baseband un-modulated data in transmit mode.
//This data is the input to the RF modulator which has the combined high speed audio and low speed data (e.g. MDC, TPL, DPL and Squelch).
//NOTE: Applicable for analog mode only.
#define IN_Raw_transmit_audio_unmodulated (unsigned char)0x09

//The device’s raw baseband audio data in transmit mode.
//In analog mode this data is the flat PCM audio data (raw high speed audio).
//NOTE: Applicable for analog mode only.
#define IN_Raw_transmit_audio_PCM (unsigned char)0x0B

//The device’s option board input port
#define IN_Option_Board (unsigned char)0x0C

//The device's non-amplified audio data that is generally routed to the speaker
#define IN_Pre_Speaker_Audio_Data (unsigned char)0x0D


//The device's post-AMBE Audio encode audio data in the transmit mode. 
#define Post_AMBE_Encoder (unsigned char)0x0F

//The device's pre-AMBE Audio decode audio data in the receive mode. 
#define Pre_AMBE_Decoder (unsigned char)0x10

//The device's Raw Voice header in Transmit mode
#define Tx_Voice_Header (unsigned char)0x11

//The device's Raw Voice header in Receive mode
#define Rx_Voice_Header (unsigned char)0x12

//The device's Voice Terminator in Transmit mode.
#define Tx_Voice_Terminator (unsigned char)0x13

//The device's Voice Terminator in Receive mode.
#define Rx_Voice_Terminator (unsigned char)0x14




//Routing Data---Audio Output----
//The device’s selected speaker.
#define OUT_Speaker (unsigned char)0x01

//The device’s raw baseband demodulated data in receive mode.
//This data is the output of the demodulated mode which has the combined high speed audio and low speed data (e.g. MDC, TPL, DPL and Squelch).
// NOTE: Applicable for analog mode only.
#define OUT_Raw_received_audio_demodulated (unsigned char)0x07

//The device’s raw baseband audio data in receive mode.
//In analog mode this data is the sub-band filtered audio data (raw high speed audio).
//NOTE: Applicable for analog mode only.
#define OUT_Raw_received_audio_PCM (unsigned char)0x08

//The device’s raw baseband un-modulated data in transmit mode.
//This data is the input to the RF modulator which has the combined high speed audio and low speed data (e.g. MDC, TPL, DPL and Squelch).
//NOTE: Applicable for analog mode only.
#define OUT_Raw_transmit_audio_unmodulated (unsigned char)0x09

//The device’s raw baseband audio data in transmit mode.
//In analog mode this data is the flat PCM audio data (raw high speed audio).
//NOTE: Applicable for analog mode only.
#define OUT_Raw_transmit_audio_PCM (unsigned char)0x0B

//The device’s option board input port.
#define OUT_Option_Board (unsigned char)0x0C

//The device’s microphone voice data that is routed to the radio for transmission.
#define OUT_Microphone_Data (unsigned char)0x0D





typedef struct
{
	unsigned char audioInput;
	unsigned char audioOutput;
}RoutingData_t;

#define MAX_ROUTING_CTR		10

typedef struct
{
	unsigned char Function;
	unsigned char NumberofRoutings[2];
	RoutingData_t RoutingData[MAX_ROUTING_CTR];
}AudioRoutingControl_req_t;

typedef struct
{
	unsigned char Result;
	unsigned char Function;
	unsigned char NumberofRoutings[2];
	RoutingData_t RoutingData[MAX_ROUTING_CTR];
}AudioRoutingControl_reply_t;

typedef struct
{
	unsigned char NumberofRoutings[2];
	RoutingData_t RoutingData[MAX_ROUTING_CTR];
	unsigned char Function;
}AudioRoutingControl_brdcst_t;
/*
Device Control Mode
This message is used to request to radio to enable device control mode. In this mode of 
 operation, the radio assumes that the XCMP device is the master that determines 
 actions on the radio. The radio, in this mode, based on the type of control, forwards 
 input signals to the controlling device for the device to decide what action needs to be 
 taken by the radio. In this mode, the radio does not take action on the inputs. The 
 controlling device in this mode uses other XCMP control messages to initiate action on 
 the radio.
*/

#define DEVICE_CONTROL_MODE 0x421

//function
#define DCM_EXIT 0x00 //To request exiting the “Device Control Mode” on the radio.
#define DCM_ENTER 0x01 //To request entering the “Device Control Mode” on the radio.
#define DCM_REVOKE 0x02 //To revoke the “Device Control Mode” by the radio. 
//NOTE: This function is valid only for the broadcast message type.Device Control Mode
						
//control type
#define DCM_DEFAULT 0x00 //Default setting. Should be ignored.
#define DCM_USER_INPUT 0x01 //All user inputs received by the radio are forwarded to the device to take action. 
//The radio shall use the PUI Broadcast message to indicate the user inputs to the device.

#define DCM_SIG_INPUT 0x02 //All signaling inputs received by the radio are forwarded to the device to take action. 
//The radio shall use the Sig Detect Broadcast message to indicate the signaling inputs to the device.

#define DCM_SPEAKER_CTRL 0x08 //All Speaker control will be handled by controlling device.
//Radio shall not mute/unmute the speaker unless controlling device requests.

#define DCM_HYBRID_BT_CTRL 0x20 //Some Bluetooth control can be initiated from the controlling device. 
//But, the Radio will handle the processing and interaction with the remote Bluetooth device.
// Some additional Bluetooth related status broadcast message (e.g. connect request failure) 
//will be sent based on the device requested function.

#define DCM_FORWARD_CAI_IP_DATA 0x40 //Specifies the Radio received In/Out bound IP datagram should be 
//routed to the controlling device. Filtering of specific IP datagram is product specific implementation

#define DCM_FORWARD_XCMP_INPUT 0x80 //All XCMP Requests received by the radio from any accessory are forwarded 
//to the device which sends the Device_Control_Mode message. Messages are forwarded using the Forward_Data message.
//The controlling device creates the reply and sends it back to the accessory via the radio 
//(also using the Forward_Data message). This message is mutually exclusive with XCMP_INPUT.

//struct
typedef struct
{
	unsigned char Function;
	unsigned char ControlTypeSize;
	unsigned char ControlType;
}DeviceControlMode_req_t;

typedef struct
{
	unsigned char Result;
	unsigned char Function;
	unsigned char ControlTypeSize;
	unsigned char ControlType;
}DeviceControlMode_reply_t;

typedef struct
{
	unsigned char Function;
	unsigned char ControlTypeSize;
	unsigned char ControlType;
}DeviceControlMode_brdcst_t;


/*

Call Control ----------------------------

This command allows a device to request that a call be initiated, terminated, answered,
rejected, or to obtain the status of a call. The remote address represents the target 
address for encode or the source address for decode.

*/
//Call type

#define NO_Call_Feature				 0x00
#define Selective_call				0x01
#define Call_Alert					0x02
#define Enhanced_Private_call		 0x04
#define Enhanced_Phone_call			0x05
#define Group_Call					0x06
#define Call_Alert_Voice			0x08
#define Telegram_call				 0x09
#define Group_Phone_call			0x0A
#define Braoadcast_Call				0x0B

//Call state

#define Call_Decoded				 0x01
#define Call_In_Progress			 0x02
#define Call_Ended					 0x03
#define Call_Initiated				0x04
#define NO_ACK						0x06
#define Call_In_Hangtime			 0x07
#define Call_Decoded_Clear			0x08
#define Call_Decoded_Key_Matched	0x09
#define Call_Decoded_Key_Mismatched	0xA0


typedef struct
{
	unsigned char Calltype;
	unsigned char Callstate;
	
	//后面结构忽略
	//unsigned char Remote_Address;
	//unsigned char Group_ID_Information;
	
}CallControl_brdcst_t;




/*

Battery Level----------------------------

This message is used by a device to report its current battery level in three different
formats: battery state, battery charge (if known) and battery voltage (if known). This
message is broadcast any time that battery state changes, or any time that battery
charge and/or battery voltage changes by a significant amount


*/
//command-broadcast
#define BATLVL  0x410

//battery state
#define Battery_Okay  0x00
#define Battery_Low   0x01

//battery charge

//battery voltage


typedef struct
{
	unsigned char  State;
	unsigned char  Charge;
	unsigned short Voltage;
	
}BatteryLevel_brdcast_t;



/*

Transmit Control ----------------------------

This message is used to put the device in the Transmit mode or remove it from the
Transmit mode. The selection of Microphone and Speaker for the functions shall be
done using the Speaker Control and the Microphone Control messages.


*/


#define KEYREQ  0x415

//Function

#define KEY_UP  0x01
#define DE_KEY  0x02

//Mode 0f Operation


#define MODE_VOICE  0x00
#define MODE_ALL    0x02
#define MODE_VOICE_TPT  0x03

//State 
#define STADNDBY_RECEIVE   0x00
#define Transmit           0x01

typedef struct
{
	unsigned char Function;
	unsigned char Mode_Of_Operation;
	unsigned char TT_Source;
	
}TransmitControl_req_t;

typedef struct
{
	unsigned char Result;
	unsigned char Function;
	unsigned char Mode_Of_Operation;
	unsigned char State;
	
}TransmitControl_reply_t;

typedef struct
{
	unsigned char Mode_Of_Operation;
	unsigned char State;
	unsigned char State_change_reason;
	
}TransmitControl_brdcast_t;







/*
Mic control ----------------------------
The message is used by the device to control the microphone and its gain factor. The
device is able to control the internal microphone and an external device microphone, if available.
*/

#define MIC_CONTROL			0x40E
//Function
#define Mic_Enable		0x01
#define Mic_Disable		0x02
#define Mic_Select		0x03
#define Mic_Mute		0x08
#define Mic_Unmute		0x09

//Microphone Type
#define Mic_Internal		0x00
#define Mic_External		0x01

//Signaling type
#define Sig_Type_Analog		0x00
#define Sig_Type_Digital	0x01

//Mic State
#define Mic_Invalid			0xFF
#define Mic_Disabled		0x00
#define Mic_Enabled			0x01
#define Mic_Muted			0x03
#define Mic_Unmuted			0x02
#define Mic_Enabled_SEL	    0x11	//The microphone is enabled and selected


//Gain Factor Offset

typedef struct
{
	
	unsigned char Function;
	unsigned char Mic_Type;
	unsigned char Signaling_Type;
	unsigned char Gain_Offset;
	
}MicControl_req_t;


typedef struct
{
	unsigned char Result;
	unsigned char Function;
	unsigned char Mic_Type;
	unsigned char Signaling_Type;
	unsigned char Mic_State;
	unsigned char Gain_Offset;
	
}MicControl_reply_t;


typedef struct
{

	unsigned char Mic_Type;
	unsigned char Signaling_Type;
	unsigned char Mic_State;
	unsigned char Gain_Offset;
	
}MicControl_brdcast_t;


/*
Device Management ----------------------------
This message is used to manage the non-XNL master device from the XNL master or
another non-XNL master device. The following functionality can be managed for the
non-XNL master device:
	Device can be enabled.
	Device can be disabled.
	Device can be programmed through another non-XNL master device using Data
Session.
		o Non-master device can start the programming of the managed non-master
device via the radio (XNL master device).
		o Non-master device can stop the programming of the managed non-master
device via the radio (XNL master device).

Another non-master device can reset the device under management by sending
the message to the Radio after completion of programming of the non-XNL master device.
Any attached device can query the status of the device under management.
Device can be enabled, disabled, reset or perform programming from another non-XNL
master device. To program or reset the unique device in the system, the device type
and XCMP device ID is included in the message. The XCMP device ID can be obtained
from the XNL system map broadcast.
If the Device Type and XCMP Device ID are cleared to a value of 0x00, Device
Management Request/Broadcast is using the sender Device as the Device being managed.
*/

//Function
typedef enum 
{
	Query	= 0x00,
	Start	= 0x03,
	Stop	= 0x04,
	Reset	= 0x05,
	GOB_Boot_Pin_Enable =	0x06,
	GOB_Boot_Pin_Disable =	0x07,
	Reset_Master		=	0x08
	
}DEVMGMTBCST_Function;
	

//State
typedef enum
{
	DEFAULT	= 0x00,
	NODETECT,
	ENABLED,
	DISABLED,
	PROGRAM,
	RESET,
	BOOT_MODE,
	PASSWORD_LOCK
	
	}DEVMGMTBCST_DeviceState;

typedef struct
{
	unsigned char Function;
	unsigned char Device_Type;
	unsigned char XCMP_Device_ID;
}DeviceManagement_req_t;


typedef struct
{
	unsigned char Result;
	unsigned char Function;
	unsigned char Device_Type;
	unsigned char XCMP_Device_ID;
	unsigned char Device_State;
}DeviceManagement_reply_t;

typedef struct
{

	unsigned char Function;
	unsigned char Device_Type;
	unsigned char XCMP_Device_ID;
	unsigned char Device_State;
	
}DeviceManagement_brdcast_t;




/*
Speaker control ----------------------------
This message is used to request the device to change the state of the currently selected
speaker. This message is also used to query the current state of the selected speaker.

The radio automatically selects an external speaker device when it is connected to the
radio. When no external speaker device is connected, the radio selects the internal
speaker. A mobile radio mutes the control head speakers (MMP) and MAP speakers
when an external handset speaker is connected.
*/
#define SPEAKER_CONTROL			0x407

//Function /State
#define MUTED (unsigned short )0x0000
#define UNMUTED (unsigned short) 0x0001
#define QUREY (unsigned short)0x0080

//Speaker Number
#define Invalid 0x0000  //Invalid
#define Primary 0x0001  //Primary speaker
#define Secondary 0x0002  //Secondary speaker
#define All 0xFFFF //All Speakers

typedef struct
{
	unsigned char SpeakerNumber[2];
	unsigned char Function[2];
}SpeakerControl_req_t;


typedef struct
{
	unsigned char Result;
	unsigned char SpeakerNumber[2];
	unsigned char State[2];
}SpeakerControl_reply_t;


typedef struct
{
	unsigned char SpeakerNumber[2];
	unsigned char State[2];
}SpeakerControl_brdcast_t;

/*
initialize the xnl layer
initialize the queue
Create the corresponding task;
*/

void xcmp_multi_tx( U8 * data_ptr, U32 data_len);

void xcmp_tx( U8 * data_ptr, U32 data_len);
void xcmp_tx_method( U8 * data_ptr, U32 data_len, U16 target);

void xcmp_init(void);

/*register the app function(callback function)*/
void xcmp_register_app_list(void * list);

/*send device initialize status request*/
void xcmp_DeviceInitializationStatus_request(void);

/*send not supported XCMP opcode*/
void xcmp_opcode_not_supported(void);

/*send tone to test*/
void xcmp_IdleTestTone(U8 type, U16 toneID);

void xcmp_audio_route_mic(void);
void xcmp_audio_route_speaker(void);
void xcmp_audio_route_revert(void);
void xcmp_enter_device_control_mode(void);
void xcmp_exit_device_control_mode(void);
void xcmp_unmute_speaker( void );
void xcmp_function_mic( void );
void xcmp_mute_speaker( void );
void xcmp_transmit_control( void );
void xcmp_transmit_dekeycontrol( void );
void xcmp_data_session_req(void *message, U16 length, U32 dest);
void xcmp_data_session_brd( void *message, U16 length, U8 SessionID);
void xcmp_button_config(void);
void xcmp_volume_control(void);
void xcmp_enter_enhanced_OB_mode(void);
void xcmp_exit_enhanced_OB_mode(void);
void xcmp_audio_route_encoder_AMBE(void);
void xcmp_audio_route_decoder_AMBE(void);
void xcmp_audio_route_AMBE(void);
void xcmp_data_session_csbk_raw_req(void *data, U16 data_ength);

#endif /* XCMP_H_ */