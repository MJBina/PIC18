//-----------------------------------------------------------------------------
// File:   hw.h
// Author: Mark Bina
// Created on May 18, 2018, 10:59 AM
//
// NOTE:    Always use the INLINE macro to achieve proper results
//-----------------------------------------------------------------------------

#ifndef _INLINE_H_    // include only once
#define _INLINE_H_

// IMPORTANT: You must have the option checked in "xc16-gcc\Optimizations"
//              Do not override inline [X]    (it is off by default)
// "always_inline" makes it inline even if optimization is turned off

#define INLINE static inline __attribute__((always_inline))

#endif // _INLINE_H_

// EOF