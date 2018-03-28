/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: ssc.c
Author: wxj
Version: 1.0.0.01
Date: 22016/3/15 14:20:51

Description:
History:
*/

#ifndef SSC_H_
#define SSC_H_

/*macro for send/receive physical data*/
/*The start of the transfer unit is designated by the 16-bit 0xABCD header*/
#define PHYHEADER32      (U32)0xABCD0000

/*
packet tailed 
The packet will always end with 0x00BA.
*/
#define PHYTERMLEFT      (U32)0x00BA0000 
#define PHYTERMRIGHT     (U32)0x000000BA

/*
idle frame
| Slot 1 | Slot 2 | Slot 3 | Slot 4 | Slot 5 | Slot 6 | Slot 7 | Slot 8 |
|--------+--------+--------+--------+--------+--------+--------+--------|
| 0xXXXX | 0xXXXX | 0xABCD | 0x5A5A | 0xABCD | 0x5A5A | 0x0000 | 0x0000 |
*/
#define XNL_IDLE         (U32)0xABCD5A5A
#define PAYLOADIDLE0     (U32)0xABCD5A5A
#define PAYLOADIDLE1     (U32)0x00000000

#define EN_OB_PAYLOAD    (U32)0xABCDC00E

#define DE_OB_PAYLOAD    (U32)0xABCDC032

#define ENCODER_PAYLOAD    (U16)0x88F2
#define DECODER_PAYLOAD    (U16)0x88F3
#define RIP_PAYLOAD		   (U16)0x847F//radio internal parameter
#define SDV_PAYLOAD		   (U16)0x9A13//soft decision value



/**
Each slot within a SSI frame is designated for specific use in the transport of
information between the radio¡¯s main board and the option board.

| Slot 1 | Slot 2 | Slot 3 | Slot 4 | Slot 5 | Slot 6 | Slot 7 | Slot 8 |
|-----------------+-----------------+-----------------------------------|
|    Reserved     |   XNL Channel   |        Payload Channel            |



Slots 1 and 2 are reserved for main board use only to transport data between
devices within the radio.
**/
typedef union {
    U32    dword;
    U16    word[2];
    U8    byte[4];
} reserved_channel_t;

/*
Slots 3 and 4 are designated for XCMP / XNL communication between the main board
and the option board; these slots constitute the XNL Channel.
*/
typedef union {
    U32    dword;
    U16    word[2];
    U8    byte[4];
} xnl_channel_t;

/*
Slot 5 to the last slot (slot 8) is designated for data payload transport
between the main board and the option board; these slots constitute the Payload
Channel.
*/
typedef union {
    U32    dword[2];
    U16    word[4];
    U8    byte[8];
} payload_channel_t;

/*ssc frame*/
typedef struct {
    reserved_channel_t    reserved_channel;
    xnl_channel_t    xnl_channel;
    payload_channel_t    payload_channel;
} ssc_fragment_t;

/*Initialize the SSC and PDCA */
void ssc_init(void);

/*register the rx/tx function(callback function)*/

void register_rx_tx_func(void (*rx_exec)(void *),  void ( *tx_exec)(void *));


#endif /* SSC_H_ */