/* 
 * File:   hw.h
 * Author: Mark Bina
 *
 * Created on May 18, 2018, 10:59 AM
 */

#ifndef HW_H
#define	HW_H

#ifdef	__cplusplus
extern "C" {
#endif


#include	<p18cxxx.h>

#define		BUT			PORTBbits.RB4
#define		LED_ON		PORTDbits.RD3 = 1
#define		LED_OFF		PORTDbits.RD3 = 0
#define		TOGGLE_LED	PORTD ^= 0x08
#define		INPUT		1
#define		OUTPUT		0
#define		LED_DIR		TRISDbits.TRISD3
#define		BUT_DIR		TRISBbits.TRISB4

    
#ifdef	__cplusplus
}
#endif

#endif	/* HW_H */

