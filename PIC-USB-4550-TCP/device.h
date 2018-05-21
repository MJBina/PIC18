// <><><><><><><><><><><><><> device.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//	Standard products include the Inverter/Charger and DC+DC Converter. These 
//	components provide energy conversion functionality, the essence of these are
//	described below:
//	  * An 'Inverter' converts DC power to AC power.
//	  * A 'Charger' provides DC current to a battery.  Typically, it converts 
//		AC power to DC power.
//	  * A DC+DC Converter converts DC power, of one voltage to DC power of a
//		different voltage.
//	The 'Device' supports these essential functions, and others which are not
//	necessarily native to those functions. For example, transfer functions, or 
//	enable/disable controls. 
//		
//-----------------------------------------------------------------------------

#ifndef __DEVICE_H    // include only once
#define __DEVICE_H

// -------
// headers
// -------
#include "options.h"    // must be first include

//-----------------------------------------------------------------------------
//                            E N U M S
//-----------------------------------------------------------------------------

typedef enum 
{
	REMOTE_DISABLED  = 0,
	REMOTE_SNAP      = 1,
	REMOTE_MOMENTARY = 2,
    REMOTE_CUSTOM    = 3
} REMOTE_MODE_t;


typedef enum 
{
	AUX_DISABLED = 0,
	AUX_RV       = 1,
	AUX_UTILITY  = 2,
	AUX_CONTROL  = 3
} AUX_MODE_t;


//-----------------------------------------------------------------------------
//                    D E F A U L T    V A L U E S
//-----------------------------------------------------------------------------
//	LP15: Forced configuration for now 
//	TODO: Determine better method for setting option details.
//	NOTE: This method assures that these settings would be applied if a failure
//		or command caused the defaults to be loaded from ROM.  
//		When updated, see additional, similar configuration in inverter.h.
//-----------------------------------------------------------------------------

#if defined(MODEL_12LP15)		
  //	Charger is disabled (hardware not present)
  //  Load-Sensing is disabled.
  //  Shutdown Timer is disabled.
  //  Remote On/Off Mode = Snap : Zero-Current draw when Off/Low
  //  Auxiliary Mode = Disabled : Not using the Auxiliary input

  #define DEVICE_DFLT_INVERTER_ENABLE  	  (1)
  #define DEVICE_DFLT_CHARGER_ENABLE  	  (0)
  #define DEVICE_DFLT_PASS_THRU_ENABLE    (0)
  #define DEVICE_DFLT_TMR_SHUTDOWN_ENABLE (0)
  
  #define DEVICE_DFLT_PUSHBUTTON_ENABLE	  (0)
  #define DEVICE_DFLT_REMOTE_MODE		  (REMOTE_SNAP)
  #define DEVICE_DFLT_AUX_MODE			  (AUX_DISABLED)

#else
  #define DEVICE_DFLT_INVERTER_ENABLE  	  (1)
  #define DEVICE_DFLT_CHARGER_ENABLE  	  (1)
  #define DEVICE_DFLT_PASS_THRU_ENABLE    (1)
  #define DEVICE_DFLT_TMR_SHUTDOWN_ENABLE (1)
  
  #define DEVICE_DFLT_PUSHBUTTON_ENABLE	  (0)
  #define DEVICE_DFLT_REMOTE_MODE		  (REMOTE_SNAP)
  #define DEVICE_DFLT_AUX_MODE			  (AUX_DISABLED)
#endif

#define DEVICE_DFLT_TMR_SHUTDOWN_DELAY 	((int16_t)(5*60*2))	// 5-min as .5-sec increments

#define DEVICE_DFLT_AC_LINE_QUAL_DELAY	((int16_t)(30*1000))	// 30-seconds as milliseconds
//	#define DEVICE_DFLT_AC_LINE_QUAL_DELAY	((int16_t)(2*1000))	// 2-seconds as milliseconds	FOR DEVELOPMENT TESTING ONLY


//-----------------------------------------------------------------------------
//                   1 5    V O L T    R E G U L A T O R
//-----------------------------------------------------------------------------
//	The signal '15V_RAIL_RESET' is the output of the 15VDC regulator what feeds 
//	the FET drivers and 3.3VDC regulator(s). Since this is a regulated output, 
//	it is difficult to collect more than a few datapoints for calibrating the 
//	A2D readings.  Very rough testing on an LPC15 showed that nominal output 
//	produced an A2D count of about 720 (0x02D0). 
//
//	The following assumptions are made to choose a cutoff threshold:
//			A2D is linear (Slope = m, Intercept = 0), 0VDC == zero A2D counts
//			y = mx + b, if b=0, then y = mx, where y = A2D counts, and x = volts
//			m = y/x = 720/15 = 48.0 A2D counts/volt
//-----------------------------------------------------------------------------

#define	VREG15_SLOPE		(48.0)
#define	VREG15_INTERCEPT	(0)

//	Cutoff at 10.0VDC (using 10.5VDC due to inherent approx 2mS latency)
#define	VREG15_LOW_SHUTDOWN ((int16_t)(10.5 * VREG15_SLOPE) + VREG15_INTERCEPT)

//	To accomodate recovery, we need hysteresis.
#define	VREG15_LOW_RECOVER 	((int16_t)(12.5 * VREG15_SLOPE) + VREG15_INTERCEPT)


//-----------------------------------------------------------------------------
//                     D E V I C E   S T A T U S
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	union 
	{
		struct
		{
			unsigned all_flags			: 15;
			unsigned unused             : 1;
		};
		struct
		{
			// --------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
			// --------------------------------------------------
			unsigned inv_request        : 1;    //  0x0001  
			unsigned chgr_request		: 1;	//	0x0002
			unsigned relay_request		: 1;	//	0x0004
			unsigned inv_enabled   		: 1;    //  0x0008	Inverter is enabled by RVC command
		  
			unsigned chgr_enable_can	: 1;	//	0x0010	Charger is enabled by RVC command
			unsigned chgr_enable_bcr	: 1;	//	0x0020	branch circuit rating is non-zero
			unsigned chgr_enable_jmpr	: 1;	//	0x0040	enable jumper in place
			unsigned chgr_enable_time   : 1;	//  0x0080	1 = charge timeout has occurred
			
			unsigned pass_thru_enabled	: 1;  	//  0x0100	xfer enabled by RVC command
			unsigned ac_line_qualified  : 1;	//	0x0200
			unsigned chgr_relay_active  : 1;	//	0x0400
			unsigned xfer_relay_active  : 1;	//	0x0800
			
			unsigned tmr_shutdown_enabled : 1;	//	0x1000 Timer Shutdown is enabled
			unsigned tmr_shutdown		: 1;    //  0x2000 Timer Shutdown occurred
			unsigned ac_line_valid      : 1;    //  0x4000 120V_LINE_OK_IN (see LPC schematic)
			unsigned unused2            : 1;
		};
	};
    int16_t batt_temp;       		 //  Battery Temperature degrees-C
    int16_t hs_temp;       			 //  Heatsink Temperature degrees-C
    int16_t tmr_shutdown_timer;  	 //  shutdown when this timer expires
	int16_t	ac_line_qual_timer_msec; // Countdown timer - zero when ac_line_qualified
    uint8_t inv_disabled_source;     // 0=CAN, 1=LCD
} DEVICE_STATUS_t;
#pragma pack()  // restore packing


//-----------------------------------------------------------------------------
//                      D E V I C E   C O N F I G
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	union
	{
	    struct
		{
			unsigned all_flags			: 10;
			unsigned unused             : 6;
		};
		struct
		{
			// --------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
			// --------------------------------------------------
            // DEVICE_CFG_ALL_FLAGS() depends upon this order
			unsigned inv_enabled   			: 1;    //  Inverter enabled on startup
			unsigned chgr_enabled  			: 1;    //  Charger enabled on startup
			unsigned pass_thru_enabled		: 1;	//	Pass-Thru enabled on startup
			unsigned tmr_shutdown_enabled	: 1;	//  Timer Shutdown is enabled on startup
			
			unsigned remote_mode			: 2;	//	0=Disabled, 1=Snap, 2=Momentary, 3=undef
			unsigned aux_mode				: 2;	//	0=Disabled, 1=RV, 2=Utility, 3=Wired Control
			
			unsigned pushbutton_enabled		: 1;
			unsigned batt_temp_sense_present: 1;   // 	0=disabled, 1=enabled
			unsigned unused2				: 6;
        };
    };
    int16_t	tmr_shutdown_delay;		//  Delay before Timer Shutdown [.5-sec resolution]
	int16_t	ac_line_qual_delay;	//  Delay before AC-Line Valid [mSec resolution]
} DEVICE_CONFIG_t;	//	(4-bytes total)
#pragma pack()  // restore packing

// helper macros
#define BITF1(name, bitno)  (( (((uint16_t)name)&0x1)<<(bitno) )) // one bit
#define BITF2(name, bitno)  (( (((uint16_t)name)&0x3)<<(bitno) )) // two bits

// macro used to generate device.config.all_flags for initialization since compiler has problems
#define DEVICE_CFG_ALL_FLAGS(inv_enable, chg_enable, pass_enable, tmr_enable, rmt_mode, aux_mode, push_btn, temp_sns)  \
    BITF1(inv_enable, 0) | \
    BITF1(chg_enable, 1) | \
    BITF1(pass_enable,2) | \
    BITF1(tmr_enable, 3) | \
    BITF2(rmt_mode,   4) | \
    BITF2(aux_mode,   6) | \
    BITF1(push_btn,   8) | \
    BITF1(temp_sns,   9)

//-----------------------------------------------------------------------------
//                     D E V I C E   E R R O R S
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef union
{
    struct
    {
		// --------------------------------------------------
        // WARNING! Do not change the order of these bits.
        //          They are an external interface used by
        //          CanBusDiag and Automated Tester and will
        //          break functionality if you change them.
        //          You can use the unused bits.
        //
		// --------------------------------------------------
		unsigned xfmr_ot			  : 1; // 0x0001  1 = transformer over-temp (drives disabled)
		unsigned hs_temp			  : 1; // 0x0002  1 = heatsink over-temperature
		unsigned nvm_is_bad           : 1; // 0x0004  1 = non-volatile-memory chip is bad
        unsigned batt_ot              : 1; // 0x0008  1 = battery over temperature

        unsigned batt_temp_snsr_open  : 1; // 0x0010  1 = battery temperature sensor open
        unsigned batt_temp_snsr_short : 1; // 0x0020  1 = battery temperature sensor shorted
		unsigned vreg15_invalid		  : 1; // 0x0040	1 = Housekeeping supply is low
        unsigned unused2              : 9;
    };
    struct
    {
        unsigned all_flags			: 7;
        unsigned unused             : 9;
    };
} DEVICE_ERRORS_t;
#pragma pack()  // restore packing


//-----------------------------------------------------------------------------
//                     D E V I C E   I N F O
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct {
	DEVICE_STATUS_t	status;
	DEVICE_CONFIG_t	config;
	DEVICE_ERRORS_t	error;
} DEVICE_STRUCT_t;
#pragma pack()  // restore packing



// -------------------
// access global data
// -------------------
extern DEVICE_STRUCT_t Device;


//-----------------------------------------------------------------------------
//                    A L L O C A T E    R O M    D A T A
//-----------------------------------------------------------------------------

#if !defined(ALLOCATE_SPACE_ROM_DEFAULTS)
	extern const DEVICE_CONFIG_t DeviceConfigDefaults;
#else
	
	//	batt_temp_sense_present
    #ifdef OPTION_HAS_CHGR_EN_SWITCH
      #define DEVICE_DFLT_TEMP_SENSOR_PRESENT  0   // charger disable switch hijacks temp sensor
    #else
      #define DEVICE_DFLT_TEMP_SENSOR_PRESENT  1   // standard temp sensor plugs into RJ-45 jack
    #endif

	const DEVICE_CONFIG_t DeviceConfigDefaults = 
	{
		//	two sets of braces are needed to initialize a structure inside of a union. 
        {{ DEVICE_CFG_ALL_FLAGS(
			DEVICE_DFLT_INVERTER_ENABLE,
			DEVICE_DFLT_CHARGER_ENABLE,
			DEVICE_DFLT_PASS_THRU_ENABLE, 
			DEVICE_DFLT_TMR_SHUTDOWN_ENABLE,
			DEVICE_DFLT_REMOTE_MODE,
			DEVICE_DFLT_AUX_MODE,
			DEVICE_DFLT_PUSHBUTTON_ENABLE,
			DEVICE_DFLT_TEMP_SENSOR_PRESENT
            )
		}},
		DEVICE_DFLT_TMR_SHUTDOWN_DELAY,
		DEVICE_DFLT_AC_LINE_QUAL_DELAY
	};
#endif	//	ALLOCATE_SPACE_ROM_DEFAULTS
	
#endif  //  __DEVICE_H

// <><><><><><><><><><><><><> device.h <><><><><><><><><><><><><><><><><><><><><><>
    