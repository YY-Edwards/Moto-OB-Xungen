/**
Copyright (C), Jihua Information Tech. Co., Ltd.

File name: ssc.c
Author: wxj
Version: 1.0.0.01
Date: 2016/3/15 14:20:43

Description: 
History:
*/
#include "string.h"
#include "conf_board.h"
#include "ssc.h"
#include "ambe.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "gpio.h"
#include "task.h"


#define SSI_SYNC_GPIO AVR32_PIN_PB11
#define FASTRUN __attribute__ ((section (".fastrun")))
xSemaphoreHandle ssi_sync_semphr = NULL;

volatile U32 intStartCount;
volatile U32 intDuration;

volatile U8 BufferIndex; // Index is used to toggle send/receive buffer


U16 TxIdle[] = {
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000
};

/*Set the PDCA data channel address to rend/receive SSC data*/
volatile unsigned char RxBuffer[2][480];
volatile unsigned char TxBuffer[2][480];

/*
Defines the interface function (callback function) is used to send/receive SSC 
data*/
 void (*phy_rx_exec)(void *) = NULL;
 void (*phy_tx_exec)(void *) = NULL;
//extern volatile  xSemaphoreHandle xBinarySemaphore;





#if __GNUC__
// GCC-specific syntax to declare an interrupt handler. The interrupt handler
// registration is done in the main function using the INTC software driver module.
__attribute__((__interrupt__))
#elif __ICCAVR32__
// IAR-specific syntax to declare and register an interrupt handler.
// Register to the interrupt group 0(cf Section "Interrupt Request Signal Map"
// in the datasheet) with interrupt level 0.
#pragma handler = AVR32_CORE_IRQ_GROUP, 0
__interrupt
#endif
void GPIOHandler(void)
{
	//enable the ssi interrupt, timer interrupt
	//quit sleep mode
	//re-start the timer for detect the GPIO

	gpio_get_pin_interrupt_flag(SSI_SYNC_GPIO);
	gpio_clear_pin_interrupt_flag(SSI_SYNC_GPIO);
	gpio_disable_pin_interrupt(SSI_SYNC_GPIO);

	//send semaphore to XNL task for SYNC
	taskENTER_CRITICAL();
	xSemaphoreGiveFromISR(ssi_sync_semphr, pdFALSE);
	taskEXIT_CRITICAL();
}


/**
Function: pdca_int_handler
Description: interrupt service routine for PDCA （SSC）
Calls: 
    void (*phy_tx_exec)(void *)--callback function for send SSC data 
    void (*phy_rx_exec)(void *)--callback function for receive SSC data 
Called By: interrupt
*/
__attribute__((__interrupt__))
FASTRUN void pdca_int_handler(void)
{
    
	//static portBASE_TYPE xHigherPriorityTaskWoken;
	//xHigherPriorityTaskWoken = pdFALSE;
	
	static U32 count=0;
	//intStartCount = Get_system_register(AVR32_COUNT);
	
	count++;
	/*Toggle Index*/
	//U8 temp = BufferIndex;
	BufferIndex ^= 0x01;
	
	/*Software reset PDCA */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr 
                              = (U32)(TxBuffer[BufferIndex]);

    /*Three words xfered each DMA.*/
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = 120;

    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr 
                              = (U32)(RxBuffer[BufferIndex]);

    /*Three words xfered each DMA.*/
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = 120;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr;
	
	/*receive SSC data*/
	
	if(phy_rx_exec != NULL)for(int i=0; i < 40;++i)
	{
		void  * p = RxBuffer[BufferIndex] + i * 12;
		phy_rx_exec(p);//phy_rx_func
	}
    

    /*transmit SSC data*/
	if(phy_tx_exec != NULL)for(int i =0; i< 40; ++i)
	{
		void  * p = TxBuffer[BufferIndex] + i * 12;
		phy_tx_exec(p);//phy_tx_func
	}

	//if(count%20000 == 0)
	{
		/* 'Give' the semaphore to unblock the task. */
		//xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken );
	}
	
		///*****测试：payload通道上把payload-RX数据直接回发*******/
		//TxBuffer[BufferIndex].payload_channel.dword[0] = RxBuffer[BufferIndex].payload_channel.dword[0];
		//TxBuffer[BufferIndex].payload_channel.dword[1] = RxBuffer[BufferIndex].payload_channel.dword[1];
		/************/

	
	//intDuration = Get_system_register(AVR32_COUNT) - intStartCount;
	
	
}/*End of pdca_int_handler.*/

/**
Function: local_start_SSC
Description: Before using the SSC receiver, the PIO controller must be 
    configured to dedicate the SSC ,receiver I/O lines to the SSC peripheral 
    mode. [23.6.1]
    Before using the SSC transmitter, the PIO controller must be configured to 
    dedicate the SSC,transmitter I/O lines to the SSC peripheral mode. [23.6.1]
Called By: void ssc_init(void) -- ssc.c
*/
static void local_start_SSC(void)
{
    /*Assign GPIO to SSC.
    gpio_enable_module
    gpio_enable_module_pin*/
    AVR32_GPIO.port[1].pmr0c = 0x00000DC0;
    AVR32_GPIO.port[1].pmr1c = 0x00000DC0;
    AVR32_GPIO.port[1].gperc = 0x00000DC0;

    /*Software reset SSC*/
    (&AVR32_SSC)->cr = AVR32_SSC_CR_SWRST_MASK;
    (&AVR32_SSC)->cmr 
                     = AVR32_SSC_CMR_DIV_NOT_ACTIVE << AVR32_SSC_CMR_DIV_OFFSET;

    /*For Slave*/
    (&AVR32_SSC)->tcmr =
          AVR32_SSC_TCMR_CKS_RK_CLOCK		<< AVR32_SSC_TCMR_CKS_OFFSET	| 
		  AVR32_SSC_TCMR_CKO_INPUT_ONLY		<< AVR32_SSC_TCMR_CKO_OFFSET	| 
		  1									<< AVR32_SSC_TCMR_CKI_OFFSET	| 
		  AVR32_SSC_TCMR_CKG_NONE			<< AVR32_SSC_TCMR_CKG_OFFSET	|	

        /*Detection of a falling edge on TX_FRAME_SYNC signal*/
		  4									<< AVR32_SSC_TCMR_START_OFFSET	|
	      32								<< AVR32_SSC_TCMR_STTDLY_OFFSET	|
	      63								<< AVR32_SSC_TCMR_PERIOD_OFFSET;


    /*For Slave*/
    (&AVR32_SSC)->tfmr = 
		31									<< AVR32_SSC_TFMR_DATLEN_OFFSET	|
        0									<< AVR32_SSC_TFMR_DATDEF_OFFSET	|
	    1									<< AVR32_SSC_TFMR_MSBF_OFFSET	|

        /*This field defines the number of data words to be transferred after 
        each transfer start, which is equal to (DATNB + 1).*/
	     2									<< AVR32_SSC_TFMR_DATNB_OFFSET	|
	     0									<< AVR32_SSC_TFMR_FSLEN_OFFSET	|
	     AVR32_SSC_TFMR_FSOS_INPUT_ONLY		<< AVR32_SSC_TFMR_FSOS_OFFSET	|
	     0									<< AVR32_SSC_TFMR_FSDEN_OFFSET	|
	     1									<< AVR32_SSC_TFMR_FSEDGE_OFFSET;

	/*For Slave*/

	(&AVR32_SSC)->rcmr = 
		 AVR32_SSC_RCMR_CKS_RK_PIN			<< AVR32_SSC_RCMR_CKS_OFFSET	|
	     AVR32_SSC_RCMR_CKO_INPUT_ONLY		<< AVR32_SSC_RCMR_CKO_OFFSET	|
	     0									<< AVR32_SSC_RCMR_CKI_OFFSET	|
	     AVR32_SSC_RCMR_CKG_NONE			<< AVR32_SSC_RCMR_CKG_OFFSET	|
	     AVR32_SSC_RCMR_START_DETECT_FALLING_RF << AVR32_SSC_RCMR_START_OFFSET|
	     0									<< AVR32_SSC_RCMR_STOP_OFFSET	|
	     32									<< AVR32_SSC_RCMR_STTDLY_OFFSET	|
	     63									<< AVR32_SSC_RCMR_PERIOD_OFFSET;

	/*For Slave*/
	(&AVR32_SSC)->rfmr = 
		31									<< AVR32_SSC_RFMR_DATLEN_OFFSET	|
	    0									<< AVR32_SSC_RFMR_LOOP_OFFSET	|
	    1									<< AVR32_SSC_RFMR_MSBF_OFFSET	|
	    2									<< AVR32_SSC_RFMR_DATNB_OFFSET	|
	    0									<< AVR32_SSC_RFMR_FSLEN_OFFSET	|
	    AVR32_SSC_RFMR_FSOS_INPUT_ONLY		<< AVR32_SSC_RFMR_FSOS_OFFSET	|
	    1									<< AVR32_SSC_RFMR_FSEDGE_OFFSET;
}/*End of local_start_SSC.*/

/*
Function: local_start_PDC
Description: none
Called By: void ssc_init(void) -- ssc.c
*/
static void local_start_PDC(void)
{
    /*Toggle Index*/	
    BufferIndex = 1;
	
	memset(RxBuffer, 0, 960);
	for(int i = 0; i < 80; ++i)
	{
		memcpy(TxBuffer[0] + i * 12 , TxIdle, 12);
	}
	
	//TxBuffer[0].xnl_channel.dword = XNL_IDLE;
	//TxBuffer[0].payload_channel.dword[0] = PAYLOADIDLE0;
	//TxBuffer[0].payload_channel.dword[1] = PAYLOADIDLE1;
	//TxBuffer[1].xnl_channel.dword = XNL_IDLE;
	//TxBuffer[1].payload_channel.dword[0] = PAYLOADIDLE0;
	//TxBuffer[1].payload_channel.dword[1] = PAYLOADIDLE1;
	
	
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->idr = 
               AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr; 
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mar = 
                                          (U32)(RxBuffer[0]);
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcr = 120;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->psr = 
                                                          AVR32_PDCA_PID_SSC_RX;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr = 
                                          (U32)(RxBuffer[1]);
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = 120;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mr = AVR32_PDCA_WORD;

	

	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->idr = 
               AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->isr;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->mar = 
                                          (U32)(TxBuffer[0]);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcr = 120;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->psr = AVR32_PDCA_PID_SSC_TX;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr = 
                                             (U32)(TxBuffer[1]);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = 120;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->mr = AVR32_PDCA_WORD;
}/*End of local_start_PDC.*/


static void ssc_sync_gpio_init(void)
{
	ssi_sync_semphr = xSemaphoreCreateCounting(1,0);//信号量

	gpio_enable_pin_interrupt(SSI_SYNC_GPIO, GPIO_RISING_EDGE);//上升沿中断
	Disable_global_interrupt();
	INTC_register_interrupt( &GPIOHandler, AVR32_GPIO_IRQ_0 + (SSI_SYNC_GPIO/8), AVR32_INTC_INT1);
	//Enable_global_interrupt();

}

/*
Function: ssc_init
Description: Initialize the SSC and PDCA 
Calls: 
    INTC_register_interrupt -- intc.c
    local_start_SSC -- ssc.c
    local_start_PDC -- ssc.c
Called By: phy_init -- physical.c
*/
void ssc_init(void)
{		
    ///*Set up PB01 to watch FS.*/不需要观察了吗？
    //AVR32_GPIO.port[1].oderc = 0x00000002;
    //AVR32_GPIO.port[1].gpers = 0x00000002;
	
	
	ssc_sync_gpio_init();//用GPIO中断观察并触发SSC初始化
	
	Disable_global_interrupt();
	
	INTC_register_interrupt (
	&pdca_int_handler
	, AVR32_PDCA_IRQ_0 //PDCA_CHANNEL_SSCRX_EXAMPLE = 0
	, AVR32_INTC_INT3 //highest priority.
	);
	
	//Enable_global_interrupt();
	//
    ///*Waits for radio to start making FSYNC.*/
    //while ((AVR32_GPIO.port[1].pvr & 0x00000002) == 0); //Wait for FS High.
    //while ((AVR32_GPIO.port[1].pvr & 0x00000002) != 0); //Wait for FS Low.
			//
	//Disable_global_interrupt(); // resume to before
				//
    ///*config the SSC*/
    //local_start_SSC();
//
    ///*config the PDCA*/
    //local_start_PDC();
		//
    ///*Start the SSC Physical Layer.*/
//
    //(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;
    //(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;
    //(&AVR32_SSC)->cr = AVR32_SSC_CR_RXEN_MASK | AVR32_SSC_CR_TXEN_MASK;
    //(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->ier = AVR32_PDCA_RCZ_MASK;
															//
	//Enable_global_interrupt();
															
}/*End of ssc_init.*/


void sync_ssi(void)
{
	//get sema ,and enable ssc
	xSemaphoreTake( ssi_sync_semphr, portMAX_DELAY );
	/*config the SSC*/
	local_start_SSC();
	/*config the PDCA*/
	local_start_PDC();
	   
	/*Start the SSC Physical Layer.*/
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;//使能PDCA_RX传输
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;//使能PDCA_TX传输
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->ier = AVR32_PDCA_RCZ_MASK;//开启PDCA_RX的重载计数为空中断
	(&AVR32_SSC)->cr = AVR32_SSC_CR_RXEN_MASK | AVR32_SSC_CR_TXEN_MASK;//使能SSC传输

}



/*
Function: register_rx_tx_func
Parameters: 
    void (*rx_exec)(void *) -- receive ssc function
	void ( *tx_exec)(void *) -- send ssc function
Description: register the rx function(callback function)
Called By: phy_init -- physical.c
*/
void register_rx_tx_func(void (*rx_exec)(void *),  void ( *tx_exec)(void *))
{
	 phy_rx_exec = rx_exec;
	 phy_tx_exec = tx_exec;//phy_tx_func, phy_rx_func
	 
}/*End of register_tx_rx_func.*/
