/*
 * voice.h
 *
 * Created: 2017/6/9 星期五 下午 17:32:56
 *  Author: Edwards
 */ 


#ifndef VOICE_H_
#define VOICE_H_

#include "data_flash.h"
#include "xcmp.h"
#include "rtc.h"


#define TRUE 1
#define FALSE 0

/**

label:			"MOTOREC"(7bytes)

voice_numbers:	0x xxxx(2bytes)

index_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes);
index_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes);
...
...
index_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes);


**/


#define LABEL_ADDRESS				0x000000
#define LABEL_LENGTH				0x07//7bytes:"MOTOREC"

#define VOICE_NUMBERS_ADDRESS		0x00000A
#define VOICE_NUMBERS_LENGTH		0x02//2bytes:0x xxxx

#define START_ADDRESS_OF_VOICE_INFO	0x000010
//#define VOICE_INFO_LENGTH	0x08//7bytes:list_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes)
#define VOICE_INFO_LENGTH	0x48//72bytes:list_number(2bytes) + VoiceHeader_t(64bytes) + address(4bytes) + length(2bytes)
#define VOICE_INFO_HEADER_LENGTH	0x40

#define VOICE_LIST_BOUNDARY 0x080000 //512k
#define VOICE_DATA_START_ADDRESS 0x090000



#define VoiceHeader (0xA5A5DCBA)

#define Transmit_Call 0x01
#define Receive_Call	0x00

#pragma  pack(1)

typedef struct  // 16bytes
{
	unsigned int RadioId;
	unsigned short Zone;
	unsigned short Channel;
	unsigned char SignalingMode;
	unsigned char Rev[7];
}Local_t;

typedef struct //8bytes
{
	unsigned char Type;
	unsigned char Direction : 1;
	unsigned char Pads : 7;
	unsigned char Rev[6];
}Call_t;

typedef struct //64 bytes;
{
	unsigned int Header;
	unsigned int Index;
	Local_t Local;
	RemoteAddress_t Remote;
	Call_t Call;
	date_time_t Time; //8bytes
	unsigned char VoiceType; //Ambe
	unsigned char Rev[7];
}VoiceHeader_t;

typedef struct //16bytes;
{
	unsigned int Offset;
	unsigned int Length;
	unsigned char Rev[8]
}VoicePayload_t;

typedef struct // 80bytes
{
	VoiceHeader_t Header;
	VoicePayload_t Payload;
}Voice_t;


typedef struct
{
	unsigned int Header;
	Voice_t BeginVoice;
	Voice_t EndVoice;
}Rec_t;

#pragma pack()


#pragma pack(1)
typedef struct
{
	unsigned short	numb;
	VoiceHeader_t	Header;
	unsigned int	address;
	unsigned short	offset;

}VoiceList_Info_t;//72bytes
#pragma pack()


void voc_init(void);
Bool voc_read_info( unsigned int index,  VoiceList_Info_t * voice);
Bool playback_voice_data(U32 voice_index);
Bool voc_save_data(void *data_ptr, U16 data_len, U8 voice_end_flag);
Bool voc_save_info(VoiceList_Info_t * voice);

void voc_read_write_test(void);

//void voc_get_new_index(Voice_t * voice);

//void voc_save_dat(unsigned int index, unsigned int offset, unsigned char * buffer, unsigned int length);
//void voc_save_info(Voice_t * voice);

//void voc_read_rec(Rec_t * rec);
//Bool voc_read_info( unsigned int index, unsigned char count,  Voice_t * voice);
//Bool voc_read_data(unsigned int index, unsigned int offset, unsigned int length, void * buffer);



#endif /* VOICE_H_ */