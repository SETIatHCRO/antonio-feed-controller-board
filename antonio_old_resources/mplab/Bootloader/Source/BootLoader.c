/*********************************************************************
 *
 *                  PIC32 Boot Loader
 *
 *********************************************************************
 * FileName:        Bootloader.c
 * Dependencies:
 * Processor:       PIC32
 *
 * Compiler:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the �Company�) for its PIC32 Microcontroller is intended
 * and supplied to you, the Company�s customer, for use solely and
 * exclusively on Microchip PIC32 Microcontroller products.
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * $Id:  $
 * $Name: $
 *
 **********************************************************************/
#include "Include/GenericTypeDefs.h"
#include "Include/HardwareProfile/HardwareProfile.h"
#include "Include/BootLoader.h"
#include "Include/Framework/Framework.h"
//#include <stdlib.h>
#include <plib.h>

// Configuring the Device Configuration Registers
// 80Mhz Core/Periph, Pri Osc w/PLL, Write protect Boot Flash
#if defined(TRANSPORT_LAYER_USB) || defined(TRANSPORT_LAYER_USB_HOST)
    #pragma config UPLLEN   = ON        // USB PLL Enabled
    #pragma config UPLLIDIV = DIV_2     // USB PLL Input Divider = Divide by 2
#endif

//#pragma config DEBUG    = OFF           // Background Debugger disabled
//#pragma config FPLLMUL = MUL_20         // PLL Multiplier: Multiply by 20
//#pragma config FPLLIDIV = DIV_2         // PLL Input Divider:  Divide by 2

#if defined(__PIC32MX1XX_2XX__)
    // For PIC32MX1xx, PIC32MX2xx devices the output divisor is set to 2 to produce max 40MHz clock.
    #pragma config FPLLODIV = DIV_2         // PLL Output Divider: Divide by 2
#elif defined (__PIC32MX3XX_7XX__)
    //For PIC32MX3xx, PIC32MX4xx, PIC32MX5xx, PIC32MX6xx and PIC32MX7xx devices, 
    //the output divisor is set to 1 to produce max 80MHz clock.
//    #pragma config FPLLODIV = DIV_1         // PLL Output Divider: Divide by 1
#endif

//#pragma config FWDTEN = OFF             // WD timer: OFF
//#pragma config POSCMOD = HS             // Primary Oscillator Mode: High Speed xtal
//#pragma config FNOSC = PRIPLL           // Oscillator Selection: Primary oscillator  w/ PLL
//#pragma config FPBDIV = DIV_4           // Peripheral Bus Clock: Divide by 4
//#pragma config BWP = OFF                // Boot write protect: OFF

#if defined(__PIC32MX1XX_2XX__)
    // For PIC32MX1xx, PIC32MX2xx devices there are jumpers on PIM to choose from PGx1/PGx2.
    #pragma config ICESEL = ICS_PGx1    // ICE pins configured on PGx1 (PGx2 is multiplexed with USB D+ and D- pins).
#elif defined(__PIC32MX3XX_7XX__)
    // For PIC32MX3xx, PIC32MX4xx, PIC32MX5xx, PIC32MX6xx and PIC32MX7xx 
    // devices the ICE connection is on PGx2. .
//    #pragma config ICESEL = ICS_PGx2    // ICE pins configured on PGx2, Boot write protect OFF.
#endif
    
#if defined(TRANSPORT_LAYER_ETH)
	#pragma config FMIIEN = OFF, FETHIO = OFF	// external PHY in RMII/alternate configuration. Applicable for devices with internal MAC only.
#endif

#define SWITCH_PRESSED 0

// copied and pasted config settings from application

// DEVCFG3
// USERID = No Setting
#pragma config FSRSSEL = PRIORITY_7     // SRS Select (SRS Priority 7)
#pragma config FMIIEN = ON              // Ethernet RMII/MII Enable (MII Enabled)
#pragma config FETHIO = ON              // Ethernet I/O Pin Select (Default Ethernet I/O)
#pragma config FCANIO = ON              // CAN I/O Pin Select (Default CAN I/O)
#pragma config FUSBIDIO = ON            // USB USID Selection (Controlled by the USB Module)
#pragma config FVBUSONIO = ON           // USB VBUS ON Selection (Controlled by USB Module)

// DEVCFG2
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config UPLLIDIV = DIV_12        // USB PLL Input Divider (12x Divider)
#pragma config UPLLEN = OFF             // USB PLL Enable (Disabled and Bypassed)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)

// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = ON             // Secondary Oscillator Enable (Enabled)
#pragma config IESO = ON                // Internal/External Switch Over (Enabled)
#pragma config POSCMOD = EC             // Primary Oscillator Configuration (external)
#pragma config OSCIOFNC = OFF           // CLKO Output Signal Active on the OSCO Pin (Disabled)
#pragma config FPBDIV = DIV_4           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/4)
#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
#pragma config WDTPS = PS65536          // Watchdog Timer Postscaler (1:65536)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))

// DEVCFG0
#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
#pragma config CP = OFF                 // Code Protect (Protection Disabled)

BOOL CheckTrigger(void);
void JumpToApp(void);
BOOL ValidAppPresent(void);
/********************************************************************
* Function: 	main()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Main entry function. If there is a trigger or 
*				if there is no valid application, the device 
*				stays in firmware upgrade mode.
*
*			
* Note:		 	None.
********************************************************************/
INT main(void)
{
	UINT pbClk;

	// Setup configuration
	pbClk = SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
	
//	InitLED();
	
	// Enter firmware upgrade mode if there is a trigger or if the application is not valid
	if(CheckTrigger() || !ValidAppPresent())
	{
		// Initialize the transport layer - UART/USB/Ethernet
		TRANS_LAYER_Init(pbClk);
		
		while(!FRAMEWORK_ExitFirmwareUpgradeMode()) // Be in loop till framework recieves "run application" command from PC
		{
			// Enter firmware upgrade mode.
			// Be in loop, looking for commands from PC
			TRANS_LAYER_Task(); // Run Transport layer tasks
			FRAMEWORK_FrameWorkTask(); // Run frame work related tasks (Handling Rx frame, process frame and so on)
			// Blink LED (Indicates the user that bootloader is running).
//       		BlinkLED();
		}
		// Close trasnport layer.
		TRANS_LAYER_Close();
	}

	
	// No trigger + valid application = run application.
	JumpToApp();
	
	return 0;
}			


/********************************************************************
* Function: 	CheckTrigger()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		TRUE: If triggered
				FALSE: No trigger
*
* Side Effects:	None.
*
* Overview: 	Checks if there is a trigger to enter 
				firmware upgrade mode.
*
*			
* Note:		 	None.
********************************************************************/
BOOL  CheckTrigger(void)
{
//	UINT SwitchStatus;
//	SwitchStatus = ReadSwitchStatus();
//	if(SwitchStatus == SWITCH_PRESSED)
//	{
//		// Switch is pressed
//		return TRUE;
//	}

//      return (TRUE);

        // enter bootloader on software reset
        if (mGetSWRFlag()) {
            mClearSWRFlag();
            return (TRUE);
        }

//	else
//	{
		// Switch is not pressed.
        return FALSE;
//	}
}	


/********************************************************************
* Function: 	JumpToApp()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Jumps to application.
*
*			
* Note:		 	None.
********************************************************************/
void JumpToApp(void)
{	
	void (*fptr)(void);
	fptr = (void (*)(void))USER_APP_RESET_ADDRESS;
	fptr();
}	


/********************************************************************
* Function: 	ValidAppPresent()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		TRUE: If application is valid.
*
* Side Effects:	None.
*
* Overview: 	Logic: Check application vector has 
				some value other than "0xFFFFFFFF"
*
*			
* Note:		 	None.
********************************************************************/
BOOL ValidAppPresent(void)
{
	DWORD *AppPtr;
	AppPtr = (DWORD *)USER_APP_RESET_ADDRESS;
	if(*AppPtr == 0xFFFFFFFF)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}			


/*********************End of File************************************/


