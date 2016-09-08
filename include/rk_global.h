//
//////////////////////////////////////////////////////////////////////////
// File: rk_global.h
// Desc: Global definition
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
#pragma once
#ifndef _RK_UTILS_H
#define _RK_UTILS_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rk_typedef.h"                 // Type definition


//////////////////////////////////////////////////////////////////////////
////-------- MFNR Module Debug Switch Setting
//enum BypassSwitch{ DISABLE_BYPASS = 0, ENABLE_BYPASS = 1 };
#define     DISABLE_BYPASS              0x0
#define     ENABLE_BYPASS               0x1
#define     BYPASS_Register             0   // 0-DisableBypass, 1-EnableBypass,  FeatureDetect,CoarseMatching,FineMatching
#define     BYPASS_Enhancer             0   // 0-DisableBypass, 1-EnableBypass,  TemporalDenoise,BayerWDR,SpatialDenoise
// #define     BYPASS_TemporalDenoise      0   // 0-DisableBypass, 1-EnableBypass,  TemporalDenoise
// #define     BYPASS_BayerWDR             0   // 0-DisableBypass, 1-EnableBypass,  BayerWDR
// #define     BYPASS_SpatialDenoise       0   // 0-DisableBypass, 1-EnableBypass,  SpatialDenoise

////-------- CEVA XM4 Debug Switch Setting
#define     CEVA_XM4_DEBUG              0   // 1/0 for CEVA_XM4 IDE Debug-View
////-------- Debug Params Setting
#define     MY_DEBUG_PRINTF             0   // for printf()  1 or 0

//////////////////////////////////////////////////////////////////////////
////-------- Input Params Setting
// Raw Info
#define     RK_MAX_FILE_NUM	        10              // max input images
#define     RAW_BIT_COUNT           10              // Raw Bit Count (10forDDR or 16forPC)
#define     THUMB_BIT_COUNT         16              // Thumb Bit Count (default=16)
// Align
#define     ALIGN_4BYTE_WIDTH(Wid, BitCt)	(((int)(Wid) * (BitCt) + 31) / 32 * 4) // 4ByteAlign
//#define     ALIGN_4PIXEL_START(X)           ((long)(X) / 4 * 4)                     // 4PixelAlign
#define     ALIGN_4PIXEL_START(X)           ((int32_t)(X) - ((int32_t)(X)%4!=0)*2)  //((X)%4) ? (((X)/4)-4) : (X)
#define     ALIGN_4PIXEL_WIDTH(W)           ((uint16_t)((double)(W) / 4 + 0.5)  * 4)    // 4PixelAlign



//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//---- MAX() & MIN()
//#define		MAX(a, b)			    ( (a) > (b) ? (a) : (b) )
//#define		MIN(a, b)			    ( (a) < (b) ? (a) : (b) )

//---- ABS()
#define	    ABS_U16(a)			    (uint16_t)( (a) > 0 ? (a) : (-(a)) )

//---- ROUND()
#define	    ROUND_U16(a)		    (uint16_t)( (double) (a) + 0.5 )
#define	    ROUND_U32(a)		    (uint32_t)( (double) (a) + 0.5 )
#define	    ROUND_I32(a)		    (int32_t)( (a) > 0 ? ((double) (a) + 0.5) : ((double) (a) - 0.5) )

//---- CEIL() & FLOOR()
#define	    CEIL(a)                 (int)( (double)(a) > (int)(a) ? (int)((a)+1) : (int)(a) )  
#define	    FLOOR(a)                (int)( (a) ) 

//---- FABS()
#define	    FABS(a)                 ( (a) >= 0 ? (a) : (-(a)) )


//////////////////////////////////////////////////////////////////////////

#endif // _RK_UTILS_H



