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





/* Define the DMA buffer size and buffer number:
 *
 * For SSI interface, 1 slot is 2bytes.
 * For 8 slot mode used in GOB, McBsp/SSC frame length is 8*2 = 16bytes.
 * A DMA frame consists of 10 McBsp frames, whose size is 10*16=160 bytes.
 *
 * How to decide the total buffer size for DMA frames?
 *
 * For 8 slot mode, SSI CLK rate is 8K*8*16 = 1.024MHz,
 * Within one second, there will be 1024000/8=128000Bytes received.
 * That's 128000/160=800 DMA frames, means each DMA frame takes 1.25ms.
 * Every 5ms, physical layer will check the data received in DMA buffer.
 * During the 5ms, there will be 4 DMA frames of data received.
 * Thus theoretically, to prevent buffer overflow, minimum buffer depth is 4.
 *
 * However, to make enough buffer space, 16 (instead of 4) DMA buffers are used
 * in actual code. In this way, in worst case, only 25% buffers (that's 4 of 16)
 * are occupied with received data which has not been processed by physical layer.
 *
 * And since GOB olny tx/rx on 6 slots of all 8 slots, thus the total buffer
 * size is 6*2*10*16=1920bytes.
 *
 * To simplify control logic, tx buffer use the same size and number of rx.
 */

#define DMABUFNUM 16//frame change
#define DMASIZE 10//frame change
#define SSI_FRAME_BUF_SIZE 6	/* 6 half-word = 12 bytes */
#define DMA_BUF_SIZE (DMASIZE * SSI_FRAME_BUF_SIZE)	/* 60 half-word = 120 bytes, total 120* 16 = 1920 bytes */


/*Initialize the SSC and PDCA */
void ssc_init(void);
void sync_ssi(void);

/*register the rx/tx function(callback function)*/

void register_rx_tx_func(void (*rx_exec)(void *),  void ( *tx_exec)(void *));


#endif /* SSC_H_ */