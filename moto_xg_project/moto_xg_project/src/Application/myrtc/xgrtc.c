/*
 * xgrtc.c
 *
 * Created: 2017/11/6 14:04:07
 *  Author: Edwards
 */ 
#include "xgrtc.h"

static volatile xSemaphoreHandle rtc_mutex = NULL;

// To specify we have to print a new time
volatile static int print_sec = 1;

// Time counter
//static int sec = 0;

volatile U32 Time_scale;

#if __GNUC__
__attribute__((__interrupt__))
#elif __ICCAVR32__
/* RTC Interrupt  */
#pragma handler = AVR32_RTC_IRQ_GROUP, 1
__interrupt
#endif
static void rtc_irq(void)
{
	U8 T,K;
	U32 T16,K16;
	// Increment the minutes counter
	//sec++;
	
	Current_time.Second++;
	if(Current_time.Second>=60)
	{
		Current_time.Second =0;
		Current_time.Minute++;
		if(Current_time.Minute>=60)
		{
			Current_time.Minute=0;
			Current_time.Hour++;
			if (Current_time.Hour>=24)
			{
				Current_time.Hour=0;
				K16=(Current_time.Year&0x03)?365:366;				//判断当年是否为闰年,并获取当年的总天数（此算法时间起点为2000年）
				K16-=337;											//计算当年二月份天数
				//统计当月天数				
				K=(Current_time.Month==2)?K16:(((Current_time.Month+(Current_time.Month>>3))&0x01)+30);			
				Current_time.Day++;
				if(Current_time.Day>K)
				{	
					Current_time.Day = 1;//重置到下月1号
					Current_time.Month++;
					if(Current_time.Month>12)
					{
						Current_time.Month = 1;//重置到下一年的第一个月
						Current_time.Year++;
						if (Current_time.Year>150)Current_time.Year=0;//重置年份											
					}
					
				}
			}
		}
	}
	
	//Time_scale++;

	// clear the interrupt flag
	rtc_clear_interrupt(&AVR32_RTC);

	// specify that an interrupt has been raised
	print_sec = 1;
}

/*!
 * \brief print_i function : convert the given number to an ASCII decimal representation.
 */
char *print_i(char *str, unsigned int n)
{
  int i = 10;

  str[i] = '\0';
  do
  {
    str[--i] = '0' + n%10;
    n /= 10;
  }while(n);

  return &str[i];
}

static void RTC_Test(void)
{
	
	temp_time.Year		= 18;
	temp_time.Month		= 3;
	temp_time.Day		= 1;
	temp_time.Hour		= 23;
	temp_time.Minute	= 59;
	
	temp_time.Second	= 50;
	//Time_scale = RTC_ENcodeTime(&temp_time);
	//RTC_DecodeTime(Time_scale, &Current_time);
	
	//Current_time.Second = 35;
	//Time_scale = RTC_ENcodeTime(&Current_time);
	//RTC_DecodeTime(Time_scale, &temp_time);
	//
	//Current_time.Second = 48;
	//Time_scale = RTC_ENcodeTime(&Current_time);
	//RTC_DecodeTime(Time_scale, &temp_time);

	
	
	//Enable_global_interrupt();
	

}



void xg_rtc_init(void)
{
	
	// Disable all interrupts. */
	//Disable_global_interrupt();
	  //
	//// Register the RTC interrupt handler to the interrupt controller.
	//INTC_register_interrupt(&rtc_irq, AVR32_RTC_IRQ, AVR32_INTC_INT1);
//
	//// Initialize the RTC
	////if (!rtc_init(&AVR32_RTC, RTC_OSC_RC, RTC_PSEL_RC_1_76HZ))
	//if (!rtc_init(&AVR32_RTC, RTC_OSC_32KHZ, RTC_PSEL_32KHZ_1HZ))
	//{
		//log_debug("Error initializing the RTC\r\n");
	//}
	//// Set top value to 0 to generate an interrupt every seconds */
	//rtc_set_top_value(&AVR32_RTC, 0);
	//// Enable the interrupts
	//rtc_enable_interrupt(&AVR32_RTC);
	//// Enable the RTC
	//rtc_enable(&AVR32_RTC);

	Current_time.Year		= 18;
	Current_time.Month		= 12;
	Current_time.Day		= 03;
	Current_time.Hour		= 16;
	Current_time.Minute		= 25;
	Current_time.Second		= 40;
	
	// Enable global interrupts
	//Enable_global_interrupt();
	  
	//RTC_Test();	

}

U32 RTC_EncodeTime(DateTime_t *DT)
{
	   volatile U8 T,K;
	   volatile U32 T32,TimeData=0;
	    for(T=0,K=0;T<DT->Year;T++){                    //累加年份
		    K=T&0x03;
		    TimeData+=K?31536000:31622400;
	    }
	    for(T=1;T<DT->Month;T++){
		    if(T!=2){                        //统计平月
			    T32=((T+(T>>3))&0x01)?2678400:2592000;
		    }else T32=K?2419200:2505600;                //统计闰月
		    TimeData+=T32;
	    }
	    T32=DT->Day-1;TimeData+=T32*86400;                //统计天
	    T32=DT->Hour;TimeData+=T32*3600;                //统计时
	    T32=DT->Minute;TimeData+=T32*60;                //统计分
	    TimeData+=DT->Second;                        //统计秒
	    return TimeData;
	
}

void RTC_DecodeTime(U32 TimeData, DateTime_t *DT)
{
	    U8 T,K;
	    U32 T16,K16;
	    K16=TimeData%86400;                    //分离时/分/秒
	    T16=TimeData/86400;                    //分离年/月/日
	    DT->Second=K16%60;                    //计算秒
	    K16/=60;DT->Minute=K16%60;                //计算分
	    K16/=60;DT->Hour=K16%24;                //计算时
	    for(K16=0,T=0;T<150;T++){                //按天累计年份
		    K16=(T&0x03)?365:366;                //统计闰年
		    if(T16<K16){K16-=337;break;}            //计算当年二月份天数
		    else T16-=K16;                    //减掉年份
	    }
	    DT->Year=T;                        //计算得到相对年份
	    for(T=1;T<13;T++){
		    K=(T==2)?K16:(((T+(T>>3))&0x01)+30);        //统计当月天数
		    if(T16>=K)T16-=K;                //减掉当前月
		    else 
				break;
	    }
	    DT->Month=T;                        //计算得到当年中的月份
	    DT->Day=T16+1;                        //计算得到当年中的天
}







