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


#define DEST 9;//目的ID

U8 CT[2];				//卡类型
U8 SN[4];				//卡号	
U8 RFID[16];			//存放RFID
#pragma pack(1)
typedef struct
{
	U16 length;//0x0008+
	U16 type;//0xa000(group);0xe000(point)
	U8  session_id;//0x81~0x9f:30
	U8  fixed_data[5];//0x04 0d 00 0a 00
	
}Message_Header_t;
#pragma pack()

void rfid_init();
U8 rfid_auto_reader(void *card_id);
U8 rfid_sendID_message();






#endif /* RFID_H_ */