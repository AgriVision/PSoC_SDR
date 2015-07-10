/*******************************************************************************
* File Name: ADC_INT.c
* Version 3.20
*
* Description:
*  This file contains the code that operates during the ADC_DelSig interrupt
*  service routine.
*
* Note:
*
********************************************************************************
* Copyright 2008-2015, Cypress Semiconductor Corporation.  All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "ADC.h"


/*******************************************************************************
* Custom Declarations and Variables
* - add user include files, prototypes and variables between the following
*   #START and #END tags
*******************************************************************************/
/* `#START ADC_SYS_VAR`  */
#include <device.h>
#include "main.h"

//----------------------------------------------------------------------------
//
//	Global variables
//
//----------------------------------------------------------------------------
uint8	AdcState;					// IQ channel processing flag
extern int16 IDCOffset, QDCOffset;	// DC offset values for ADC input
extern uint8 AdcTick;				// ADC interrupt flag every 6.521 msec
extern uint8 AgcGain;				// RF input gain
extern int16 AdcPeak;				// ADC input peak value
extern uint8 UpperSideBand;			// upper side band mode selection
int16	IDelay [64];
uint8	IDelayPtr;

/* `#END`  */


#if(ADC_IRQ_REMOVE == 0u)


    /*****************************************************************************
    * Function Name: ADC_ISR1
    ******************************************************************************
    *
    * Summary:
    *  Handle Interrupt Service Routine.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    * Reentrant:
    *  No
    *
    *****************************************************************************/
    CY_ISR( ADC_ISR1)
    {
        /**************************************************************************
        *  Custom Code
        *  - add user ISR code between the following #START and #END tags
        **************************************************************************/
        /* `#START MAIN_ADC_ISR1`  */
//----------------------------------------------------------------------------
//
//	SIMPLE SDR RECEIVER PSoC3 FIRMWARE for Hardware Rev -
//
//	Copyright 2011 Simple Circuits Inc.
//
//	06/03/2011	Original release.
//
//	ADC interrupt processing
//
//
//	Interrupts every 31.18 usec, 32068 sps, 16034 sps per channel
//	TP2 is low for 8.5 to 11 usec (optimization 5), freq = 14.070 MHz = Fvco = 56.28 MHz
//----------------------------------------------------------------------------
	int16	rxRaw;		// must be signed int
	uint16	sample;		// less compiled code when using unsigned int
    uint16 test;
        
	//CyPins_ClearPin(TP2_OUT_0);

	if (AdcState & 1)			// I channel input processing
	{
		IQMUX_Control = 2;		// selects I channel of IQMUX, IQMUX_Write (2);
		
		// read ADC, subtract out DC offset, and multiply by gain
		rxRaw = ADC_GetResult16 () - IDCOffset;
		IDCOffset += (rxRaw & 0x8000) ? -1 : 1;	// calc new DC offset
		rxRaw = rxRaw * AgcGain;

		IDelay [IDelayPtr++] = rxRaw;			// store I channel in delay line
		IDelayPtr &= 31;

		// Write Filter A output to PRS DAC
		VDAC_Data = Filter_HOLDAH_REG ^ 0x80;
        // test !!!!!!!!!!!!!!!!!!!
        //test = Filter_Read16(Filter_CHANNEL_B);
        //VDAC_Data = (uint8)(test >> 8);
	}
	else						// Q channel input processing
	{
		IQMUX_Control = 1;		// selects Q channel of IQMUX, IQMUX_Write (1);

		// read ADC, subtract out DC offset, and multiply by gain
		rxRaw = ADC_GetResult16 () - QDCOffset;
		QDCOffset += (rxRaw & 0x8000) ? -1 : 1;	// calc new DC offset before any gain adjust
		rxRaw = rxRaw * AgcGain;
		
		if (UpperSideBand)
			sample = IDelay [IDelayPtr] - Filter_Read16 (Filter_CHANNEL_B);
		else
			sample = IDelay [IDelayPtr] + Filter_Read16 (Filter_CHANNEL_B);

		// Filter_Write16 (Filter_CHANNEL_A, IDelay [IDelayPtr] +/- Filter_Read16 (Filter_CHANNEL_B));
		Filter_STAGEAM_REG = (uint8)(sample);
        Filter_STAGEAH_REG = (uint8)(sample >> 8);

		//  replace Filter_Write16 (Filter_CHANNEL_B, rxRaw);
        Filter_STAGEBM_REG = (uint8)(rxRaw);
        Filter_STAGEBH_REG = (uint8)(rxRaw >> 8);

		if (!AdcState)
			AdcTick = TRUE;
	}
	AdcState++;
	
	if (rxRaw > AdcPeak)
		AdcPeak = rxRaw;
	
	//CyPins_SetPin(TP2_OUT_0);		   


        /* `#END`  */

        /* Stop the conversion if conversion mode is single sample and resolution
        *  is above 16 bits.
        */
        #if(ADC_CFG1_RESOLUTION > 16 && \
            ADC_CFG1_CONV_MODE == ADC_MODE_SINGLE_SAMPLE)
            ADC_StopConvert();
            /* Software flag for checking conversion complete or not. Will be used when
            *  resolution is above 16 bits and conversion mode is single sample 
			*/
            ADC_convDone = ADC_DEC_CONV_DONE;
        #endif /* Single sample conversion mode with resolution above 16 bits */

    }


    /*****************************************************************************
    * Function Name: ADC_ISR2
    ******************************************************************************
    *
    * Summary:
    *  Handle Interrupt Service Routine.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    * Reentrant:
    *  No
    *
    *****************************************************************************/
    CY_ISR( ADC_ISR2)
    {
        /***************************************************************************
        *  Custom Code
        *  - add user ISR code between the following #START and #END tags
        **************************************************************************/
        /* `#START MAIN_ADC_ISR2`  */

        /* `#END`  */

        /* Stop the conversion conversion mode is single sample and resolution
        *  is above 16 bits.
        */
        #if(ADC_CFG2_RESOLUTION > 16 && \
            ADC_CFG2_CONVMODE == ADC_MODE_SINGLE_SAMPLE)
            ADC_StopConvert();
            /* Software flag for checking conversion complete or not. Will be used when
            *   resolution is above 16 bits and conversion mode is single sample 
			*/
            ADC_convDone = ADC_DEC_CONV_DONE;
        #endif /* Single sample conversion mode with resolution above 16 bits */

    }


    /*****************************************************************************
    * Function Name: ADC_ISR3
    ******************************************************************************
    *
    * Summary:
    *  Handle Interrupt Service Routine.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    * Reentrant:
    *  No
    *
    *****************************************************************************/
    CY_ISR( ADC_ISR3)
    {
        /***************************************************************************
        *  Custom Code
        *  - add user ISR code between the following #START and #END tags
        **************************************************************************/
        /* `#START MAIN_ADC_ISR3`  */

        /* `#END`  */

        /* Stop the conversion if conversion mode is set to single sample and
        *  resolution is above 16 bits.
        */
        #if(ADC_CFG3_RESOLUTION > 16 && \
            ADC_CFG3_CONVMODE == ADC_MODE_SINGLE_SAMPLE)
            ADC_StopConvert();
            /* Software flag for checking conversion complete or not. Will be used when
            *  resolution is above 16 bits and conversion mode is single sample 
			*/
            ADC_convDone = ADC_DEC_CONV_DONE;
        #endif /* Single sample conversion mode with resolution above 16 bits */
    }


    /*****************************************************************************
    * Function Name: ADC_ISR4
    ******************************************************************************
    *
    * Summary:
    *  Handle Interrupt Service Routine.
    *
    * Parameters:
    *  None
    *
    * Return:
    *  None
    *
    * Reentrant:
    *  No
    *
    *****************************************************************************/
    CY_ISR( ADC_ISR4)
    {
        /***************************************************************************
        *  Custom Code
        *  - add user ISR code between the following #START and #END tags
        **************************************************************************/
        /* `#START MAIN_ADC_ISR4`  */

        /* `#END`  */

        /* Stop the conversion if conversion mode is set to single sample and
        *  resolution is above 16 bits.
        */
        #if(ADC_CFG4_RESOLUTION > 16 && \
            ADC_CFG4_CONVMODE == ADC_MODE_SINGLE_SAMPLE)
            ADC_StopConvert();
            /* Software flag for checking conversion complete or not. Will be used when
            *  resolution is above 16 bits and conversion mode is single sample 
			*/
            ADC_convDone = ADC_DEC_CONV_DONE;
        #endif /* Single sample conversion mode with resolution above 16 bits */
    }

#endif   /* End ADC_IRQ_REMOVE */


/* [] END OF FILE */
