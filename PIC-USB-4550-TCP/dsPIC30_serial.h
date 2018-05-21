//-----------------------------------------------------------------------------
//	Copyright(C) 2012 - Sensata Technologies, Inc.  All rights reserved.
//
//	$Workfile: dsPIC30_serial.h $
//	$Author: A1021170 $
//	$Date: 2014-09-30 07:51:53-05:00 $
//	$Revision: 7 $ 
//
//	Project:
//
//	Description:
// 		PROCESSOR : dsPIC30F4011
// 		CLOCK     : 10MHz, EXTERNAL
//
//	TODO:
//
//	
//=============================================================================

#ifndef __DSPIC30_SERIAL_H
#define __DSPIC30_SERIAL_H

//	The function serial_printf() calls variants of the printf() function which 
//	have substantial resource requirements. It was determoned that including 
//	serial_printf() requires an additional 3321 bytes Flash and 188 bytes RAM.
//	Alternatives should be used for production code.
//	#define _OMIT_PRINTF
#undef _OMIT_PRINTF


#include <stdint.h>


#ifndef EOF
#define	EOF	(-1)
#endif	//	EOF


extern int16_t _serial_getchar(void);

extern void _serial_Config(void);

extern void _serial_Start(void);

extern int16_t _serial_TxBuf( unsigned char *data, int len );

extern int16_t _serial_TxBE(void);

extern int16_t _serial_getc(void);

extern int16_t _serial_putc(char ch);


#ifndef _OMIT_PRINTF

extern int16_t _serial_printf(char * fmt, ...);

#endif	//	_OMIT_PRINTF


//	serial_printf is a VARIADIC macro, accepting optional multiple arguments. 
//	For a good explanation of variadic macros, refer to the link:
//		http://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html#Variadic-Macros

#define serial_Config()	_serial_Config()
#define serial_Start() 	_serial_Start()
#define serial_TxBuf(data, len) _serial_TxBuf(data, len)
#define serial_TxBE() 	_serial_TxBE()
#define serial_getc()	_serial_getc()
#define serial_putc(ch)	_serial_putc(ch)

#ifdef _OMIT_PRINTF
	#warning serial_printf() is undefined - refer to dsPIC30_serial.h for details.
	#define serial_printf(format, ...)		//	(format, ##__VA_ARGS__)
#else
	//  #define serial_printf(format, ...) 	_serial_printf(format, ##__VA_ARGS__)
	#define serial_printf 	_serial_printf
#endif


#endif //	__DSPIC30_SERIAL_H

// EOF
