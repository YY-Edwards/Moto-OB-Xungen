/*
 * RFID.h
 *
 * Created: 2017/8/10 星期四 下午 16:21:48
 *  Author: Edwards
 */ 


#ifndef RFID_H_
#define RFID_H_
#include "rc522.h"
#include "timer.h"

#define DEST 9;

U8 CT[2];				//卡类型
U8 SN[4];				//卡号	
U8 RFID[16];			//存放RFID
U8 unsure_data[5]={0x04, 0x0d, 0x00, 0x0a, 0x00};
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
void rfid_sendID_message();






#endif /* RFID_H_ */