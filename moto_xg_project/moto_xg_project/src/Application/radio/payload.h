/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: payload.c
Author: wxj
Version: 1.0.0.01
Date: 2016/4/6 13:21:55

Description:
History:
*/

#ifndef PAYLOAD_H_
#define PAYLOAD_H_

//Fixed size 256 byte Rx fragment.
typedef struct {
	phy_header_t     phy_header;  // 2  words
	U16				 payload[126]; //126 words
} payload_fragment_t;

/*
register the function to receive/transmit payload
Create the corresponding task;
*/
void payload_init(void ( *payload_rx_func)(void * ), void (*payload_tx_func)(void *));
#endif /* MEDIA_H_ */