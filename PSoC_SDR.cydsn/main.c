/******************************************************************************
* Project Name		: PSoC_SDR
* File Name			: main.c
* Version 			: 1.0
* Device Used		: CY8C5888LTI-LP097
* Software Used		: PSoC Creator 3.1 SP2
* Compiler    		: ARM GCC 4.8.4, ARM RVDS Generic, ARM MDK Generic
* Related Hardware	: CY8CKIT059 PSoC 5 LP Prototyping Kit 
* Owner				: G. Polder, PA3BYA
*
********************************************************************************/

#include <device.h>
#include "main.h"



//----------------------------------------------------------------------------
//
//	Global variables
//
//----------------------------------------------------------------------------
uint8	Freq [8];					// receive frequency in ASCII BCD
uint8	LastFreq [8];				// copy of last tuned rx frequency
int32	Frequency;					// receive frequency in binary
int32	FrequencyError;				// frequency error at 10 MHz

uint8	ComRxCmd [32];				// remote command rx buffer
uint8	ComRxPtr;					// pointer into com rx buffer
uint8	UsbTxBuffer [64];			// USB transmit buffer
uint8	UsbRxBuffer [256];			// USB receive buffer

uint8	AdcTick;					// ADC interrupt flag every 6.521 msec
int16	IDCOffset, QDCOffset;		// DC offset values for ADC input
int16	AdcPeak, AdcPeakAve;		// ADC input peak value, and its average
uint8	AgcDisable;					// Agc disable flag
uint8	AgcGain;					// AGC for ADC input
uint8	AgcTimer;					// AGC processing timer
uint8	AgcIntegrator;				// AGC integrator

uint8	LedTimer;					// LED on timer
uint8	DisplayIndex;				// index for status display

uint8	Filter;						// audio filter bandwidth selection
uint8	Mode;						// operating mode
uint8	UpperSideBand;				// upper side band mode selection
uint8	AttenuatorOn;				// rx attenuator on/off flag
uint8	Squelch;					// squelch level, 0 - 250.
uint8	Debug;						// debug mode flag, turns off boundary checks


void main()
{
    /* Prepare components */
    
    Initialize();
    UpperSideBand = 0;
    LedTimer = LED_BLINK;
    
    for (;;)
    {
    		AdjustAgc ();				// AGC adjustment		
    }
}

//
//	Adjust AGC by monitoring ADC peak signal level
//	Adjusts gain every 8 msec, rate of 32068 / 256 Hz
//
void AdjustAgc (void)
{
	uint16 atten;
	
	if (!AdcTick)					// 8 msec tick
		return;
	AdcTick = FALSE;
	
	if (LedTimer)
	{
		if (--LedTimer == 0)
			CyPins_ClearPin (BLUELED_0);	// led off
		else
			CyPins_SetPin (BLUELED_0);	// led on
	}	

	if (AgcDisable)
		AdcPeak = 0;				// disable AGC adjustments and go to max gain
	else if (AdcPeak > 0x3800)
	{
		AgcIntegrator = (AgcIntegrator >> 2) * 3;	// overloaded input, reduce gain to 75%
		AgcGain = 1 + (AgcIntegrator >> 2);	
		LedTimer = LED_SHORTBLINK;	// blink the LED for overload
	}
	AdcPeakAve = (AdcPeakAve + AdcPeak) / 2;		// compute average peak ADC value
	AdcPeak = 0;	
	
	if (--AgcTimer)
		return;
	AgcTimer = 4;					// every 32 msec
	
	atten = (AttenuatorOn) ? 0x1000 : 0x2800;
	if (AdcPeakAve < atten)			 // clips if higher than this number
	{
		if (AgcIntegrator != 0xFF)
			AgcIntegrator++;		// increase gain
	}
	else if (AgcIntegrator)
		AgcIntegrator--;			// decrease gain
		
	AgcGain = 1 + (AgcIntegrator >> 2);	
}

//----------------------------------------------------------------------------
//
//	Initialize and configure hardware, initialize variables
//
//----------------------------------------------------------------------------
void Initialize (void)
{
	uint8 td;				        // transaction descriptor

	CyDelay (100);					// delay in msec
	
	Filter_Start ();				// start FIR filter
	VDAC_Start ();					// start DAC output
	
	ADC_Start ();					// power up and start 16 bit ADC
	ADC_IRQ_Enable ();
	ADC_StartConvert ();			// start conversions

	CyDelay (100);					// delay in msec
	
	IDCOffset = QDCOffset = 0x500;	// preset offset for better settling time

	CyDelay (10);					// delay in msec

	FrequencyError = 0;
	CyDelay (200);					// delay in msec
	
	CYGlobalIntEnable;
	
	// PLL lock test to prevent out of phase lock
	AgcGain = 0x0C;
	CyDelay (2000);							// let PLL lock
	for (td = 10; td; td--)
	{
		AdcPeak = 0;
		CyDelay (100);						// delay in msec
		AdcPeakAve += AdcPeak;
		AdcPeakAve /= 2;
	}
}

/* [] END OF FILE */
