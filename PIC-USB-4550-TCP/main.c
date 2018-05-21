//PROGRAMM FOR BLINKING LED
//
// PROCESSOR : PIC18F4550
// CLOCK	 : 20MHz, EXTERNAL



// PIC18F4550 Configuration Bit Settings

// 'C' source line config statements

// CONFIG1L
#pragma config PLLDIV = 1       // PLL Prescaler Selection bits (No prescale (4 MHz oscillator input drives PLL directly))
#pragma config CPUDIV = OSC1_PLL2// System Clock Postscaler Selection bits ([Primary Oscillator Src: /1][96 MHz PLL Src: /2])
#pragma config USBDIV = 1       // USB Clock Selection bit (used in Full-Speed USB mode only; UCFG:FSEN = 1) (USB clock source comes directly from the primary oscillator block with no postscale)

// CONFIG1H
// OK #pragma config FOSC = INTOSC_HS // Oscillator Selection bits (Internal oscillator, HS oscillator used by USB (INTHS))
#pragma config FOSC = HS		// Oscillator Selection bits (HS oscillator (HS))
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable bit (Fail-Safe Clock Monitor enabled)
#pragma config IESO = OFF       // Internal/External Oscillator Switchover bit (Oscillator Switchover mode disabled)

// CONFIG2L
#pragma config PWRT = OFF       // Power-up Timer Enable bit (PWRT disabled)
#pragma config BOR = ON         // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware only (SBOREN is disabled))
#pragma config BORV = 3         // Brown-out Reset Voltage bits (Minimum setting 2.05V)
#pragma config VREGEN = OFF     // USB Voltage Regulator Enable bit (USB voltage regulator disabled)

// CONFIG2H
#pragma config WDT = OFF        // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
#pragma config WDTPS = 32768    // Watchdog Timer Postscale Select bits (1:32768)

// CONFIG3H
#pragma config CCP2MX = ON      // CCP2 MUX bit (CCP2 input/output is multiplexed with RC1)
#pragma config PBADEN = ON      // PORTB A/D Enable bit (PORTB<4:0> pins are configured as analog input channels on Reset)
#pragma config LPT1OSC = OFF    // Low-Power Timer 1 Oscillator Enable bit (Timer1 configured for higher power operation)
#pragma config MCLRE = ON       // MCLR Pin Enable bit (MCLR pin enabled; RE3 input pin disabled)

// CONFIG4L
#pragma config STVREN = ON      // Stack Full/Underflow Reset Enable bit (Stack full/underflow will cause Reset)
#pragma config LVP = OFF        // Single-Supply ICSP Enable bit (Single-Supply ICSP disabled)
#pragma config ICPRT = OFF      // Dedicated In-Circuit Debug/Programming Port (ICPORT) Enable bit (ICPORT disabled)
#pragma config XINST = OFF      // Extended Instruction Set Enable bit (Instruction set extension and Indexed Addressing mode disabled (Legacy mode))

// CONFIG5L
#pragma config CP0 = OFF        // Code Protection bit (Block 0 (000800-001FFFh) is not code-protected)
#pragma config CP1 = OFF        // Code Protection bit (Block 1 (002000-003FFFh) is not code-protected)
#pragma config CP2 = OFF        // Code Protection bit (Block 2 (004000-005FFFh) is not code-protected)
#pragma config CP3 = OFF        // Code Protection bit (Block 3 (006000-007FFFh) is not code-protected)

// CONFIG5H
#pragma config CPB = OFF        // Boot Block Code Protection bit (Boot block (000000-0007FFh) is not code-protected)
#pragma config CPD = OFF        // Data EEPROM Code Protection bit (Data EEPROM is not code-protected)

// CONFIG6L
#pragma config WRT0 = OFF       // Write Protection bit (Block 0 (000800-001FFFh) is not write-protected)
#pragma config WRT1 = OFF       // Write Protection bit (Block 1 (002000-003FFFh) is not write-protected)
#pragma config WRT2 = OFF       // Write Protection bit (Block 2 (004000-005FFFh) is not write-protected)
#pragma config WRT3 = OFF       // Write Protection bit (Block 3 (006000-007FFFh) is not write-protected)

// CONFIG6H
#pragma config WRTC = OFF       // Configuration Register Write Protection bit (Configuration registers (300000-3000FFh) are not write-protected)
#pragma config WRTB = OFF       // Boot Block Write Protection bit (Boot block (000000-0007FFh) is not write-protected)
#pragma config WRTD = OFF       // Data EEPROM Write Protection bit (Data EEPROM is not write-protected)

// CONFIG7L
#pragma config EBTR0 = OFF      // Table Read Protection bit (Block 0 (000800-001FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR1 = OFF      // Table Read Protection bit (Block 1 (002000-003FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR2 = OFF      // Table Read Protection bit (Block 2 (004000-005FFFh) is not protected from table reads executed in other blocks)
#pragma config EBTR3 = OFF      // Table Read Protection bit (Block 3 (006000-007FFFh) is not protected from table reads executed in other blocks)

// CONFIG7H
#pragma config EBTRB = OFF      // Boot Block Table Read Protection bit (Boot block (000000-0007FFh) is not protected from table reads executed in other blocks)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#include <xc.h>



//#include	<p18cxxx.h>

#define		BUT			PORTBbits.RB4
#define		LED_ON		PORTDbits.RD3 = 1
#define		LED_OFF		PORTDbits.RD3 = 0
#define		TOGGLE_LED	PORTD ^= 0x08
#define		INPUT		1
#define		OUTPUT		0
#define		LED_DIR		TRISDbits.TRISD3
#define		BUT_DIR		TRISBbits.TRISB4



static int _Timer0Count = 0;


///////////////////////////////////////////////////////////////////////////////
//	Timer0

//-----------------------------------------------------------------------------
//	_Timer0Config
//-----------------------------------------------------------------------------
//	Initialize Timer0 for 1-mSec interrupt
//	The Timer0 module is an 8-bit timer/counter
//
//=============================================================================

// LEGACY:	1:256 prescaler for a delay of: (insruction-cycle * 256-counts)*prescaler = ((8uS * 256)*256) =~ 524mS

static void _Timer0Config(void)
{
	//	T0CS: Timer0 Clock Source Select bit
	//	1 = Transition on T0CKI pin
	//	0 = Internal instruction cycle clock (CLKO)
	T0CONbits.T0CS = 0;
	
	//	PSA: Timer0 Prescaler Assignment bit
	//	1 = TImer0 prescaler is NOT assigned. Timer0 clock input bypasses prescaler.
	//	0 = Timer0 prescaler is assigned. Timer0 clock input comes from prescaler output.
	T0CONbits.PSA = 0;
	
	//	PS<2:0>: Prescaler Rate Select bits
	//		000	= 1 : 2
	//		001 = 1 : 4
	//		010 = 1 : 8
	//		011 = 1 : 16
	//		100 = 1 : 32
	//		101 = 1 : 64
	//		110 = 1 : 128
	//		111 = 1 : 256
	T0CONbits.T0PS = 0b100;

	return;
}

//-----------------------------------------------------------------------------
//	_Timer0Start
//-----------------------------------------------------------------------------
//	Start the timer 
//
//=============================================================================

#define 	TIMER0_INIT	(0x06)

static void _Timer0Start(void)
{
	//	Clear the Timer0 Interrupt Flag
	INTCONbits.TMR0IF = 0;
	
	//	Initialize the timer register (Timer0)
	TMR0 = TIMER0_INIT;
	
	//	Enable the Timer0 rollover interrupt
    INTCONbits.TMR0IE = 1;
	return;
}


//-----------------------------------------------------------------------------
//	_Timer0ISR()
//
//	This is a one-millisecond timer interrupt.
//
//=============================================================================
static void _Timer0ISR(void)
{
	//	Initialize the timer register (Timer0)
	TMR0 = TIMER0_INIT;		
	_Timer0Count++;

	//	Start the ADC conversion by setting the GO/DONE bit.
//	ADCON0bits.GO = 1;
	
	return;
}


void interrupt ISR(void) 
{
	if(INTCONbits.TMR0IF)	//	Timer0 Interrupt Flag
	{
		//	Clear the Timer0 Interrupt Flag
		INTCONbits.TMR0IF = 0;
		_Timer0ISR();
	}
	
//	if(PIR1bits.ADIF)		//	ADC Interrupt Flag
//	{
//		PIR1bits.ADIF = 0;
//		_AdcISR();
//	}
		
}					





//Just simple delay
void Delay(unsigned long cntr) {
	while (--cntr != 0);
}

// Main function
void main(void) {
	

	INTCON 	= 0x0;			// Disable interupt
	CMCON 	= 0x07;		    // Comparators Off
	ADCON1  = 0x06;			// Port as Digital IO
//	RDPU    = 0;			// Disable pull ups
	PORTEbits.RDPU = 0;		// Disable pull ups	
	CCP1CON = 0; 			// P1B, P1C, P1D assigned as port pins
	LED_DIR = OUTPUT;		// Led pin as output	
	BUT_DIR = INPUT;		// Button pin as insput

	
	_Timer0Config();
	_Timer0Start();
	INTCONbits.GIE = 1;		//	Enables all unmasked interrupts
	
	// loop forever - echo 
	while(1) {

		// Toggle led
		TOGGLE_LED;
		// Simple delay
        while(_Timer0Count < 1000);
        _Timer0Count = 0;
//		Delay(50000);
	}	
}



