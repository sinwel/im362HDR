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
#include <assert.h>
#include <vec-c.h>
#include "rk_typedef.h"
#include "rk_global.h" 
#include "profiler.h"
#include "XM4_defines.h"

#include "DebugFiles.h"

#ifdef __XM4__
#include "profiler.h"
#include <asm-dsp.h>
#endif

#define     HDR_BLOCK_W				64
#define     HDR_BLOCK_H				32
#define     HDR_PADDING 			2
#define	    HDR_SRC_STRIDE			(HDR_BLOCK_W+2*HDR_PADDING)
#define	    HDR_FILTER_W			(HDR_BLOCK_W+2) // for 3x3 filter.
#define     ALIGN_CLIP(w,wAlign)    (((w+wAlign-1)/wAlign)*wAlign);
#define     THUMB_SIZE_W			256              // default is 4K/16
#define     THUMB_SIZE_H			256



#define     R_B_LONG_PATTERN	0X5555
#define     R_B_SHORT_PATTERN	0XAAAA 
#define     G_LONG_PATTERN		0XFFFF
#define     G_SHORT_PATTERN		0X0
#define     CONNECT_LUT         0
#define     ENABLE_WDR          0

#define     HDR_VECC           1


#define max_(a,b) ((a) > (b) ? (a) : (b))
#define min_(a,b) ((a) < (b) ? (a) : (b))


#endif // _RK_BAYER_WDR_H



