//                          Software License Agreement
//
// The software supplied herewith by Microchip Technology Incorporated (the "Company")
// for its PICmicro® Microcontroller is intended and supplied to you, the Company’s
// customer, for use solely and exclusively on Microchip PICmicro Microcontroller
// products.
//
// The software is owned by the Company and/or its supplier, and is protected under
// applicable copyright laws. All rights are reserved. Any use in violation of the
// foregoing restrictions may subject the user to criminal sanctions under applicable
// laws, as well as to civil liability for the breach of the terms and conditions of
// this license.
//
// THIS SOFTWARE IS PROVIDED IN AN "AS IS" CONDITION. NO WARRANTIES, WHETHER EXPRESS,
// IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE
// COMPANY SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
// CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
//
// ###############################################################################
//	filename:		USB_MAIN.C
//				Sample main program
//
// Implements a USB mouse/keyboard combination device.  The main routine moves the
// cursor in a circle and sends keyboard keycodes for "Microchip USB   " over and  
// over again. 
//
// ###############################################################################
//
//	Authors:		Reston Condit, Dan Butler
//	Company:		Microchip Technology Inc
//
//	Revision:		-
//	Date:			3 April 2002
//	Compiled using:		HI-TECH C 7.87 PL2
//	Configuration Bits:	H4 Oscillator, WDT Off, Power up timer off
// 	Revision History:
//################################################################################
//
//	include files:
//		pic.h	        
//		usb.h
//
//###############################################################################

#include <pic.h>
#include "usb.h"
__CONFIG(0x3F3A);  // H4 Oscillator, WDT Off, Power up timer off, Code Protect off

bank1 static volatile bit NOT_RBPU @ 0x81*8+7;
bank3 static volatile unsigned char UCRL @ 0x15;

// Handler for NON-peripheral interrupts
void CoreInterrupt() {
	return;
}

// Handler for PORTB Change interrupt. Perform Remote Wakeup here
void PortBChange() {
	RBIF = 0;
	RemoteWakeup();
	return;
}

// Interrupt service routine. Branch off to different interrupts
interrupt ISR () {
	if (USBIF && USBIE) {
		ServiceUSBInt();
	} else if (RBIF && RBIE) {   // currently not enabled
		PortBChange();
	} else {
		CoreInterrupt();
	}
}

void main() {
	unsigned char i;
	unsigned char cnt;
	unsigned char state = 0;
	const unsigned char table [] = {-4, -4, -4, 0, 4, 4, 4, 0, -4, -4};
//					   M   i   c   r   o   c   h   i   p       U   S   B
	const unsigned char greeting [] = {16, 12, 06, 21, 18, 06, 11, 12, 19, 44, 24, 22, 05, 44, 44, 44}; // usb keycodes for "microchip usb   "
	const unsigned char capital [] =  {1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  0}; // capitalizations for "Microchip USB   "
	unsigned char buffer [4];
	unsigned char buffer2 [8];
	
	TRISB = 0;	// PORTB used as status indicator for USB.
	PORTB = 0;	// RB0 = powered, RB1 = default, RB2 = addressed, RB3 = configured
			// RB4 = sleeping, RB5 = EP0 activity, RB6 = EP1 activity, RB7 = EP2 activity
	for (i = 0; i < 50; i ++);
	InitUSB ();
	while ((USWSTAT & 0x03) != 3);  /* wait here until configured */
	buffer [0] = 0;
	cnt = 0;
	
	while (1)
	{
		buffer[1] = table [state];
		buffer[2] = table [state+2];
		++state;
		for (i = 0; i < 15; i++)
			while (PutEP1(4, buffer) == 0);
		if (state > 7)
			state = 0;
		if ((state & 0x01) == 1) {
			if (capital[cnt] == 1)		// capitalizes first letter of each word
				buffer2[0] = 2;
			buffer2[2] = greeting[cnt]; 	// sends message in greeting array
			while (PutEP2(8, buffer2) == 0);
			buffer2[0] = 0;
			buffer2[2] = 0;		
			while (PutEP2(8, buffer2) == 0);
			cnt++;
			cnt &= 0xF;
		}
	}
}
