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

void rfid_init();
bool rfid_auto_reader();
void rfid_sendID_message();



#endif /* RFID_H_ */