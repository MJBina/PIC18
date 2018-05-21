//PROGRAMM FOR BUTTON and LED
//
// PROCESSOR : PIC18F4550
// CLOCK	 : 20MHz, EXTERNAL

#include	<p18cxxx.h>
/** V E C T O R  R E M A P P I N G *******************************************/


#define		BUT			PORTBbits.RB4
#define		BUT_DIR		TRISBbits.TRISB4

#define		LED_ON		PORTDbits.RD3 = 1
#define		LED_OFF		PORTDbits.RD3 = 0
#define		LED_DIR		TRISDbits.TRISD3

#define		INPUT		1
#define		OUTPUT		0

extern void _startup (void);        // See c018i.c in your C18 compiler dir
#pragma code _RESET_INTERRUPT_VECTOR = 0x000800
void _reset (void)
{
    _asm goto _startup _endasm
}
#pragma code

#pragma code _HIGH_INTERRUPT_VECTOR = 0x000808
void _high_ISR (void)
{
    ;
}

#pragma code _LOW_INTERRUPT_VECTOR = 0x000818
void _low_ISR (void)
{
    ;
}
#pragma code


//Just simple delay
void Delay(unsigned long cntr) {
	while (--cntr != 0);
}

// Main function
void main(void) 
{
	INTCON 	= 0x0;			// Disable inerupt
	CMCON 	= 0x07;		    // Comparators Off
	ADCON1  = 0x06;			// Port as Digital IO
	PORTEbits.RDPU = 0;		// Disable pull ups	
	CCP1CON = 0; 			// P1B, P1C, P1D assigned as port pins
	LED_DIR = OUTPUT;		// Led pin as output	
	BUT_DIR = INPUT;		// Button pin as insput



	// loop forever
	while(1) {

		// Read button and on/off led
		if(BUT) 
			LED_OFF;
		else 
			LED_ON;

	}	
}