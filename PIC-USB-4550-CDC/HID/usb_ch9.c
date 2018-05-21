//                            Software License Agreement
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
//	filename:		USB_CH9.AS
//				USB Human Interface Device (HID) class specific
//				commands
//
// Implements the chapter 9 enumeration commands for Microchip's
// PIC16C7x5 parts.  
//
// Control is routed here when a token done interrupt is received and
// the bit 5 of the RequestType byte is set (0x20).  Prior to this, the
// EP0 buffer has been copied to common RAM.
//
// ###############################################################################
//
//	Authors:		Reston Condit, Dan Butler
//	Company:		Microchip Technology Inc
// 
// 	Revision:		1.21
// 	Date:			5 March 2002
// 	Compiled using:		HI-TECH C V7.87 PL2
// 	Revision History:
// 
// ################################################################################
// 
// 	include files:
// 		Pic.h     	Rev 1.00
// 		usb_defs.h	Rev 1.00
// 
// ################################################################################

#include "pic.h"
#include "usb_defs.h"
#define SHOW_ENUM_STATUS 1
void USBReset ();
void TokenDone ();
void USBSleep ();
void USBActivity ();
void USBStall ();
void USBError ();
extern void	ClassSpecificRequest ();

bank2 unsigned char  isidle;
bank2 unsigned char  USB_status_device;
bank2 unsigned char  USB_Curr_Config; 
bank2 unsigned char  USB_address_pending;
bank2 unsigned char  USB_dev_req;
bank2 unsigned char  USB_Interface [3];
bank2 unsigned char  USB_USTAT;

static volatile unsigned int USB_Buffer @ 0x1b8;

bank2 unsigned int USB_PID_ERR   = 0;
bank2 unsigned int USB_CRC5_ERR  = 0;
bank2 unsigned int USB_CRC16_ERR = 0;
bank2 unsigned int USB_DFN8_ERR  = 0;
bank2 unsigned int USB_BTO_ERR   = 0;
bank2 unsigned int USB_WRT_ERR   = 0;
bank2 unsigned int USB_OWN_ERR   = 0;
bank2 unsigned int USB_BTS_ERR   = 0;

const char * EP0_start;
const char * EP0_end;
bank2 unsigned char EP0_maxLength;

struct BufferDescriptorEntry 
{
	unsigned char EPStat;
	unsigned char bytes;
	unsigned char address;
	unsigned char filler;
};
struct BufferStruct 
{
	unsigned char bmRequestType;
	unsigned char bRequest;
	unsigned int   wValue;
	unsigned int   wIndex;
	unsigned int   wLength;
};

bank2 struct BufferDescriptorEntry * BDT= (struct BufferDescriptorEntry *)0xa0;
bank2 struct BufferStruct * bank2 Buffer;
bank2 struct BufferStruct  BufferCopy;
bank2 struct BufferDescriptorEntry BDTCopy;

// mouse report descriptor
const char ReportDescriptor1 [] =      {0x05, 1,	/* usage page */
					0x09, 2,	/* usage mouse */
					0xa1, 1,	/* collection (application) */
					0x09, 1,	/* usage (pointer) */
					0xa1, 0,	/* collection (linker) */
					0x05, 9,	/* usage page (buttons) */
					0x19, 1,	/* usage minimum (1) */
					0x29, 3,	/* usage maximum (3) */
					0x15, 0,	/* logical minimum (0) */
					0x25, 1,	/* logical maximum (0) */
					0x95, 3,	/* report count (3 bytes) */
					0x75, 1,	/* report size (1 bit) */
					0x81, 2, 	/* input (3 bits) */
					0x95, 1,	/* report count (1 byte) */
					0x75, 5,	/* Report size (5 bits) */
					0x81, 1, 	/* input (constant 5 bit padding) */
					0x05, 1,	/* usage page (generic desktop) */
					0x09, 0x30,	/* usage X */
					0x09, 0x31,	/* usage Y */
					0x09, 0x38, 
					0x15, 0x81,	/* logical minimum -127 */
					0x25, 0x7F,	/* logical maximum 127 */
					0x75, 0x08,	/* report size (8) */
					0x95, 3,	/* report count 2 */
					0x81, 6,	/* input (2 position butes X & Y) */
					0xc0, 0xc0};	/* end collection */
// keyboard report descriptor
const char ReportDescriptor2 [] =	{0x05, 0x01,	// usage page (generic desktop)
					0x09, 0x06,	// usage (keyboard)
					0xA1, 0x01,	// collection (application)
					0x05, 0x07,	// usage page (key codes)
					0x19, 0xE0,	// usage minimum (224)
					0x29, 0xE7,	// usage maximum (231)
					0x15, 0x00,	// logical minimum (0)
					0x25, 0x01,	// logical maximum (1)
					0x75, 0x01,	// report size (1)
					0x95, 0x08,	// report count (8)
					0x81, 0x02,	// input (data, variable, absolute)
					0x95, 0x01,	// report count (1)
					0x75, 0x08,	// report size (8)
					0x81, 0x01,	// input (constant)
					0x95, 0x05,	// report count (5)
					0x75, 0x01,	// report size (1)
					0x05, 0x08,	// usage page (page# for LEDs)
					0x19, 0x01,	// usage minimum (1)
					0x29, 0x05,	// usage maxmimum (5)
					0x91, 0x02,	// output (data, variable, absolute)
					0x95, 0x01,	// report count (1)
					0x75, 0x03,	// report size (3)
					0x91, 0x01,	// output (constant)
					0x95, 0x06,	// report count (6)
					0x75, 0x08,	// report size (8)
					0x15, 0x00,	// logical minimum (0)
					0x25, 0x65,	// logical maximum (101)
					0x05, 0x07,	// usage page (key codes)
					0x19, 0x00,	// usage minimum (0)
					0x29, 0x65,	// usage maximum (101)
					0x81, 0x00,	// input (data, array)
					0xC0};		// end collection

const char DeviceDescriptor [] = {0x12, DEVICE, 0, 1, 0, 0, 0, 8, 0xD8, 4, 0x0C, 0, 0x41, 4, 1, 2, 0, 1};
const char ConfigDescriptor [] = {9, CONFIGURATION, 0x3B, 0, 2, 1, 4, 0xa0, 0x32, 
/* Interface Descriptor1  */      9, INTERFACE, 0, 0, 1, 3, 1, 2, 5,
/* HID descriptor1        */      9, 0x21, 0, 1, 0, 1, 0x22, sizeof(ReportDescriptor1), sizeof(ReportDescriptor1)>>8,
/* Endpoint 1 descriptor */       7, ENDPOINT, 0x81, 3, 4, 0, 0x0a,
/* Interface Descriptor2  */      9, INTERFACE, 1, 0, 1, 3, 1, 1, 6,
/* HID descriptor2        */      9, 0x21, 0, 1, 0, 1, 0x22, sizeof(ReportDescriptor2), sizeof(ReportDescriptor2)>>8,
/* Endpoint 2 descriptor */       7, ENDPOINT, 0x82, 3, 8, 0, 0x0a};
const char HIDDescriptor1 [] = {9, 0x21, 0, 1, 0, 1, 0x22, sizeof(ReportDescriptor1), sizeof(ReportDescriptor1)>>8};
const char HIDDescriptor2 [] = {9, 0x21, 0, 1, 0, 1, 0x22, sizeof(ReportDescriptor2), sizeof(ReportDescriptor2)>>8};
const char String0 [] = { 4, STRING, 9, 4};
const char String1 [] = {20, STRING, 'M', 0, 'i', 0, 'c', 0, 'r', 0, 'o', 0, 'c', 0, 'h', 0, 'i', 0, 'p', 0};
const char String2 [] = {68, STRING, 'M', 0, 'o', 0, 'u', 0, 's', 0, 'e', 0, '/', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0, 
				     ' ', 0, 'C', 0, 'o', 0, 'm', 0, 'b', 0, 'i', 0, 'n', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0, ' ', 0, 'D', 0, 
				     'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0};
const char String3 [] = {12, STRING, 'V', 0, '1', 0, '.', 0, '1', 0, '1', 0};
const char String4 [] = {10, STRING, 'C', 0, 'f', 0, 'g', 0, '1', 0};
const char String5 [] = {24, STRING, 'E', 0, 'P', 0, '1', 0, 'I', 0, 'N', 0, ' ', 0, 'M', 0, 'o', 0, 'u', 0, 's', 0, 'e', 0};
const char String6 [] = {30, STRING, 'E', 0, 'P', 0, '2', 0, 'I', 0, 'N', 0, ' ', 0, 'K', 0, 'e', 0, 'y', 0, 'b', 0, 'o', 0, 'a', 0, 'r', 0, 'd', 0}; 

/* ****** */
/* PutEP1 **************************************************************** */
/* Tests the EP1 IN OWNS bit.  If there is a buffer available to us, your  */
/* buffer is copied and turned over to the SIE for transmission on the     */
/* next IN transfer and returns TRUE (1).  If the buffer is not available, */
/* FALSE is returned (0).                                                  */
/* *********************************************************************** */
unsigned char PutEP1 (unsigned char bytes, unsigned char *buffer)
{
	bank2 unsigned char * tobuffer;
	unsigned char i;

	if ((BD1IST & 0x80) == 0)  /* do we own the buffer? */
	{
		BD1IBC = bytes;
		tobuffer = (unsigned char *)BD1IAL;
		for (i = 0; i < bytes; i++)
			tobuffer [i] = buffer[i];
		BD1IST &= 0x40; /* save only the Data 1/0 bit */
		BD1IST ^= 0x40; /* toggle Data 0/1 bit */
		BD1IST |= 0x88;	/* Turn the buffer back over to the SIE */
		return TRUE;
	}
	else
		return FALSE;			/* Buffer not available, return false */
}

/* ****** */
/* PutEP2 **************************************************************** */
/* Tests the EP2 IN OWNS bit.  If there is a buffer available to us, your  */
/* buffer is copied and turned over to the SIE for transmission on the     */
/* next IN transfer and returns TRUE (1).  If the buffer is not available, */
/* FALSE is returned (0).                                                  */
/* *********************************************************************** */
unsigned char PutEP2 (unsigned char bytes, unsigned char *buffer)
{
	bank2 unsigned char * tobuffer;
	unsigned char i;

	if ((BD2IST & 0x80) == 0)  /* do we own the buffer? */
	{
		BD2IBC = bytes;
		tobuffer = (unsigned char *)BD2IAL;
		for (i = 0; i < bytes; i++)
			tobuffer [i] = buffer[i];
		BD2IST &= 0x40; /* save only the Data 1/0 bit */
		BD2IST ^= 0x40; /* toggle Data 0/1 bit */
		BD2IST |= 0x88;	/* Turn the buffer back over to the SIE */
		return TRUE;
	}
	else
		return FALSE;			/* Buffer not available, return false */
}

/* ****** */
/* GetEP1 **************************************************************** */
/* Tests the EP1 OUT OWNS bit.  If there is a buffer available to us (data */
/* has been received, it copies the buffer to your buffer and returns the  */
/* number of bytes received.  If no buffer is available, it returns 0      */
/* *********************************************************************** */
unsigned char GetEP1 (unsigned char *buffer)
{
	bank2 unsigned char *  FromBuffer;
	unsigned char i;

	if ((BD1OST & 0x80) == 0)  /* do we own the buffer? */
	{
		FromBuffer = (unsigned char * )BD1OAL;
		for (i = 0; i < BD1OBC; i++)
			buffer [i] = FromBuffer[i];
		BD1OST &= 0x40; /* save only the Data 1/0 bit */
		BD1OST ^= 0x40; /* toggle Data 0/1 bit */
		BD1OST |= 0x88;	/* Turn the buffer back over to the SIE */
		return BD1OBC;
	}
	else
		return FALSE;			/* Buffer not available, return false */
}


/* ******************* */
/* CopyDescriptorToEP0 ************************************************** */
/* copies the next chunk of buffer descriptor over to the EP0 In buffer.  */
/* Inputs:                                                                */
/*    EP0_start - points to first byte of configuration table to transfer */
/*    EP0_end - total number of bytes to transfer                         */
/*    EP0_maxLength - maximum number of bytes that can be sent during     */
/*    a single transfer                                                   */
/*                                                                        */
/* toggles the data0/1 bit before setting the UOWN bit over to SIE.       */
/* ********************************************************************** */
void CopyDescriptorToEP0 ()
{
	bank2 unsigned char *  USBbuffer;  /* pointer to the USB Buffers */
	unsigned char bufindex;

	USBbuffer = (unsigned char * ) BD0IAL + 0x100;
	bufindex  = 0;

	while ((bufindex < EP0_maxLength) && (EP0_start < EP0_end))
	{
		USBbuffer [bufindex] = *EP0_start;
		++ EP0_start;
		++ bufindex;
	}
	if (bufindex < EP0_maxLength)	/* are we sending a short packet? */
		USB_dev_req = 0;	/* Yes, clear the device reqest */
	BD0IBC  = bufindex;
	BD0IST &= 0x40;		/* save only the Data0/1 bit */
	BD0IST ^= 0x40;		/* toggle data 0/1 bit */
	BD0IST |= 0x88;		/* set OWN and DTS bits */
}



/* ******** */
/* Init USB ********************************************************* */
/* Initializes the USB peripheral, sets up the interrupts             */
/* ****************************************************************** */
void InitUSB ()
{
	USWSTAT           = 0;
	UIE               = 0x01;	// default to powered state
	UIR               = 0;		// clear all USB interrupt flags
	UCTRL             = 0x08;	// Device attached
	USB_Curr_Config   = 0;
	USB_status_device = 1;
	USB_Interface [0] = 0;
	USB_Interface [1] = 0;
	USB_Interface [2] = 0;
	USB_dev_req       = NULL;
	USB_PID_ERR       = 0;
	USB_CRC5_ERR      = 0;
	USB_CRC16_ERR     = 0;
	USB_DFN8_ERR      = 0;
	USB_BTO_ERR       = 0;
	USB_WRT_ERR       = 0;
	USB_OWN_ERR       = 0;
	USB_BTS_ERR       = 0;
	USBIF             = 0;
	USBIE             = 1;
	INTCON            = 0xC0;	// Enable GIE & PEIE
#ifdef SHOW_ENUM_STATUS
	RB0               = 1;
#endif
}

/* ****************************************************************** */
/* DeInit USB                                                         */
/* Shuts down the USB peripheral, clears the interrupt enable.        */
/* ****************************************************************** */
void DeInitUSB ()
{
	DEV_ATT = 0;
	SUSPND  = 1;
	USWSTAT = 0;
	USBIE   = 0;
}

/* *************** */
/* Stall Endpoint. ***************************************************** */
/* Sets the stall bit in the Endpoint Control Register.  For the control */
/* Endpoint, this implements a Protocol stall and is used when the request */
/* is invalid for the current device state.  For non-control Endpoints,  */
/* this is a Functional Stall, meaning that the device needs outside     */
/* intervention and trying again later won't help until it's been serviced. */
/* enter with endpoint # to stall in Wreg.                               */
/* ********************************************************************* */
void StallUSBEP (unsigned char EndPoint)
{
	*(&UEP0 + EndPoint) |= 1;
}

/* ***************** */
/* Unstall Endpoint. ***************************************************** */
/* Sets the stall bit in the Endpoint Control Register.  For the control   */
/* Endpoint, this implements a Protocol stall and is used when the request */
/* is invalid for the current device state.  For non-control Endpoints,    */
/* this is a Functional Stall, meaning that the device needs outside       */
/* intervention and trying again later won't help until it's been serviced. */
/* enter with endpoint # to stall in Wreg.                                 */
/* *********************************************************************** */
UnstallUSBEP (unsigned char EndPoint)
{
	*(&UEP0 + EndPoint) &= 0xFE;
}


/* ********************************************************************* */
/* USB Soft Detach                                                       */
/* Clears the DEV_ATT bit, electrically disconnecting the device to the  */
/* bus.  This removes the device from the bus, then reconnects so it can */
/* be re-enumerated by the host.  This is envisioned as a last ditch     */
/* effort by the software.                                               */
/* ********************************************************************* */
void SoftDetachUSB ()
{
	unsigned int  i;

	DEV_ATT = 0;	// clear the attach bit
	for (i = 0; i < 65000; i++);  // delay 50mS
	InitUSB ();
}

/* ******************************************************************** */
/* Remote Wakeup                                                        */
/* Checks USB_status_device to see if the host enabled Remote Wakeup.   */
/* If so, perform Remote wakeup and disable remote wakeup feature.      */
/* It is called by PortBChange.                                         */
/* ******************************************************************** */

void RemoteWakeup ()
{
	unsigned int i;
	if (USB_status_device&0x02) {
		SUSPND = 0;
		UIDLE_E = 1;
		UIDLE = 0;
		ACTIVITY_E = 0;
		ACTIVITY = 0;
		UCTRL |= 0x04;
	}
	for (i = 0; i < 60000; i++);   // delay 10ms
	UCTRL &= 0xFB;
}

/* ****************************************************************** */
/* USB interrupt triggered, Why?                                      */
/* OK, we've identified that we're servicing a USB interrupt.         */
/* ****************************************************************** */
void ServiceUSBInt ()
{
	if (TOK_DONE && TOK_DONE_E)
		TokenDone();
	else if (USB_RST && USB_RST_E)
		USBReset ();
	else if (STALL && STALL_E)
		USBStall ();
	else if (UERR && UERR_E)
		USBError ();
	else if (UIDLE && UIDLE_E)
		USBSleep ();
	else if (ACTIVITY && ACTIVITY_E)
		USBActivity ();
	USBIF = 0;
}

/* ****************************************************************** */
/* USB Reset interrupt triggered (SE0)                                */
/* initialize the Buffer Descriptor Table,                            */
/* Transition to the DEFAULT state,                                   */
/* Set address to 0                                                   */
/* enable the USB                                                     */
/* ****************************************************************** */
void USBReset ()
{
	USW_STAT = POWERED_STATE;
	isidle   = 0;

	TOK_DONE = 0;	// clear 4 times to clear out the USTAT FIFO
	TOK_DONE = 0;
	TOK_DONE = 0;
	TOK_DONE = 0;

	BD0OBC   = 8;
	BD0OST   = 0x88;	// EP0 Out buffer
	BD0OAL   = 0xB8;

	BD0IST   = 0x08;
	BD0IAL   = 0xC0;

	UADDR    = 0;
	UIR      = 0;
	USBIF    = 0;

	UEP0     = ENDPT_CONTROL;
	UIE      = 0x3B;	// enable all interrupts except activity
	UEIE     = 0xFF;	// enable all error interrupts
	USWSTAT  = DEFAULT_STATE;
	USB_status_device = 0;
#ifdef SHOW_ENUM_STATUS
	RB1 = 1;		// set bit one to indicate Reset status
#endif
}

/* ********************************************************************* */
/* This is activated by the STALL bit in the UIR register.  It really    */
/* just tells us that the SIE sent a STALL handshake.  So far, Don't     */
/* see that any action is required.  Clear the bit and move on.          */
/* ********************************************************************* */
void USBStall ()
{
	STALL = 0;
	USBIF = 0;
}

/* ********************************************************************* */
/* Enable Wakeup on interupt and Activity interrupt then put the         */
/* device to sleep to save power.  Activity on the D+/D- lines will      */
/* set the ACTIVITY interrupt, waking up the part.                       */
/* ********************************************************************* */
void USBSleep ()
{
	UIDLE      = 0;
	UIDLE_E    = 0;
	ACTIVITY   = 0;
	ACTIVITY_E = 1;
	SUSPND	   = 1;
	USBIF      = 0;
	isidle     = 1;
}

/* ********************************************************************* */
/* Service the Activity Interrupt.  This is only enabled when the        */
/* device is put to sleep as a result of inactivity on the bus.  This    */
/* code wakes up the part, disables the activity interrupt and reenables */
/* the idle interrupt.                                                   */
/* ********************************************************************* */
void USBActivity ()
{
	ACTIVITY   = 0;
	ACTIVITY_E = 0;
	UIDLE      = 0;
	UIDLE_E    = 1;
	SUSPND     = 0;
	USBIF      = 0;
	isidle     = 0;
}

/* ****************************************************************** */
/* The SIE detected an error.  This code increments the appropriate   */
/* error counter and clears the flag.                                 */
/* ****************************************************************** */
void USBError ()
{
	if (PID_ERR && PID_ERR_E)
		++ USB_PID_ERR;
	if (CRC5 && CRC5_E)
		++ USB_CRC5_ERR;
	if (CRC16 && CRC16_E)
		++ USB_CRC16_ERR;
	if (DFN8 && DFN8_E)
		++ USB_DFN8_ERR;
	if (BTO_ERR && BTO_ERR_E)
		++ USB_BTO_ERR;
	if (WRT_ERR && WRT_ERR_E)
		++ USB_PID_ERR;
	if (OWN_ERR && OWN_ERR_E)
		++ USB_OWN_ERR;
	if (BTS_ERR && BTS_ERR_E)
		++ USB_BTS_ERR;
	UERR = 0;
}


/* ******************************************************************* */
/* Process token done interrupt...  Most of the work gets done through */
/* this interrupt.  Token Done is signaled in response to an In, Out,  */
/* or Setup transaction.                                               */
/* ******************************************************************* */
void TokenDone ()
{
	bank3 unsigned char  *OutBuffer;
	bank3 unsigned char *UEPArray;
	unsigned char  DescriptorType;
	unsigned char  Endpoint;
	unsigned char  Interface;
	unsigned char  DescriptorID;
	unsigned char  StringID;

	BDTCopy.EPStat  = BDT[USTAT/4].EPStat;
	BDTCopy.address = BDT[USTAT/4].address;
	BDTCopy.bytes   = BDT[USTAT/4].bytes;
	USB_USTAT       = USTAT;
	TOK_DONE        = 0;
	USBIF           = 0;

#ifdef SHOW_ENUM_STATUS
	if (USB_USTAT == 0x00)  PORTB ^= 0x20;
	if (USB_USTAT == 0x08)  PORTB ^= 0x40;
	if (USB_USTAT == 0x18)  PORTB ^= 0x80;
#endif
	if ((BDTCopy.EPStat & 0x3C) == TOKEN_IN)
	{
		if (USB_USTAT == 0x04)
		{ /* Process EP0 In's */
			if (USB_dev_req == GET_DESCRIPTOR)
			{
				CopyDescriptorToEP0 ();
			}
			else if (USB_dev_req == SET_ADDRESS)
			{   /* Finish Set Address */
				USB_dev_req = NULL;
				USB_Curr_Config = 0;
				UADDR = USB_address_pending;
				if (USB_address_pending > 0)
				{
					USWSTAT = ADDRESS_STATE;
#ifdef SHOW_ENUM_STATUS
				RB2 = 1;
#endif		
				}
				else
					USWSTAT = DEFAULT_STATE;
			}	
			else if (USB_dev_req == SET_FEATURE)
			{
			}
			else if (USB_dev_req == CLEAR_FEATURE)
			{
				if (BufferCopy.wIndex == 1)
					UEP1 &= 0x01;  /* Clear the endpoint stall bit */
				if (BufferCopy.wIndex == 2)
					UEP2 &= 0x01;  /* Clear the endpoint stall bit */
			}
		}
		else if (USB_USTAT == 0x0C)
		{ /* process EP1 In's */
		}
		else
		{ /* process EP2 In's */
		}
	}
	else if ((BDTCopy.EPStat & 0x3C) == TOKEN_OUT)
	{
	    if (USB_dev_req == HID_SET_REPORT)
		{
			// Call your own function that will implement Set_Report here.
			Send_0Len_pkt;
		}
		BD0OBC = 0x08;		// Reset buffer and move on.
		BD0OST = 0x88;
		if (BufferCopy.wLength > 8)
			(BufferCopy.wLength -= 8;
		else
			Send_0Len_pkt;
	}
	else if ((BDTCopy.EPStat & 0x3C) == TOKEN_SETUP)
	{
		
		Buffer = (struct BufferStruct * bank2) BD0OAL;
		BufferCopy.bmRequestType = Buffer->bmRequestType;
		BufferCopy.bRequest      = Buffer->bRequest;
		BufferCopy.wValue        = Buffer->wValue;
		BufferCopy.wIndex        = Buffer->wIndex;
		BufferCopy.wLength       = Buffer->wLength;

		BD0OBC      = 0x08;
		if (BufferCopy.bmRequestType == HID_SET_REPORT)
			BD0OST	= 0xC8;
		else
			BD0OST  = 0x88;	/* Turn the buffer around, make it available for the SIE */
		BD0IST      = 0x08;
		PKT_DIS     = 0;
		USB_dev_req = 0;

		switch (BufferCopy.bmRequestType)
		{
		case HOSTTODEVICE:
			switch (BufferCopy.bRequest)
			{
			case CLEAR_FEATURE:	/* Remote wakeup is only valid device feature */
				if (BufferCopy.wValue == 1)
				{
					USB_status_device &= 0xFD;
					Send_0Len_pkt;
				}
				else
					STALL_EP0;
				break;

			case SET_FEATURE:				/* Set Device Feature.  Only valid device */
				if ((BufferCopy.wValue & 0xff) == 1)
				{
					USB_status_device |= 0x02;	/* feature is remote wakeup */
					Send_0Len_pkt;
				}
				else
					STALL_EP0;
				break;

			case SET_ADDRESS:
				USB_address_pending = BufferCopy.wValue;
				if (USB_address_pending < 0x80)
				{
					Send_0Len_pkt;
					USB_dev_req = SET_ADDRESS;
				}
				else
					STALL_EP0;
				break;

			case SET_CONFIGURATION:
				if (BufferCopy.wValue <= NUM_CONFIGURATIONS)
					USB_Curr_Config = BufferCopy.wValue;

				if (BufferCopy.wValue == 0)
					USWSTAT = ADDRESS_STATE;
				else
				{
					USWSTAT = CONFIG_STATE;
#ifdef SHOW_ENUM_STATUS
					RB3 = 1;		// set bit 3 to show configured
#endif
				}
				Send_0Len_pkt;
				BD1OST = 0x88;
				BD1OAL = (unsigned char)Buffer + 0x10;
				BD1OBC = 8;

				BD1IST = 0x48;
				BD1IAL = (unsigned char)Buffer + 0x18;
				BD1IBC = 8;

				BD2OST = 0x88;
				BD2OAL = (unsigned char)Buffer + 0x20;
				BD2OBC = 8;

				BD2IST = 0x48;
				BD2IAL = (unsigned char)Buffer + 0x20;
				BD2IBC = 8;

				UEP1 = ENDPT_NON_CONTROL;
				UEP2 = ENDPT_NON_CONTROL;

				break;

			default:
				STALL_EP0;
				break;
			}
			break;

		case HOSTTOINTERFACE:
			switch (BufferCopy.bRequest)
			{
			case SET_INTERFACE:
				if (USWSTAT == CONFIG_STATE)
				{
					Interface = BufferCopy.wIndex;
					USB_Interface [Interface] = BufferCopy.wValue;
					Send_0Len_pkt;
				}
				else
					STALL_EP0;
				break;

			case CLEAR_FEATURE:
			case SET_FEATURE:	/* Set Interface feature - Not Valid */
			default:
				STALL_EP0;
				break;
			}
			break;

		case HOSTTOENDPOINT:
			switch (BufferCopy.bRequest)
			{
			case CLEAR_FEATURE:
				UEPArray = (unsigned char *) &UEP0;
				Endpoint = BufferCopy.wIndex & 0x0F;
				if (BufferCopy.wValue == 0)  /* Only valid feature is 0 (Remote Wakeup) */
				{
					if (((USWSTAT & 0x03) == ADDRESS_STATE) && (Endpoint == 0))
					{
						UEPArray [Endpoint] &= 0xFE;
						Send_0Len_pkt;
					}
					else if (((USWSTAT & 0x03) == CONFIG_STATE) && (Endpoint < 3))
					{
						UEPArray [Endpoint] &= 0xFE;
						Send_0Len_pkt;
					}
					else
						STALL_EP0;
				}
				else
					STALL_EP0;
				break;

			case SET_FEATURE:
				UEPArray = (unsigned char *) &UEP0;
				Endpoint = BufferCopy.wIndex & 0x0F;
				if (BufferCopy.wValue == 0)  /* Only valid feature is 0 (Remote Wakeup) */
				{
					if (((USWSTAT & 0x03) == ADDRESS_STATE) && (Endpoint == 0))
					{
						UEPArray [Endpoint] |= 1;
						Send_0Len_pkt;
					}
					else if (((USWSTAT & 0x03) == CONFIG_STATE) && (Endpoint < 3))
					{
						UEPArray [Endpoint] |= 1;
						Send_0Len_pkt;
					}
					else
						STALL_EP0;
				}
				else
					STALL_EP0;
				break;

			default:
				STALL_EP0;
				break;
			}
			break;

		case DEVICETOHOST:
			switch (BufferCopy.bRequest)
			{
			case GET_CONFIGURATION:
				OutBuffer = (unsigned char * bank2)BD0IAL;
				OutBuffer [0] = USB_Curr_Config;
				BD0IBC = 1;
				BD0IST = 0xc8;
				break;

			case GET_DESCRIPTOR:
				DescriptorID = (unsigned char) (BufferCopy.wValue >> 8);
				if (DescriptorID == DEVICE)
				{
					USB_dev_req = GET_DESCRIPTOR;
					EP0_start = DeviceDescriptor;
					EP0_end   = DeviceDescriptor + sizeof(DeviceDescriptor);
					if (BufferCopy.wLength < (EP0_end - EP0_start))
						EP0_end = EP0_start + BufferCopy.wLength;
					EP0_maxLength = 8;
					CopyDescriptorToEP0 ();
				}
				else if (DescriptorID == CONFIGURATION)
				{
					USB_dev_req = GET_DESCRIPTOR;
					EP0_start = ConfigDescriptor;
					EP0_end   = ConfigDescriptor + sizeof(ConfigDescriptor);
					if (BufferCopy.wLength < (EP0_end - EP0_start))
						EP0_end = EP0_start + BufferCopy.wLength;
					EP0_maxLength = 8;
					CopyDescriptorToEP0 ();
				}
				else if (DescriptorID == STRING)
				{
					if (BufferCopy.wIndex != 0x409 && BufferCopy.wIndex != 0)  /* make sure language is supported */
						STALL_EP0;				/* only language 0 is valid */
					else
					{
					StringID = (unsigned char) BufferCopy.wValue;
					USB_dev_req = GET_DESCRIPTOR;
					EP0_maxLength = 8;
					switch (StringID)
					{
					case 0:
						EP0_start = String0;
						EP0_end   = String0 + String0[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 1:
						EP0_start = String1;
						EP0_end   = String1 + String1[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 2:
						EP0_start = String2;
						EP0_end   = String2 + String2[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 3:
						EP0_start = String3;
						EP0_end   = String3 + String3[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 4:
						EP0_start = String4;
						EP0_end   = String4 + String4[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 5:
						EP0_start = String5;
						EP0_end   = String5 + String5[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 6:
						EP0_start = String6;
						EP0_end   = String6 + String6[0];
						if (BufferCopy.wLength < (EP0_end - EP0_start))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					default:
						STALL_EP0;	/* request error */
					}
					}
				}
				break;

			case GET_STATUS:
				OutBuffer = (unsigned char *)BDT [EP0IN].address;
				OutBuffer[0] = USB_status_device;
				OutBuffer[1] = 0;
				BD0IBC = 2;
				BD0IST = 0xc8;
				break;

			default:
				break;
			}
			break;

		case INTERFACETOHOST:
			switch (BufferCopy.bRequest)
			{
			case GET_INTERFACE:
				Interface = BufferCopy.wIndex;
				if ((USWSTAT == CONFIG_STATE) && (Interface < NUM_INTERFACES))
				{
					OutBuffer = (unsigned char *) BDT [EP0IN].address;
					OutBuffer[0] = USB_Interface [Interface];
					BD0IBC = 1;
					BD0IST = 0xc8;
				}
				else
					STALL_EP0;
				break;

			case GET_STATUS:
				OutBuffer = (unsigned char *) BDT [EP0IN].address;
				OutBuffer[1] = 0;
				BD0IBC = 2;
				Interface = BufferCopy.wIndex;
				if ((USWSTAT == ADDRESS_STATE) && (Interface == 0))
				{
					OutBuffer[0] = USB_Interface [Interface];
					BD0IST = 0xc8;
				}
				else if ((USWSTAT == CONFIG_STATE) && (Interface < NUM_INTERFACES))
				{
					OutBuffer[0] = USB_Interface [Interface];
					BD0IST = 0xc8;
				}
				else
					STALL_EP0;
				break;

			case GET_DESCRIPTOR:
				DescriptorType = BufferCopy.wValue >> 8;
				if (DescriptorType == 0x22)  /* special HID request to return report descriptor */
				{
					USB_dev_req = GET_DESCRIPTOR;
					switch ((BufferCopy.wIndex | BufferCopy.wValue) & 0x0001) 
						// This should only be BufferCopy.wValue but Windows is out
						// of specification and looks at BufferCopy.wIndex instead 
						// for the descriptor index.  See 7.1.1 of HID Class definition.
					{
					case 0:
						EP0_start = ReportDescriptor1;
						EP0_end   = EP0_start + sizeof (ReportDescriptor1);
						EP0_maxLength = 8;
						if (BufferCopy.wLength < sizeof (ReportDescriptor1))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 1:
						EP0_start = ReportDescriptor2;
						EP0_end   = EP0_start + sizeof (ReportDescriptor2);
						EP0_maxLength = 8;
						if (BufferCopy.wLength < sizeof (ReportDescriptor2))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					}
				}
				else if (DescriptorType == 0x21) /* HID descriptor */
				{
					USB_dev_req = GET_DESCRIPTOR;
					switch ((BufferCopy.wIndex | BufferCopy.wValue) & 0x0001)
						// This should only be BufferCopy.wValue but Windows is out
						// of specification and looks at BufferCopy.wIndex instead 
						// for the descriptor index.  See 7.1.1 of HID Class definition.
					{
					case 0:
						EP0_start = HIDDescriptor1;
						EP0_end   = EP0_start + sizeof (HIDDescriptor1);
						EP0_maxLength = 8;
						if (BufferCopy.wLength < sizeof (HIDDescriptor1))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					case 1:
						EP0_start = HIDDescriptor2;
						EP0_end   = EP0_start + sizeof (HIDDescriptor2);
						EP0_maxLength = 8;
						if (BufferCopy.wLength < sizeof (HIDDescriptor2))
							EP0_end = EP0_start + BufferCopy.wLength;
						CopyDescriptorToEP0 ();
						break;
					}
				}
				else
					STALL_EP0;  /* unrecognised request */
				break;

			default:
				break;
			}
			break;

		case ENDPOINTTOHOST:
			if (BufferCopy.bRequest == GET_STATUS)
			{
				UEPArray = (unsigned char *) &UEP0;
				Endpoint = BufferCopy.wIndex & 0x0F;
				OutBuffer = (unsigned char * ) BD0IAL;
				OutBuffer[1] = 0;
				BD0IBC = 2;

				if (Endpoint < 3)
				{
					OutBuffer[0] = UEPArray [Endpoint] & 0x01;
					BD0IST = 0xc8;
				}
				else
					STALL_EP0;
			}
			break;

		default:
			if (BufferCopy.bmRequestType & 0x20) {
				switch (BufferCopy.bmRequestType)
				{
				case	HID_SET_REPORT:	/* Host to Device HID request */
					switch (BufferCopy.bRequest)
					{
					case 	0x09:	/* Set HID Report */
					    USB_dev_req = HID_SET_REPORT;
						break;
					case 	0x0A:	/* Set Idle */
					    Send_0Len_pkt;
						break;
					case 	0x0B:	/* Set Protocol */
					default:
						STALL_EP0;
					}
					break;
				case	HID_GET_REPORT:	/* Dev2HostHIDRequest */
					switch (BufferCopy.bRequest)
					{
					case 	0x01:	/* Get HID Report */
					    USB_dev_req = HID_GET_REPORT;
			    		// Add Get HID Report Function here
			    		BD0IST = 0xc8;
						break;
					case 	0x02:	/* Get Idle */
					case 	0x03:	/* Get Physical */
					case 	0x06:	/* Get Report Descriptor */
					default:
						STALL_EP0;
					}
					break;
				case	0x22:	/* Host2DevReportRequest */
				case	0x23:	/* Host2DevPhysicalRequest */
				case	0xA2:	/* Dev2HostReportRequest */
				case	0xA3:	/* Dev2HostPhysicalRequest */
				default:
					STALL_EP0;
				}		
			}
		}
	}
}



