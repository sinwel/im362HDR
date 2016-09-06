//
//////////////////////////////////////////////////////////////////////////
// File: rk_bayerwdr.h
// Desc: BayerWDR
// 
// Date: Revised by yousf 20160824
//
//////////////////////////////////////////////////////////////////////////
// 
/***************************************************************************
**  
**  rk_wdr.h
**  
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/12 9:12:44 version 1.0
**  
**	version 1.0 	have not MERGE to branch.
**   
**
** Copyright 2016, rockchip.
**
***************************************************************************/

//#pragma once
#ifndef _RK_BAYER_WDR_H
#define _RK_BAYER_WDR_H

//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition
#include "rk_global.h"                  // Global definition

#include <assert.h>
#include <vec-c.h>
#include "XM4_defines.h" 
#ifdef __XM4__
#include "profiler.h"
#include <asm-dsp.h>
#endif

#define     HDR_BLOCK_W	    64
#define     HDR_BLOCK_H	    32
#define     HDR_PADDING 	2
#define	    HDR_SRC_STRIDE (HDR_BLOCK_W+2*HDR_PADDING)

//#define     DEBUG_OUTPUT_FILES                  1

#define     CEVA_VECC                           1
#ifdef  WIN32
#define DEBUG_VECC		    1
#else
#define DEBUG_VECC		    0
#endif

 

#define max_(a,b) ((a) > (b) ? (a) : (b))
#define min_(a,b) ((a) < (b) ? (a) : (b))


#endif // _RK_BAYER_WDR_H



