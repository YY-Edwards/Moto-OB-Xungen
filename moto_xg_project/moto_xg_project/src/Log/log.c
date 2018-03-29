/*
 * log.c
 *
 * Created: 2015/11/24 9:34:16
 *  Author: JHDEV2
 */ 
#include "log.h"

#include "string.h"
#include "stdarg.h"
#include "stdio.h"
//#include "ff11a/ff.h"

//#include "rtc/rtc.h"
#include "gpio/gpio.h"
#include "usart/usart.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define MAX_LOG_LINE_SIZE 256


xQueueHandle logQueue;

	
static void task_mylog(void * pvParameters);

static char *  PrintChar(char c, char * str)
{
	*str++ = c;
	return str;
}

static char * PrintDec(int i, char len, char * str)
{
	char * p = str;
	
	int sign = i>=0 ? 0 : 1;
	char s[10];
	memset(s, 0 , 10);
	int cnt=0;
	if(sign)
	{
		*p++ = '-';
		i = -i;
	}
	
	if(i == 0 )s[cnt++] = '0';
	
	int ten = i%10;
	while(i)
	{
		s[cnt] = ten+'0';
		cnt++;
		i /= 10;
		ten = i%10;
	}
	
	if(len - cnt > 0 )
	{
		char num = len - cnt;
		for(int i = 0; i < num; i++)
		{
			s[cnt++] = '0';
		}
	}
	
	while(cnt>0)
	*p++ = s[--cnt];
	
	return str;
}

static char * PrintHex(int i,char len, char * str)
{
	char * p = str;
	char s[12];
	memset(s, 0 , 12);
	int cnt=0;
	
	if(i)
	while(i)
	{
		s[cnt++] = ((i & 0x0F) >= 0x0A) ? ((i & 0x0F) + 'A' - 10) :  ((i & 0x0F)  + '0');
		i = (i >> 4 ) & 0x0FFFFFFF;
		//i >>= 4
	}
	else
	s[cnt++] = '0';
	
	if(len - cnt > 0 )
	{
		char num = len - cnt;
		for(int i = 0; i < num; i++)
		{
			s[cnt++] = '0';
		}
	}
	
	s[cnt++] = 'x';
	s[cnt++] = '0';
	
	while(cnt>0)
	*p++ = s[--cnt];
		
	return str;
}

void log_init(void)
{
	static const gpio_map_t USART_GPIO_MAP =
	{
		{EXAMPLE_USART_RX_PIN, EXAMPLE_USART_RX_FUNCTION},
		{EXAMPLE_USART_TX_PIN, EXAMPLE_USART_TX_FUNCTION}
	};

	// USART options.
	static const usart_options_t USART_OPTIONS =
	{
		.baudrate     = 19200,//115200,
		.charlength   = 8,
		.paritytype   = USART_NO_PARITY,
		.stopbits     = USART_1_STOPBIT,
		.channelmode  = USART_NORMAL_CHMODE
	};

	// Assign GPIO to USART.
	gpio_enable_module(USART_GPIO_MAP,
	sizeof(USART_GPIO_MAP) / sizeof(USART_GPIO_MAP[0]));

	// Initialize USART in RS232 mode.
	usart_init_rs232(EXAMPLE_USART, &USART_OPTIONS, EXAMPLE_TARGET_PBACLK_FREQ_HZ);			
			
	logQueue  = xQueueCreate(256, sizeof(char *));
	
	xTaskCreate(
	task_mylog
	,  (const signed portCHAR *)"LOG"
	,  100//384
	,  NULL
	,  tskLOG_PRIORITY
	,  NULL );
	
}

int mylog(char * content, ...)
{
	
	
	char logTmp[MAX_LOG_LINE_SIZE];
	memset(logTmp, '\0', MAX_LOG_LINE_SIZE);	
	
	int len = 0 ;
	

	va_list arg_ptr;
	char* str = content;
	int x;
	char y;
//	float f;
	//char* s;
	char length= 0;
		
	va_start(arg_ptr, content);
		
	//if(memcmp(content, "[-t]", 4) == 0)
	//{
		//DateTime_t t;
		//rtc_readTime(&t);
		//sprintf(logTmp,"[%04d%02d%02d,%02d%02d%02d] ", t.year,t.month,t.day,t.hour,t.minute,t.second);
		//str += 4;
	//}	
			
	do
	{
		char strBuf[MAX_LOG_LINE_SIZE], *strTmp = strBuf;
		memset(strTmp, 0, MAX_LOG_LINE_SIZE);
			
					if(*str == '%')
					{
						static char lench = 0;
						lench =*(str + 1);
						if((lench >= '0') &&  (lench <= '9'))
						{
							len = *(++str) - '0';
						}
						
						str++;
						
						switch(*(str))
						{
							case('i'):
							case('I'):
							case('d'):
							case('D'):
							x = va_arg(arg_ptr,int);
							strTmp = PrintDec(x, len, strTmp);
							break;
							
							case('f')://¸¡µãÊýÊä³ö
							case('F'):
							//f = va_arg(arg_ptr,int);
							//strTmp = PrintDec(x, len, strTmp);
							mylog("I need float.");
							break;
							
							
							case('x'):
							case('X'):
							x = va_arg(arg_ptr,int);
							PrintHex(x,len, strTmp);
							break;
							
							case('c'):
							case('C'):
							y = va_arg(arg_ptr,int);
							PrintChar(y, strTmp);
							break;
							
							case('s'):
							case('S'):
							strTmp = va_arg(arg_ptr,char*);
							//PrintStr(s);
							break;
							
							case('%'):
							PrintChar('%', strTmp);
							//PrintChar('%');
							break;
							
							default:
							mylog("I need relax.");
						}
						str++;
						
						length = sprintf(logTmp,"%s%s",logTmp,strTmp );
					}
					//else if(*str == '[')
					//{
						//if(memcmp(str, "[-t]", 4) == 0)
						//{
							//DateTime_t t;
							//rtc_readTime(&t);
							//sprintf(logTmp,"%s[%04d-%02d-%02d %02d:%02d:%02d]  ",logTmp, t.year,t.month,t.day,t.hour,t.minute,t.second);
							//str += 4;
						//}
						//else
						//length = sprintf(logTmp,"%s%c",logTmp, *str++);
					//}
					else
					length = sprintf(logTmp,"%s%c",logTmp, *str++);
			
			
			
	}while(*str != '\0');
		
		
				
	va_end(arg_ptr);
	//while(1)		
	length = sprintf(logTmp,"%s\r\n",logTmp);
	
	char * p = pvPortMalloc(length+1);
	memcpy(p, logTmp, length+1);
	//
	//usart_write_line(EXAMPLE_USART, logTmp)
	
	xQueueSend( logQueue, &p, 5);
	
	return 0;
}
	
	
int logFromISR(char * content, ...)
	{
		
		
		char logTmp[MAX_LOG_LINE_SIZE];
		memset(logTmp, '\0', MAX_LOG_LINE_SIZE);
		
		int len = 0 ;
		

		va_list arg_ptr;
		char* str = content;
		int x;
		char y;
		//char* s;
		char length= 0;
		
		va_start(arg_ptr, content);
		
		//if(memcmp(content, "[-t]", 4) == 0)
		//{
		//DateTime_t t;
		//rtc_readTime(&t);
		//sprintf(logTmp,"[%04d%02d%02d,%02d%02d%02d] ", t.year,t.month,t.day,t.hour,t.minute,t.second);
		//str += 4;
		//}
		
		do
		{
			char strBuf[MAX_LOG_LINE_SIZE], *strTmp = strBuf;
			memset(strTmp, 0, MAX_LOG_LINE_SIZE);
			
			if(*str == '%')
			{
				static char lench = 0;
				lench =*(str + 1);
				if((lench >= '0') &&  (lench <= '9'))
				{
					len = *(++str) - '0';
				}
				
				str++;
				
				switch(*(str))
				{
					case('d'):
					case('D'):
					x = va_arg(arg_ptr,int);
					strTmp = PrintDec(x, len, strTmp);
					break;
					case('x'):
					case('X'):
					x = va_arg(arg_ptr,int);
					PrintHex(x,len, strTmp);
					break;
					case('c'):
					case('C'):
					y = va_arg(arg_ptr,int);
					PrintChar(y, strTmp);
					break;
					case('s'):
					case('S'):
					strTmp = va_arg(arg_ptr,char*);
					//PrintStr(s);
					break;
					case('%'):
					PrintChar('%', strTmp);
					//PrintChar('%');
					break;
					default:
					mylog("I need relax.");
				}
				str++;
				
				length = sprintf(logTmp,"%s%s",logTmp,strTmp );
			}
			//else if(*str == '[')
			//{
			//if(memcmp(str, "[-t]", 4) == 0)
			//{
			//DateTime_t t;
			//rtc_readTime(&t);
			//sprintf(logTmp,"%s[%04d-%02d-%02d %02d:%02d:%02d]  ",logTmp, t.year,t.month,t.day,t.hour,t.minute,t.second);
			//str += 4;
			//}
			//else
			//length = sprintf(logTmp,"%s%c",logTmp, *str++);
			//}
			else
			length = sprintf(logTmp,"%s%c",logTmp, *str++);
			
			
			
		}while(*str != '\0');
		
		
		
		va_end(arg_ptr);
		//while(1)
		length = sprintf(logTmp,"%s\r\n",logTmp);
		
		char * p = pvPortMalloc(length+1);
		memcpy(p, logTmp, length+1);
		//
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		
		//usart_write_line(EXAMPLE_USART, p);
		//vPortFree(p);
		xQueueSendFromISR( logQueue, &p, &xHigherPriorityTaskWoken );
		
		
		//if (xHigherPriorityTaskWoken == pdTRUE)
		//{
			////taskYIELD();
		//}
	return 0;
	}
//portTickType log_water_value =0;	
static void task_mylog(void * pvParameters)
{
	char * str;
	//char str[MAX_LOG_LINE_SIZE];
	for(;;)
	{
		//log_water_value = uxTaskGetStackHighWaterMark(NULL);
		if(xQueueReceive( logQueue, &str, (2000*2) / portTICK_RATE_MS) == pdTRUE )
		{
			if( NULL != str)
			{
				usart_write_line(EXAMPLE_USART, str);
				vPortFree(str);
			}
			
		}

	}

}


