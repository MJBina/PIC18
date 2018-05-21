// <><><><><><><><><><><><><> charger.h <><><><><><><><><><><><><><><><><><><><><><>
//-----------------------------------------------------------------------------
//  Copyright(C) 2016 - Sensata Technologies, Inc.  All rights reserved.
//-----------------------------------------------------------------------------
//
//  Charger Functionality
//  
//      The Charger code comprises 3-files which is intended to accomodate 
//      multiple battery types:
//
//      High-level functions provide a 'generic' interface to the transfer 
//      function(s). The High-level functions in the file charger.c are:
//          chgr_Init()
//          chgr_Driver()
//          chgr_Start()
//          chgr_Stop()
//          chgr_GetVoltageSetpoint()
//          chgr_GetCurrentSetpoint()
//          chgr_GetCurrentPercent()
//          
//      Mid-level functions implements the battery recipe and charge algorithm.
//      The Mid-level functions in the file charger_liion.c are:
//          chgr_ChargeCycleReset()
//          _chgr_Driver()
//  
//      Low-level functions implement the interrupt service routine.
//          chgr_PwmIsr()
//          _chgr_DutyReset()
//          _chgr_DutyInc()
//          _chgr_DutyDec()
//          chgr_PwmIsr()
//          _chgr_Driver()
//  
//-----------------------------------------------------------------------------

#ifndef __CHARGER_H    // include only once
#define __CHARGER_H

// -------
// headers
// -------
#include "options.h"    // must be first include
#include "analog.h"
#include "device.h"
#include "battery_recipe.h"
#include "charger_dbg.h"

// ------
// enums
// ------
typedef enum 
{
    CS_INVALID = 0, 
    CS_INITIAL,         //  1
    CS_SOFT_START,      //  2
    CS_LOAD_MGMT,       //  3
    CS_CONST_CURR,      //  4  constant current
    CS_CONST_VOLT,      //  5  constant voltage - a.k.a. absorbtion 
    CS_FLOAT,           //  6  trickle charge 
    CS_WARM_BATT,       //  7
    CS_SOFT_STOP,       //  8
    CS_SHUTDOWN,        //  9
	CS_EQUALIZE,		//	10
	CS_MONITOR,			//	11
	NUM_CHARGER_STATE_t        //  this has to be last
} CHARGER_STATE_t;

typedef enum 
{
	CS_EQ_INACTIVE = 0,	   // 0 INVALID
	CS_EQ_PRECHARGE,       // 1
	CS_EQ_ACTIVE,          // 2
	CS_EQ_COMPLETE,        // 3
    NUM_CHARGER_EQ_STATE_t // 4
} CHARGER_EQ_STATE_t;	//	2-bits

// -----------------------
//  Charger Current Limit
//   Used by CAN
// -----------------------
#define MAX_BRANCH_CIRCUIT_AMPS       (30)	// AC amps
#define DEFAULT_BRANCH_CIRCUIT_AMPS   (15)  // AC amps
//	BCR_A2D_SETPOINT_ADJUST:  When in Load-Management, the BCR (A2D) setpoint 
//	is the current limit.  The control algorith applies a Dead-Band around the
//	setpoint.  We don't want the output to exceed the BCR so we decrease the 
//	setpoint slightly by multiplying the A2D value by BCR_A2D_SETPOINT_ADJUST.
#define BCR_A2D_SETPOINT_ADJUST		((float)(0.97))	//	97%


//-----------------------------------------------------------------------------
// 	These parameters are specific to a given MODEL. They are used to initialize
//	the charger configuration (see structure CHARGER_CONFIG_t). The charger 
//	configuration may be altered via CAN.

//	CHGR_ILIMIT_REF_A2D:  The reference value (in A2D counts) that is compared 
//	with the ILimit signal to set charger max AC amps.  
//	For NP, the ILimit signal circuitry accommodates adjustment of the signal 
//	level via a potentiometer.  This reference value is (normally) fixed.
//	For LPC, the ILimit signal is not adjustable, therefore this reference must 
//	be adjusted to limit current.
//-----------------------------------------------------------------------------

// charger ILimit adjustment
	#define CHGR_ILIMIT_REF_A2D (ILINE_AMPS_ADC(12)*ILIMIT_POT_ADJUST)	// A2D counts

//  Load Management Values in ADC counts
    #define LOAD_MGMT_DEADBAND  (5)		//	A2D counts
    #define LOAD_MGMT_RESET     (30)	//	A2D counts
	
#if defined(MODEL_12LPC15) || defined(MODEL_51LPC15)
    #undef  LOAD_MGMT_RESET    
    #define LOAD_MGMT_RESET     (20) // redefine difference
#endif


//-----------------------------------------------------------------------------
//                   C H A R G E R     S T A T U S
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    union
    {
        struct
        {
            unsigned all_flags          : 16;
        };
        struct
        {
			// ----------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
			// ----------------------------------------------------
            unsigned charger_active     : 1;    //  1 = charge-mode is active
            unsigned output_active      : 1;    //  1 = charger output is active
            unsigned charge_state       : 4;    //  see 'CHARGER_STATE_t'
            unsigned batt_warm  		: 1;	//  1 = warm-batt mode
            unsigned unused             : 1;	//	was 'cc_timeout'
			
            unsigned cv_timeout         : 1;
            unsigned cv_roc_timeout     : 1;
			unsigned float_timeout		: 1; 
			unsigned ss_cc_lm_timeout 	: 1; 
			
			unsigned cc_cv_timeout 		: 1; 	
            unsigned eq_status			: 2;
			unsigned batt_ot     		: 1;
        };
    };

    union
    {
        struct
        {
			unsigned all_flags2			: 3;
			unsigned 		 			: 13;
        };
        struct
        {
			// ----------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
			// ----------------------------------------------------
			unsigned charge_complete	: 1;
			unsigned eq_timeout			: 1;
			unsigned oc_detected		: 1;
			unsigned 					: 13;
        };
    };
	
    // charger status info
    ADC10_RDG_t volt_setpoint;
    ADC10_RDG_t amps_setpoint;		//	I-Limit applied (lesser of BCR or Device limit)
    int32_t 	cc_timer_amp_minutes;
    int16_t 	cc_elapsed_minutes;
    int16_t 	cv_timer_minutes;
    int16_t 	cv_roc_timer_minutes;
    int16_t 	float_timer_minutes;
	int16_t		ss_cc_lm_timer_minutes;
	int16_t		cc_cv_timer_minutes;
    int16_t 	eq_timer_minutes;
} CHARGER_STATUS_t;
#pragma pack()  // restore packing setting


//-----------------------------------------------------------------------------
//                      C H A R G E R     E R R O R S
//-----------------------------------------------------------------------------
//	Errors prevent the charger from running.  The prerequisite for charger 
//	operation is that there are no errors.  Battery temperature is a gray area
//	in this regard.  The charge should stop when the battery is over-temp, 
//	HOWEVER, the charger is likely the cause of this condition.  Therefore, we
//	won't consider battery OT an error.
//-----------------------------------------------------------------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    union
    {
        struct
        {
            unsigned all_flags          	:  4;
            unsigned unused         		: 12;
        };
        struct
        {
			// ----------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
            //
            //  TODO: Charger low-batt should be a system error
			//        if batt volt is too low, FET body diodes conduct
			// ----------------------------------------------------
            unsigned batt_lo_volt_shutdown 	: 1;
            unsigned batt_hi_volt_shutdown 	: 1;
			unsigned oc_shutdown		   	: 1;
            unsigned cc_timeout          	: 1;
            
            unsigned unused2            	: 12;
        };
    };
} CHARGER_ERROR_t;
#pragma pack()  // restore packing setting


//-----------------------------------------------------------------------------
//  Battery Recipe Temperature Trip Points (in Celcius)
//       > 60°C : Warm Batt to Shutdown
//      <= 55°C : From Shutdown to Warm Batt
//      >= 50°C : From Normal to Warm Batt (bulk and acceptance timers on hold)
//      <= 45°C : From Warm Batt to Normal (bulk and acceptance timers resume)
//-----------------------------------------------------------------------------

#define CHGR_BATT_TEMP_SHUTDOWN_THRES    ((int16_t)(60))  // Celsius
#define CHGR_BATT_TEMP_SHUTDOWN_RECOV    ((int16_t)(55))  // Celsius
#define CHGR_BATT_TEMP_WARM_THRES        ((int16_t)(50))  // Celsius
#define CHGR_BATT_TEMP_WARM_RECOV        ((int16_t)(45))  // Celsius

//-----------------------------------------------------------------------------
//  CV to Float Rate-Of-Change (ROC): 
//	The goal of using Rate-of-Change is to transition from CV to Float state 
//	when the battery is near full charge.
//
//	As the battery becomes charged, the current decreases. Normally, charge 
//	current decreases exponentially with charge. When fully charged the current 
//	is essentially constant, but may vary in magnitude due to parasitic loads,
//	and battery condition.
//	
//	In Charger Mode, the transformer primary current (IMeas) is proportional to
//	the charge current. The "Standard Deviation" (a.k.a. SD) of the charge 
//	current is calculated and used to determine the rate of change.
//
//	There are 2 parameters that affect the transition from CV to Float:
//		cv_roc_sdev_threshold - Float Rate-of-Change Std Dev Threshold (adc)
//		cv_roc_timeout_minutes - Float Rate-of-Change Threshold  (minutes)
//	The basic function is that when the SD is less-than the threshold, then the 
//	timer {counter) will decrement. When the timer decrements to zero, then
//	we transition from CV to Float occurs.
//
//-----------------------------------------------------------------------------

//	NOTE: WACR is updated every 68.27 secs ((64*64)/60)
#define CHGR_BATT_ROC_TIMEOUT		39  //39x68.27/60=44.4min
#define CHGR_BATT_ROC_SDEV_THRES	0   //3


// ---------------------
// Charger Configuration
// ---------------------

#pragma pack(1)  // structure packing on byte alignment
typedef struct
{
	union
	{
        struct
        {
            unsigned    all_flags   : 16;     //	(2-bytes)
        };
		struct
		{
			// ----------------------------------------------------
            // WARNING! Do not change the order of these bits.
            //          They are an external interface used by
            //          CanBusDiag and Automated Tester and will
            //          break functionality if you change them.
            //          You can use the unused bits.
			// ----------------------------------------------------
			unsigned 	eq_request  : 1;		//	Equalization Requested
			unsigned	unused      : 15;
		};
	};
    ADC10_RDG_t amps_limit;    	// charger max AC amps 
    int16_t     bcr_int8;      	// branch circuit max AC amps (RV-C compatible)
    ADC10_RDG_t bcr_adc;     	// branch circuit max AC amps in ADC counts
    BATTERY_RECIPE_t	battery_recipe;
} CHARGER_CONFIG_t;
#pragma pack()  // restore packing setting


// --------
// Charger
// --------
#pragma pack(1)  // structure packing on byte alignment
typedef struct 
{
    CHARGER_CONFIG_t   config;     // configuration
    CHARGER_STATUS_t   status;     // status
    CHARGER_ERROR_t    error;      // state
} CHARGER_t;
#pragma pack()  // restore packing setting


//-----------------------------------------------------------------------------
//        A L L O C A T E      R O M      R E C I P E S
//-----------------------------------------------------------------------------

#if defined(ALLOCATE_SPACE_ROM_DEFAULTS)

  const BATTERY_RECIPE_t ChgrConfigBattRecipe_Agm = CHARGER_AGM_BATTERY_RECIPE;
  const BATTERY_RECIPE_t ChgrConfigBattRecipe_Gel = CHARGER_GEL_BATTERY_RECIPE;
  const BATTERY_RECIPE_t ChgrConfigBattRecipe_Wet = CHARGER_WET_BATTERY_RECIPE;
  const BATTERY_RECIPE_t ChgrConfigBattRecipe_LI  = CHARGER_LI_BATTERY_RECIPE;
  
  const CHARGER_CONFIG_t ChgrConfigDefaults = 
  {
      {{0}},							//	unused bit-flags
  	CHGR_ILIMIT_REF_A2D,				//	amps_limit (A2D counts)
  	DEFAULT_BRANCH_CIRCUIT_AMPS,		//	bcr_int8
    (ADC10_RDG_t)(ILINE_AMPS_ADC(DEFAULT_BRANCH_CIRCUIT_AMPS) * BCR_A2D_SETPOINT_ADJUST),	//	bcr_adc
    CHARGER_DEFAULT_BATTERY_RECIPE,		//	battery_recipe;
  };

#else
	
  extern const CHARGER_CONFIG_t ChgrConfigDefaults;
  extern const BATTERY_RECIPE_t ChgrConfigBattRecipe_Agm;
  extern const BATTERY_RECIPE_t ChgrConfigBattRecipe_Gel;
  extern const BATTERY_RECIPE_t ChgrConfigBattRecipe_Wet;
  extern const BATTERY_RECIPE_t ChgrConfigBattRecipe_LI;
  
#endif	


// -----------------
// access global data
// -----------------
extern CHARGER_t  Chgr;
extern int8_t   chgr_StopRequest;
extern int8_t   _chgr_pos_neg_zc;
extern uint16_t _comp_q16;
extern uint16_t _duty_cycle;
extern int16_t  chgr_isr_hist[];
extern int16_t  chgr_isr_hist_index;
extern int8_t   chgr_resynch;
extern int8_t   chgr_over_volt;
extern int16_t  debug_IMeas[];
extern int16_t  debug_IMeasAvg[];
extern int16_t  debug_array_ix;
extern int16_t  debug_trigger;
extern int8_t   debug_arm;

//------------------------------
// [public] Function Prototypes
//------------------------------

extern void  	chgr_ChargeCycleReset(void);
extern void  	chgr_Driver(int8_t reset);
extern int8_t  	chgr_GetCurrentPercent(void);
extern int16_t 	chgr_GetCurrentSetpoint(void);
extern int16_t 	chgr_GetVoltageSetpoint(void);
extern int8_t 	chgr_IsPwmDutyMinimum(void);
extern void  	chgr_PwmIsr(void);
extern void  	chgr_Start(void);
extern void  	chgr_Stop(void);
extern void  	chgr_StopNow(void);
extern void  	chgr_TimerOneMinuteUpdate(void);
extern void  	TASK_chgr_Driver(void);

//	extern const char* chgr_StateToStr(uint16_t chargeState);
//	extern const char* ChgrEqualStateToStr(uint16_t equalState);
//	extern void  chgr_LogState(void);
//	extern void  chgr_LogErrFlags(void);


//-------------------------------
// [private] Function Prototypes
//-------------------------------
extern void   _chgr_pwm_isr(int8_t reset);
extern void   _chgr_DutyReset(void);
extern int8_t _chgr_DutyInc(void);
extern int8_t _chgr_DutyDec(void);
extern int8_t _chgr_DutyDecStop(void);

#endif  //  __CHARGER_H

// <><><><><><><><><><><><><> charger.h <><><><><><><><><><><><><><><><><><><><><><>
