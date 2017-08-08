/*
 * ssc.c
 *
 * Created: 2016/3/15 14:20:43
 *  Author: JHDEV2
 */ 
#include "Interface/xnl.h"
#include "conf_board.h"

//#include "app/app.h"

#include "ssc.h"



volatile ssc_fragment_t RxBuffer[2];
volatile ssc_fragment_t TxBuffer[2];
volatile U8 BufferIndex;

volatile void (*phy_tx_exec)(void *) = NULL;
volatile void (*phy_rx_exec)(void *) = NULL;


__attribute__((__interrupt__))
static void pdca_int_handler(void)
{
	BufferIndex ^= 0x01; //Toggle Index.
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr = (U32)(&TxBuffer[BufferIndex].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = 3;  //Three words xfered each DMA.


	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr = (U32)(&RxBuffer[BufferIndex].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = 3;  //Three words xfered each DMA.
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr;

	
	if(phy_rx_exec != NULL)phy_rx_exec((void *)&RxBuffer[BufferIndex]);
	if(phy_tx_exec != NULL)phy_tx_exec((void *)&TxBuffer[BufferIndex]);
}//End of pdca_int_handler.


void local_start_SSC(void)
{
	//Before using the SSC receiver, the PIO controller must be configured to dedicate the SSC
	//receiver I/O lines to the SSC peripheral mode. [23.6.1]
	//Before using the SSC transmitter, the PIO controller must be configured to dedicate the SSC
	//transmitter I/O lines to the SSC peripheral mode. [23.6.1]
	// Assign GPIO to SSC.
	//gpio_enable_module
	//     gpio_enable_module_pin
	AVR32_GPIO.port[1].pmr0c = 0x00000DC0;
	AVR32_GPIO.port[1].pmr1c = 0x00000DC0;
	AVR32_GPIO.port[1].gperc = 0x00000DC0;

	//Software reset SSC
	(&AVR32_SSC)->cr = AVR32_SSC_CR_SWRST_MASK;

	(&AVR32_SSC)->cmr = AVR32_SSC_CMR_DIV_NOT_ACTIVE << AVR32_SSC_CMR_DIV_OFFSET;           //For Slave


	//For Slave
	(&AVR32_SSC)->tcmr =
	AVR32_SSC_TCMR_CKS_RK_CLOCK             << AVR32_SSC_TCMR_CKS_OFFSET    |
	AVR32_SSC_TCMR_CKO_INPUT_ONLY           << AVR32_SSC_TCMR_CKO_OFFSET    |
	1                                       << AVR32_SSC_TCMR_CKI_OFFSET    |
	AVR32_SSC_TCMR_CKG_NONE                 << AVR32_SSC_TCMR_CKG_OFFSET    |
	4                                       << AVR32_SSC_TCMR_START_OFFSET  |/*Detection of a falling edge on TX_FRAME_SYNC signal*/
	32                                      << AVR32_SSC_TCMR_STTDLY_OFFSET |
	63                                      << AVR32_SSC_TCMR_PERIOD_OFFSET;


	//For Slave
	(&AVR32_SSC)->tfmr =
	31                                    << AVR32_SSC_TFMR_DATLEN_OFFSET |
	0                                     << AVR32_SSC_TFMR_DATDEF_OFFSET |
	1                                     << AVR32_SSC_TFMR_MSBF_OFFSET   |
	2                                     << AVR32_SSC_TFMR_DATNB_OFFSET  |/*This field defines the number of data words to be transferred after each transfer start, which is equal to (DATNB + 1).每帧3个字的数据*/
	0                                     << AVR32_SSC_TFMR_FSLEN_OFFSET  |
	AVR32_SSC_TFMR_FSOS_INPUT_ONLY        << AVR32_SSC_TFMR_FSOS_OFFSET   |
	0                                     << AVR32_SSC_TFMR_FSDEN_OFFSET  |
	1                                     << AVR32_SSC_TFMR_FSEDGE_OFFSET;

	//For Slave
	(&AVR32_SSC)->rcmr =
	AVR32_SSC_RCMR_CKS_RK_PIN               << AVR32_SSC_RCMR_CKS_OFFSET    |
	AVR32_SSC_RCMR_CKO_INPUT_ONLY           << AVR32_SSC_RCMR_CKO_OFFSET    |
	0                                       << AVR32_SSC_RCMR_CKI_OFFSET    |
	AVR32_SSC_RCMR_CKG_NONE                 << AVR32_SSC_RCMR_CKG_OFFSET    |
	AVR32_SSC_RCMR_START_DETECT_FALLING_RF  << AVR32_SSC_RCMR_START_OFFSET  |
	0                                       << AVR32_SSC_RCMR_STOP_OFFSET   |
	32                                      << AVR32_SSC_RCMR_STTDLY_OFFSET |
	63                                      << AVR32_SSC_RCMR_PERIOD_OFFSET;

	//For Slave
	(&AVR32_SSC)->rfmr =
	31                                    << AVR32_SSC_RFMR_DATLEN_OFFSET |
	0                                     << AVR32_SSC_RFMR_LOOP_OFFSET   |
	1                                     << AVR32_SSC_RFMR_MSBF_OFFSET   |
	2                                     << AVR32_SSC_RFMR_DATNB_OFFSET  |
	0                                     << AVR32_SSC_RFMR_FSLEN_OFFSET  |
	AVR32_SSC_RFMR_FSOS_INPUT_ONLY        << AVR32_SSC_RFMR_FSOS_OFFSET   |
	1                                     << AVR32_SSC_RFMR_FSEDGE_OFFSET;


}



void local_start_PDC(void)
{
	BufferIndex = 1;

	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->idr = AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->isr;                             //Dummy read?
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mar = (U32)(&RxBuffer[0].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcr = 3;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->psr = AVR32_PDCA_PID_SSC_RX;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->marr = (U32)(&RxBuffer[1].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->tcrr = 3;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->mr = AVR32_PDCA_WORD;


	TxBuffer[0].xnl_channel.dword = XNL_IDLE;
	TxBuffer[0].payload_channel.dword[0] = PAYLOADIDLE0;
	TxBuffer[0].payload_channel.dword[1] = PAYLOADIDLE1;
	TxBuffer[1].xnl_channel.dword = XNL_IDLE;
	TxBuffer[1].payload_channel.dword[0] = PAYLOADIDLE0;
	TxBuffer[1].payload_channel.dword[1] = PAYLOADIDLE1;

	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->idr = AVR32_PDCA_RCZ_MASK | AVR32_PDCA_TRC_MASK | AVR32_PDCA_TERR_MASK; //Atomic!
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->isr;                                                                    //Dummy read?
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->mar = (U32)(&TxBuffer[0].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcr = 3;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->psr = AVR32_PDCA_PID_SSC_TX;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->marr = (U32)(&TxBuffer[1].xnl_channel.dword);
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->tcrr = 3;
	(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->mr = AVR32_PDCA_WORD;
}


void ssc_init(void)
{
		
		//Set up PB03 to watch FS.
		//Waits for radio to start making FSYNC.
		AVR32_GPIO.port[1].oderc = 0x00000002;
		AVR32_GPIO.port[1].gpers = 0x00000002;
		while ((AVR32_GPIO.port[1].pvr & 0x00000002) == 0); //Wait for FS High.
		while ((AVR32_GPIO.port[1].pvr & 0x00000002) != 0); //Wait for FS Low.
		
		
		INTC_register_interrupt(&pdca_int_handler, AVR32_PDCA_IRQ_0, AVR32_INTC_INT3);
				
		local_start_SSC();
		local_start_PDC();
		
		//Start the SSC Physical Layer.
		(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;
		(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCTX_EXAMPLE])->cr = AVR32_PDCA_TEN_MASK;
		(&AVR32_SSC)->cr = AVR32_SSC_CR_RXEN_MASK | AVR32_SSC_CR_TXEN_MASK;
		(&AVR32_PDCA.channel[PDCA_CHANNEL_SSCRX_EXAMPLE])->ier = AVR32_PDCA_RCZ_MASK;		
}

void register_tx_rx_func( void ( *tx_exec)(void *), void (*rx_exec)(void *))
{
	 phy_tx_exec = tx_exec;
	 phy_rx_exec = rx_exec;
}
