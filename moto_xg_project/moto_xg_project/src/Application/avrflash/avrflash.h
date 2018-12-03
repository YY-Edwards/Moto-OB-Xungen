/*
 * avrflash.h
 *
 * Created: 2018/12/3 16:40:52
 *  Author: Edwards
 */ 


#ifndef AVRFLASH_H_
#define AVRFLASH_H_
#include "flashc.h"


void write_flash_in_multitask(volatile void *dst, const void *src, size_t nbytes);
void avr_flash_test(void);


#endif /* AVRFLASH_H_ */