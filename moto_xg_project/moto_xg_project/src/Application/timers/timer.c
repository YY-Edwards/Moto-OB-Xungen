/*
 * timer.c
 *
 * Created: 2016/3/15 14:06:41
 *  Author: JHDEV2
 */ 

#include <stdbool.h>

#include "conf_example.h"
#include "tc.h"

#include "intc.h"

#include "../log/log.h"

//#include "Interface/xcmp.h"


volatile U32 tc_tick = 0;

//brief Default interrupt handler.
__attribute__((__interrupt__))
static void _tc_interrupt(void)
 {
	// Increment the 200ms seconds counter
	tc_tick++;
	/*
	 * TODO: Place a breakpoint here and watch the update of tc_tick variable
	 * in the Watch Window.
	 */
	// Clear the interrupt flag. This is a side effect of reading the TC SR.
	tc_read_sr(EXAMPLE_TC, EXAMPLE_TC_CHANNEL);
	
}


//	SSC Related I/O
//	MAKO_DCLK				AVR32_SSC_TX_CLOCK_0_PIN		NU	[41 PortB Pin  9 00000200 Func 0]
//							AVR32_SSC_RX_CLOCK_0_PIN			[38 PortB Pin  6 00000040 Func 0]
//	Timer Clk				AVR32_TC_CLK0_0_1_PIN				[20 PortA Pin 20 00100000 Func 1]
//	MAKO_FSYNC				AVR32_SSC_TX_FRAME_SYNC_0_PIN		[43 PortB Pin 11 00000800 Func 0]
//							AVR32_SSC_RX_FRAME_SYNC_0_PIN		[40 PortB Pin  8 00000100 Func 0]
//	Timer Sync				AVR32_TC_B0_0_0_PIN					[33 PortB Pin  1 00000002 Func 0]
//	SSC_TX_DATA_ENABLE		AVR32_TC_A0_0_0_PIN					[32 PortB Pin  0 00000001 Func 0]
//	MAKO_TX					AVR32_SSC_TX_DATA_0_PIN				[42 PortB Pin 10 00000400 Func 0]
//	MAKO_RX					AVR32_SSC_RX_DATA_0_PIN				[39 PortB Pin  7 00000080 Func 0]
//
void local_start_timer(void)
{
	//Route CLK to Timer
	AVR32_GPIO.port[0].pmr0s = 0x00100000;
	AVR32_GPIO.port[0].pmr1c = 0x00100000;
	AVR32_GPIO.port[0].gperc = 0x00100000;
	//Route FS and Tri-State to Timer.
	AVR32_GPIO.port[1].pmr0c = 0x00000003;
	AVR32_GPIO.port[1].pmr1c = 0x00000003;
	AVR32_GPIO.port[1].gperc = 0x00000003;

	(&AVR32_TC)->bmr = 4;
	(&AVR32_TC)->channel[0].cmr =
	AVR32_TC_BSWTRG_NONE       << AVR32_TC_BSWTRG_OFFSET   |
	AVR32_TC_BEEVT_NONE        << AVR32_TC_BEEVT_OFFSET    |
	AVR32_TC_BCPC_NONE         << AVR32_TC_BCPC_OFFSET     |
	AVR32_TC_BCPB_NONE         << AVR32_TC_BCPB_OFFSET     |
	AVR32_TC_ASWTRG_SET        << AVR32_TC_ASWTRG_OFFSET   |
	AVR32_TC_AEEVT_SET         << AVR32_TC_AEEVT_OFFSET    |
	AVR32_TC_ACPC_NONE         << AVR32_TC_ACPC_OFFSET     |
	AVR32_TC_ACPA_CLEAR        << AVR32_TC_ACPA_OFFSET     |
	1                          << AVR32_TC_WAVE_OFFSET     |
	AVR32_TC_WAVSEL_UP_NO_AUTO << AVR32_TC_WAVSEL_OFFSET   |
	1                          << AVR32_TC_ENETRG_OFFSET   |
	AVR32_TC_EEVT_TIOB_INPUT   << AVR32_TC_EEVT_OFFSET     |
	AVR32_TC_EEVTEDG_POS_EDGE  << AVR32_TC_EEVTEDG_OFFSET  |
	0                          << AVR32_TC_CPCDIS_OFFSET   |
	0                          << AVR32_TC_CPCSTOP_OFFSET  |
	AVR32_TC_BURST_NOT_GATED   << AVR32_TC_BURST_OFFSET    |
	1                          << AVR32_TC_CLKI_OFFSET     |
	AVR32_TC_TCCLKS_XC0        << AVR32_TC_TCCLKS_OFFSET;



	(&AVR32_TC)->channel[0].ra = 32;
	(&AVR32_TC)->channel[0].ccr = AVR32_TC_SWTRG_MASK | AVR32_TC_CLKEN_MASK;
}



/**
 * \brief TC Initialization
 *
 * Initializes and start the TC module with the following:
 * - Counter in Up mode with automatic reset on RC compare match.
 * - fPBA/8 is used as clock source for TC
 * - Enables RC compare match interrupt
 * \param tc Base address of the TC module
 */
void tc_init()
{

	volatile avr32_tc_t * tc = EXAMPLE_TC;
	
	INTC_register_interrupt(&_tc_interrupt, AVR32_TC_IRQ1, AVR32_INTC_INT2);

	// Options for waveform generation.
	static const tc_waveform_opt_t waveform_opt = {
		// Channel selection.
		.channel  = EXAMPLE_TC_CHANNEL,
		// Software trigger effect on TIOB.
		.bswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOB.
		.beevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOB.
		.bcpc     = TC_EVT_EFFECT_NOOP,
		// RB compare effect on TIOB.
		.bcpb     = TC_EVT_EFFECT_NOOP,
		// Software trigger effect on TIOA.
		.aswtrg   = TC_EVT_EFFECT_NOOP,
		// External event effect on TIOA.
		.aeevt    = TC_EVT_EFFECT_NOOP,
		// RC compare effect on TIOA.
		.acpc     = TC_EVT_EFFECT_NOOP,
		/* RA compare effect on TIOA.
		 * (other possibilities are none, set and clear).
		 */
		.acpa     = TC_EVT_EFFECT_NOOP,
		/* Waveform selection: Up mode with automatic trigger(reset)
		 * on RC compare.
		 */
		.wavsel   = TC_WAVEFORM_SEL_UP_MODE_RC_TRIGGER,
		// External event trigger enable.
		.enetrg   = false,
		// External event selection.
		.eevt     = 0,
		// External event edge selection.
		.eevtedg  = TC_SEL_NO_EDGE,
		// Counter disable when RC compare.
		.cpcdis   = false,
		// Counter clock stopped with RC compare.
		.cpcstop  = false,
		// Burst signal selection.
		.burst    = false,
		// Clock inversion.
		.clki     = false,
		// Internal source clock 3, connected to fPBA / 8.
		.tcclks   = TC_CLOCK_SOURCE_TC3
	};

	// Options for enabling TC interrupts
	static const tc_interrupt_t tc_interrupt = {
		.etrgs = 0,
		.ldrbs = 0,
		.ldras = 0,
		.cpcs  = 1, // Enable interrupt on RC compare alone
		.cpbs  = 0,
		.cpas  = 0,
		.lovrs = 0,
		.covfs = 0
	};
	// Initialize the timer/counter.
	tc_init_waveform(tc, &waveform_opt);

	/*
	 * Set the compare triggers.
	 * We configure it to count every 10 milliseconds.
	 * We want: (1 / (fPBA / 8)) * RC = 10 ms, hence RC = (fPBA / 8) / 100
	 * to get an interrupt every 10 ms.
	 */
	//tc_write_rc(tc, EXAMPLE_TC_CHANNEL, (sysclk_get_pba_hz() / 8 / 100));
	
	tc_write_rc(tc, EXAMPLE_TC_CHANNEL, ((FOSC0*2) / 8 / 2000));//200ms
	
	//tc_write_rc(tc, EXAMPLE_TC_CHANNEL, (FOSC0 / 8 / 100000));
	
	
	// configure the timer interrupt
	tc_configure_interrupts(tc, EXAMPLE_TC_CHANNEL, &tc_interrupt);
	// Start the timer/counter.
	tc_start(tc, EXAMPLE_TC_CHANNEL);
}