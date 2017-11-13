/*
 * RFID.c
 *
 * Created: 2017/8/10 星期四 下午 16:21:29
 *  Author: Edwards
 */ 

#include "RFID.h"
U8 CT[2];				//卡类型
U8 SN[4];				//卡号
U8 RFID[16];			//存放RFID
U8 unsure_data[5]={0x04, 0x0d, 0x00, 0x0a, 0x00};


void rfid_init()
{
	char card_id[4]={0};
	//tc_init();//用定时器200ms自动寻卡
	
	rc522_init();
	
	//if(rfid_auto_reader(card_id) == 0){
		//log("card_id : 0x%x, 0x%x, 0x%x, 0x%x\n", &card_id[0], &card_id[1], &card_id[2], &card_id[3]);	
	//}
		
}

//流程：寻卡->防冲撞->选卡->发送卡号
U8 rfid_auto_reader(void *card_id)
{
	U8 status = MI_ERR;
	
	PcdReset();
//while(1){
	status=PcdRequest(PICC_REQALL,	CT);//寻天线区内全部的卡，返回卡片类型 2字节
	if(status!=MI_OK) return status;
	//continue;
	
	if(CT[0]==0x04&&CT[1]==0x00)
		log("MFOne-S50\n");
	else if(CT[0]==0x02&&CT[1]==0x00)
		log("MFOne-S70\n");
	else if(CT[0]==0x44&&CT[1]==0x00)
		log("MF-UltraLight\n");
	else if(CT[0]==0x08&&CT[1]==0x00)
		log("MF-Pro\n");
	else if(CT[0]==0x44&&CT[1]==0x03)
		log("MF Desire\n");
	else
		log("Unknown\n");
		
	status=PcdAnticoll(SN);//防冲撞，返回卡的序列号 4字节
	if(status!=MI_OK)
	{
		xcmp_IdleTestTone(Tone_Start, BT_Disconnecting_Success_Tone);//set tone to noticy failure!!!
		return status;
	}
	//continue;
	
	//memcpy(MLastSelectedSnr,&RevBuffer[2],4);
	status=PcdSelect(SN);//选卡
	if(status!=MI_OK)return status;
	//continue;
	else{//选卡成功
			
		memcpy(card_id, SN, 4);
		log("select okay\n");
		//continue;
		return status;	
	}
	
//}
	
}

extern volatile DateTime_t Current_time;
U8 rfid_sendID_message()
{
	char SN[10];
	//char data_buffer[16];
	char message[80];
	U8 return_err =0;
	U8 temp =0;
	U8 destination = DEST;
	static U8 start_session = 0x80;
	Message_Header_t header;
	Message_Data_t data_buffer;//22bytes
	
	//memset(data_buffer, 0x00, 16);
	memset(SN, 0x00, 10);
	memset(message, 0x00, 80);

	return_err = rfid_auto_reader(SN);
	
	if(return_err == 0){
		
		log("card_id : %X, %X, %X, %X\n", SN[0], SN[1], SN[2], SN[3]);
		xcmp_IdleTestTone(Tone_Start, BT_Connection_Success_Tone);//set tone to indicate scan rfid success!!!		 
		for(int i = 0; i<4; i++){//将Unicode码转换为大端模式	
		
			 temp = ((SN[i] & 0xF0) >> 4);//取字节高四位
			 if((temp >= 0) && (temp <= 9))data_buffer.RFID_ID[i*4] = temp+0x30;
			 else 
				data_buffer.RFID_ID[i*4] = ((temp - 0x0a)+0x61);
			
			 data_buffer.RFID_ID[i*4+1] = 0x00;
		 
			 temp = (SN[i] & 0x0F);//取字节低四位
			 if((temp >= 0) && (temp <= 9))data_buffer.RFID_ID[i*4+2] = temp+0x30;
			 else
				data_buffer.RFID_ID[i*4+2] = ((temp - 0x0a)+0x61);

			 data_buffer.RFID_ID[i*4+3] = 0x00; 
		}
		
		memcpy(&data_buffer.XG_Time.Year, &Current_time.Year, sizeof(DateTime_t))	;
	
		header.length = (0x0008 + sizeof(Message_Data_t));
	
		if(start_session > 0x9f)start_session = 0x80;
	
		header.session_id = (++start_session);
	
		memcpy(&header.fixed_data[0], unsure_data, sizeof(unsure_data));
		header.type = 0xe000;
	
		memcpy(message, &header, sizeof(Message_Header_t));//拷贝header数据
		memcpy(&message[sizeof(Message_Header_t)], &data_buffer, sizeof(Message_Data_t));//拷贝短信内容数据
	
		xcmp_data_session_req(message, (sizeof(Message_Header_t)+sizeof(Message_Data_t)), destination);
		
	}
	else
	{
		xcmp_IdleTestTone(Tone_Start, BT_Disconnecting_Success_Tone);//set tone to indicate scan rfid failure!!!
		log("no card find...\n");
	}
	
	return return_err;
	
}







