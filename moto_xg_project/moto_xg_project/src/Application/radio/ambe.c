/*
 * ambe.c
 *
 * Created: 2016/6/21  14:05:57
 *  Author: JHDEV2
 */ 
#include "ambe.h"
//Public_AMBEkey:encryption key
//Note:
//0x1949 is not used
const U16 Public_AMBEkey[] ={0x1AC3, 0x1840 , 0xA380 , 0x1949};



/**
Function: CalculateBurst
Description: Calculate the Burst flag 
Calls:
Called By: phy_payload_rx
*/

volatile RxAMBEBurstType CalculateBurst(U8 vf_sn)
{
	U8 Burst_ID = 0;
	
	switch(vf_sn)
	{
		case 0x01:
		case 0x02:
		case 0x03:
		
			Burst_ID = 0x0A;
			return VOICEBURST_A;
		
		case 0x04:
		case 0x05:
		case 0x06:
		
			Burst_ID = 0x0B;
			return VOICEBURST_B;
		
		case 0x07:
		case 0x08:
		case 0x09:
		
			Burst_ID = 0x0C;
			return VOICEBURST_C;
		
		case 0x0A:
		case 0x0B:
		case 0x0C:
		
			Burst_ID = 0x0D;
			return VOICEBURST_D;
		
		case 0x0D:
		case 0x0E:
		case 0x0F:
		
			Burst_ID = 0x0E;
			return VOICEBURST_E;
			
		case 0x10:
		case 0x11:
		case 0x12:
		
				Burst_ID = 0x0F;
			return VOICEBURST_F;     
		
		default:
		
			Burst_ID = 0x00;
			return VOICE_WATING;
		
	}
	
}








