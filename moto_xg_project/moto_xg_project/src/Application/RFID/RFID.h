/*
 * RFID.h
 *
 * Created: 2017/8/10 星期四 下午 16:21:48
 *  Author: Edwards
 */ 


#ifndef RFID_H_
#define RFID_H_
#include <string.h>
#include "rc522.h"
#include "timer.h"
#include "xcmp.h"
#include "log.h"
#include "xgrtc.h"
#include "xgflash.h"



#define DEST 1050//目的ID
#pragma pack(1)
typedef struct
{
	U16 length;//0x0008+
	U16 type;//0xa000(group);0xe000(point)
	U8  session_id;//0x81~0x9f:30
	U8  fixed_data[5];//0x04 0d 00 0a 00
	
}Message_Header_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	//unsigned char	RFID_ID[4];
	unsigned char	RFID_ID[16];//Unicode码,大端模式
	//DateTime_t		XG_Time;

}Message_Data_t;//22bytes
#pragma pack()

#pragma pack(1)
typedef struct
{

	Message_Header_t	header;
	Message_Data_t		data;

}Message_Protocol_t;//10+22bytes
#pragma pack()


void rfid_init();
U8 rfid_sendID_message();
U8 scan_rfid_save_message();
U8 rfid_auto_reader(void *card_id);
U8 scan_patrol(char* SN);






#endif /* RFID_H_ */