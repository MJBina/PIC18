//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//
//	$Workfile: dsPIC30_serial.c $
//	$Author: A1021170 $
//	$Date: 2015-02-09 08:24:32-06:00 $
//	$Revision: 18 $ 
//
//	Project:
//
//	Description:
// 		PROCESSOR : dsPIC30F4011
// 		CLOCK     : (~30MIPS) 7.3728MHz, EXTERNAL, PLLx16
//				
//
//	TODO:
//
//	
//=============================================================================


#include "dsPIC30_serial.h"

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>


#ifdef OPTION_UART1
	#define		_UxRXInterrupt	_U1RXInterrupt
	#define		_UxTXInterrupt	_U1TXInterrupt
	#define		UxRXIF			IFS0bits.U1RXIF
	#define		UxTXIF			IFS0bits.U1TXIF
	#define		UxTXEN			U1STAbits.UTXEN
	#define		UxTXREG			U1TXREG
	#define		UxRXREG			U1RXREG
	#define		UxPDSEL			U1MODEbits.PDSEL
	#define		UxSTSEL			U1MODEbits.STSEL
	#define		UxBRG			U1BRG
	#define		UxUTXISEL		U1STAbits.UTXISEL
	#define		UxURXISEL		U1STAbits.URXISEL
	#define		UxTXIE			IEC0bits.U1TXIE
    #define		UxRXIE			IEC0bits.U1RXIE
	#define		UxUARTEN		U1MODEbits.UARTEN
	#define		UxTRMT			U1STAbits.TRMT
	#define		UxTXBF 			U1STAbits.UTXBF
#else 
	#define		_UxRXInterrupt	_U2RXInterrupt
	#define		_UxTXInterrupt	_U2TXInterrupt
	#define		UxRXIF			IFS1bits.U2RXIF
	#define		UxTXIF			IFS1bits.U2TXIF
	#define		UxTXEN			U2STAbits.UTXEN
	#define		UxTXREG			U2TXREG
	#define		UxRXREG			U2RXREG
	#define		UxPDSEL			U2MODEbits.PDSEL
	#define		UxSTSEL			U2MODEbits.STSEL
	#define		UxBRG			U2BRG
	#define		UxUTXISEL		U2STAbits.UTXISEL
	#define		UxURXISEL		U2STAbits.URXISEL
	#define		UxTXIE			IEC1bits.U2TXIE
    #define		UxRXIE			IEC1bits.U2RXIE
	#define		UxUARTEN		U2MODEbits.UARTEN
	#define		UxTRMT			U2STAbits.TRMT
	#define		UxTXBF 			U2STAbits.UTXBF
#endif

#define TXBUF_SIZE	0x020
#define TXBUF_MASK	0x01F
uint8_t TxBuf[TXBUF_SIZE];

#define RXBUF_SIZE	0x010
#define RXBUF_MASK	0x00F
uint8_t RxBuf[RXBUF_SIZE];


typedef struct tagFIFOBUF
{
	uint8_t * buf;
	uint16_t ofst;
	int16_t	head;
	int16_t	tail;
	int16_t free_space;	
} FIFOBUF_t;

FIFOBUF_t	TxFifo;
FIFOBUF_t	RxFifo;


//-----------------------------------------------------------------------------
//	_UxRXInterrupt()
//
//=============================================================================

void __attribute__ ((interrupt, no_auto_psv)) _UxRXInterrupt(void) 
{
	*((uint8_t *)(RxFifo.buf + RxFifo.tail++)) = UxRXREG;
	RxFifo.tail &= RXBUF_MASK; 
	RxFifo.free_space--;
	UxRXIF = 0;
}


//-----------------------------------------------------------------------------
//	_UxTXInterrupt()
//
//=============================================================================

void __attribute__ ((interrupt, no_auto_psv)) _UxTXInterrupt(void) 
{
	UxTXIF = 0;	// Clear UART2 TX Interrupt Flag

	//	If there is nothing to transmit, then just return
	if ( TXBUF_SIZE == TxFifo.free_space )
	{
		return;
	}

	//	The transmit buffer is 9 bits wide and 4 characters deep. Including 
	//	the Transmit Shift register (UxTSR), the user effectively has a 5-deep 
	//	FIFO (First In First Out) buffer. The UTXBF Status bit (UxSTA<9>)
	//	indicates whether the transmit buffer is full.
	
	while (( TXBUF_SIZE > TxFifo.free_space ) && (UxTXBF == 0))
	{
		UxTXEN = 1;	//	Transmit Enable bit
		UxTXREG = (uint8_t)*((uint8_t *)(TxFifo.buf + TxFifo.head++));
		TxFifo.head &= TXBUF_MASK;
		TxFifo.free_space++;
	}
}

//-----------------------------------------------------------------------------
//	_serial_Config()
//
//=============================================================================

void _serial_Config(void)
{
	//	The buffer is empty when the head and tail point to the same location.
	TxFifo.buf = &TxBuf[0];
	TxFifo.ofst = 0;
	TxFifo.tail = TxFifo.head = 0;
	TxFifo.free_space = TXBUF_SIZE;

	RxFifo.buf = &RxBuf[0];
	RxFifo.ofst = 0;
	RxFifo.tail = RxFifo.head = 0;
	RxFifo.free_space = RXBUF_SIZE;

	
	//-- UART2 ----------------------------------------------------------------
	//	From the dsPIC30F4011 datasheet, 18.3.1 TRANSMITTING IN 8-BIT DATA MODE
	//	The following steps must be performed in order to transmit 8-bit data:
	//	 1. Set up the UART:
	//		First, the data length, parity and number of Stop bits must be 
	//		selected. Then, the transmit and receive interrupt enable and 
	//		priority bits are set up in the UxMODE and UxSTA registers. Also,
	//		the appropriate baud rate value must be written to the UxBRG reg.
	//	 2. Enable the UART by setting the UARTEN bit (UxMODE<15>).
	//		Once enabled, the UxTX and UxRX pins are configured as an output 
	//		and an input, respectively, overriding the TRIS and LATCH register 
	//		bit settings for the corresponding I/O port pins. The UxTX pin is 
	//		at logic ‘1’ when no transmission is taking place.
	//	 3. Set the UTXEN bit (UxSTA<10>), thereby enabling a transmission.
	//	 4. Write the byte to be transmitted to the lower byte of UxTXREG. The 
	//		value will be transferred to the Transmit Shift register (UxTSR) 
	//		immediately and the serial bit stream will start shifting out during 
	//		the next rising edge of the baud clock. 
	//		Alternatively, the data byte may be written while UTXEN = 0, 
	//		following which, the user may set UTXEN. This will cause the serial 
	//		bit stream to begin immediately because the baud clock will start 
	//		from a cleared state.
	//	 5. A transmit interrupt will be generated depending on the value of 
	//		the interrupt control bit UTXISEL (UxSTA<15>).
	//	
	// 	This was copied from an example, as a place to start.  
	// 	The Modified Olimex board has a FTDI RS232 to USB converter wired to 
	// 	UART2, so we will be configuring that port.

	//	UxMODE: UART2 Mode Register
	UxPDSEL = 0;	//	00 = 8-bit, No Parity
	UxSTSEL = 0;	// 	One Stop Bit
	
	//	UART2: Baud Rate Generator (U2BRG) - See section 19.3.1 of datasheet.
	//  U2BRG = (Fcy/(16*BaudRate))-1	
	//	Baud Rate Table for 40 MIPS			|	for ~30 MIPS
	//		Kbaud	BRG		Calc.	%error	|	BRG		Calc.  %error
	//		1200	2082	1200	 0.00%	|  1535	    1200	0.00%
	//		2400	1040	2401	 0.04%	|   767	    2400    0.00%
	//		4800	519		4807	 0.15%	|   383	    4800    0.00%
	//		9600	259		9615	 0.16%	|   191	    9600    0.00%
	//		19200	129		19230	 0.16%	|    95	   19200    0.00%
	//		38400	 64		38461	 0.16%	|    47	   38400    0.00%
	//		115200	 20		119047	 3.34%	|    15	  115200    0.00%
	//		115200	 21		113636	-1.36%	|    
	//		230400	  9		250000	 8.51%	|     7	  230400    0.00%
	//		230400	 10		227272	-1.36%	|     
	
	
	
#ifdef OPTION_BAUD_9600
	#ifdef OPTION_UART1
		#warning UART1 configured for 9600 Baud 
	#else
		#warning UART2 configured for 9600 Baud 
	#endif
	UxBRG = 191;	//	9600 Baud with -0% error (~30MIPS)
#else
	#ifdef OPTION_UART1
		#warning UART1 configured for 115200 Baud 
	#else
		#warning UART2 configured for 115200 Baud 
	#endif
	UxBRG = 15;		//	115200 Baud with -0% error (~30MIPS)
#endif

	//	UxSTA: UARTx Status and Control Register
	// 	If UTXISEL = 0, an interrupt is generated when a word is transferred 
	//		this from the transmit buffer to the Transmit Shift register UxTSR.
	//		This implies that the transmit buffer has at least one empty word.
	//	If UTXISEL = 1, an interrupt is generated when a word is transferred 
	//		from the transmit buffer to the Transmit Shift register (UxTSR) and 
	//		the transmit buffer is empty.
	UxUTXISEL = 1;	
	
	UxURXISEL = 1;	//	Receive Interrupt Mode Selection bit

	return;
}


//-----------------------------------------------------------------------------
//	_serial_Start()
//
//=============================================================================

void _serial_Start(void)
{
	// Clear UART2 interrupts flags
	UxTXIF = 0;	// Clear the Transmit Interrupt Flag
	UxRXIF = 0;	// Clear the Recieve Interrupt Flag

	// Enable UART2all interrupts
	UxTXIE = 1;	// Enable Transmit Interrupts
	UxRXIE = 1;	// Enable Recieve Interrupts

	//	Once enabled, the UxTX and UxRX pins are configured as output and 
	//	input, respectively, overriding the TRIS and PORT register bit 
	//	settings for the corresponding I/O port pins. 
	UxUARTEN = 1;	// 	Turn UART2 on
	
	return;
}


//-----------------------------------------------------------------------------
//	_serial_TxBuf
//
//=============================================================================

int16_t _serial_TxBuf( unsigned char *data, int len )
{
	int i;

	
	if (len > TxFifo.free_space)
	{
		return(ERR_BUF_FULL);
	}	

	for(i=0; i<len; i++)
	{
		*((uint8_t *)(TxFifo.buf + TxFifo.tail++)) = *data++;
		TxFifo.tail &= TXBUF_MASK; 
		TxFifo.free_space--;
	}
	
	//	If the transmit shift register is empty, then transmit the first byte.
	if ( UxTRMT == 1)
	{
		UxTXIF = 1;	// Set UART2 TX Interrupt Flag
	}
	
	return(SUCCESS);
}


//-----------------------------------------------------------------------------
//	_serial_printf()
//
//	Return Value
//		On success, the total number of characters written is returned.
//		On failure, a negative number is returned.
//=============================================================================

#ifndef _OMIT_PRINTF

int16_t _serial_printf (char * fmt, ...)
{
	char buf[256];
	va_list args;
	int	len;
	

	va_start (args, fmt);
	len = vsprintf (buf, fmt, args);
	if (len > 0 )
	{
		if (SUCCESS != _serial_TxBuf ((unsigned char *)buf, len));
		{
			len = -1;
		}
	}
	va_end (args);
	
	return(len);
}

#endif	//	_OMIT_PRINTF


//-----------------------------------------------------------------------------
//	_serial_getc()
//
//	Return Value
//		On success, the character read is returned (promoted to an int value).
//		On failure, EOF.
//=============================================================================

int16_t _serial_getc(void)
{
	static int16_t ch;

	
	if ( RXBUF_SIZE == RxFifo.free_space )
	{
		return(EOF);
	}
	
	ch = (int16_t)*((uint8_t *)(RxFifo.buf + RxFifo.head++));
	RxFifo.head &= RXBUF_MASK;
	RxFifo.free_space++;
	return(ch);
}

//-----------------------------------------------------------------------------
//	_serial_TxBE() - Transmit Buffer Empty
//
//	Return Value
//		Zero if Transmit Buffer has data
//		Non-zero if Transmit Buffer is empty
//=============================================================================

int16_t _serial_TxBE(void)
{
	if(TXBUF_SIZE == TxFifo.free_space ){
		return(1);
	}
	return(0);
}

//-----------------------------------------------------------------------------
//	_serial_putc(int16_t ch)
//
//	'ch' is the integer promotion of the character to be written.
//	The value is internally converted (back) to an unsigned char when written.
//
//	Return Value
//		On success, the character written is returned.
//		On failure, EOF.
//=============================================================================
int16_t _serial_putc(char ch)
{
	if (1 > TxFifo.free_space)
	{
		return(EOF);
	}	

	*((uint8_t *)(TxFifo.buf + TxFifo.tail++)) = (int16_t)ch;
	TxFifo.tail &= TXBUF_MASK; 
	TxFifo.free_space--;
	
	UxTXIF = 1;	// Set UART2 TX Interrupt Flag
	
	return(ch);
}



// EOF
