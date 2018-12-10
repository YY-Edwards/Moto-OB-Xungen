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
#include "timer.h"


#define SSI_SYNC_GPIO AVR32_PIN_PB11
#define FASTRUN __attribute__ ((section (".fastrun")))
xSemaphoreHandle ssi_sync_semphr = NULL;
extern volatile bool is_writing;
extern xSemaphoreHandle modem_semphr;

volatile U32 intStartCount = 0;
volatile U32 intDuration = 0;

volatile U8 BufferIndex; // Index is used to toggle send/receive buffer


U16 TxIdle[DMA_BUF_HALF_WORD_SIZE] = {
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000,
	0xABCD, 0x5A5A,
	0xABCD, 0x5A5A,
	0x0000, 0x0000};

///*Set the PDCA data channel address to rend/receive SSC data*/
//volatile unsigned char RxBuffer[2][480];
//volatile unsigned char TxBuffer[2][480];

/*
 * Define one Tx buffer queue and one Rx buffer queue:
 * 16 frames for Tx DMA and 16 frames for Rx DMA, with each size 60 words.
 */
volatile U16 RxBuffer[DMABUFNUM][DMA_BUF_HALF_WORD_SIZE];//16*60*2 = 1920bytes
volatile U16 TxBuffer[DMABUFNUM][DMA_BUF_HALF_WORD_SIZE];

/*
 * Define 3 indexes to record DMA frame transfer status:
 * Which tx/rx buffer has just been transferred;
 * Which tx/rx buffer is now being transferred;
 * Which tx/rx buffer will be transferred next.
 */
volatile static U8 previous_index = DMABUFNUM - 1;
volatile static U8 dma_buffer_index = 0;
volatile static U8 next_index = 1;

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
/*
--------------------------- DETAILED DESCRIPTION ------------------------------
*
* Each time when this interrupt handler is called, it's the time that
* 1. RxBuffer[dma_buffer_index] and TxBuffer[dma_buffer_index] has just been
* Transferred;
* 2. Transfer of RxBuffer[next_index] and TxBuffer[next_index] just began.
* 3. DMA Transfer address and size register has just been cleared.
*
*******************************************************************************/

volatile U32 intStartCount;
volatile U32 intDuration;
__attribute__((__interrupt__))
FASTRUN void pdca_int_handler(void)
{
    U32 addr;
	U16 i;
	static U8 int_counter = 0;
	static portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;	
	//static U32 count=0;
	intStartCount = get_system_time();//Get_system_register(AVR32_COUNT);

	/*Toggle Index*/
	
	/* Fill the buffer just sent with idle frame */
	//将tx_buff已传输完毕的数据全部再次重置为空闲，貌似不用这样的操作。待测试
	//for (i = 0; i < DMA_BUF_HALF_WORD_SIZE; i++)
	//{
		//TxBuffer[dma_buffer_index][i] = TxIdle[i];
	//}
	
	/* update buffer index record */	
	previous_index = (previous_index + 1) % DMABUFNUM;//已传输完成的索引
	dma_buffer_index = (dma_buffer_index + 1) % DMABUFNUM;//当前正在传输的索引
	next_index = (next_index + 1) % DMABUFNUM;//下一个等待传输的索引
	
	/*Software reset PDCA */
	/* Update Rx address and size for next transfer */
	addr = (U32)&(RxBuffer[next_index]);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr = addr;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = DMA_BUF_WORD_SIZE;
	
	 /* Update Tx address and size for next transfer */
	addr = (U32)&(TxBuffer[next_index]);
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr = addr;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = DMA_BUF_WORD_SIZE;
	
	
	if(phy_rx_exec != NULL)
	{
		void  * p = RxBuffer[previous_index];//处理已传输完成的rx_buff
		phy_rx_exec(p);//phy_rx_func
	}
	if(phy_tx_exec != NULL)
	{
		void  * p = TxBuffer[next_index];//处理下一个即将传输的tx_buff：即应该将即将需要传输的数据拷贝到此地址上
		phy_tx_exec(p);//phy_tx_func
	}
	
	
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr;//clear isr
	
	int_counter++;
	if (5 == int_counter) //hanle slower than before:5ms
	{
		if (!is_writing)
		{
			portENTER_CRITICAL();
			xSemaphoreGiveFromISR(modem_semphr, 0 );
			portEXIT_CRITICAL();
		}
		int_counter = 0;
	}
		
	
	intDuration = get_system_time() - intStartCount;
	
	
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

	
	U16 i = 0, j=0;
	
	/* Clear RxBuffer and set TxBuffer to idle frame. */
	for(j = 0; j < DMABUFNUM; j++)
	{
		for (i = 0; i < DMA_BUF_HALF_WORD_SIZE; i++)
		{
			RxBuffer[j][i] = 0;
			TxBuffer[j][i] = TxIdle[i];
		}
	}
	
	dma_buffer_index = 0;
	//disable interrupt
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->idr =  AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK;
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr;							//clear interrupt
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mar = (U32)(RxBuffer[0]);	/* 1st RxBuffer address */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcr = DMA_BUF_WORD_SIZE;			/* 1st transfer size:30 */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->psr = AVR32_PDCA_PID_SSC_RX; /* set DMA source to SSC RX */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr = (U32)(RxBuffer[1]);   /* next RxBuffer address */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = DMA_BUF_WORD_SIZE;			/* 2nd transfer size:30 */
    (&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mr = AVR32_PDCA_WORD;		//data size:4bytes. 
																						//transfer bytes:4*30=120bytes,12bytes per 125us,
																						//then,it takes 10*125us=1.25ms/per interrupt.	

	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->idr = AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->isr;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->mar = (U32)(TxBuffer[0]);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcr = DMA_BUF_WORD_SIZE;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->psr = AVR32_PDCA_PID_SSC_TX;/* set DMA source to SSC TX */
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr =  (U32)(TxBuffer[1]);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = DMA_BUF_WORD_SIZE;
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
