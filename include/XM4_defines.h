/*************************************************************************************\
*																					 *
*Copyright (C) CEVA Inc. All rights reserved                                         *
*																					 *
*																					 *
*THIS PRODUCT OR SOFTWARE IS MADE AVAILABLE EXCLUSIVELY TO LICENSEES THAT HAVE       *
*RECEIVED EXPRESS WRITTEN AUTHORIZATION FROM CEVA TO DOWNLOAD OR RECEIVE THE         *
*PRODUCT OR SOFTWARE AND HAVE AGREED TO THE END USER LICENSE AGREEMENT (EULA).       *
*IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE               *
*CEVA EULA, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS PRODUCT OR SOFTWARE.           *
*																					 *
*The information contained in this document is subject to change without notice and  *
*does not represent a commitment on any part of CEVA? Inc. CEVA? Inc. and its      *
*subsidiaries make no warranty of any kind with regard to this material, including,  *
*but not limited to implied warranties of merchantability and fitness for a          *
*particular purpose whether arising out of law, custom, conduct or otherwise.        *
*																					 *
*While the information contained herein is assumed to be accurate, CEVA? Inc.       *
*assumes no responsibility for any errors or omissions contained herein, and         *
*assumes no liability for special, direct, indirect or consequential damage,         *
*losses, costs, charges, claims, demands, fees or expenses, of any nature or kind,   *
*which are incurred in connection with the furnishing, performance or use of this    *
*material.                                                                           *
*																				     *
*This document contains proprietary information, which is protected by U.S. and      *
*international copyright laws. All rights reserved. No part of this document may be  *
*reproduced, photocopied, or translated into another language without the prior      *
*written consent of CEVA? Inc.                                                      *
*																					 *
***************************************************************************************
*
* File Name :	XM4_defines.h 
*
* Description:
* This file contains definitions for CEVA XM4 platform
*
************************************************************************/

#ifndef __XM4_DEFINES_H__
#define __XM4_DEFINES_H__

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char		uchar;
typedef unsigned short		ushort;
typedef unsigned int        uint;
//typedef unsigned long long  ulong;

#ifndef PRAGMA_DSECT_NO_LOAD
#ifdef XM4 
	#define PRAGMA_DSECT_NO_LOAD(name)		__attribute__ ((section (".DSECT " name)))
	#define PRAGMA_DSECT_LOAD(name)			__attribute__ ((section (".DSECT " name)))
	#define PRAGMA_CSECT(name)				__attribute__ ((section (".CSECT " name)))
	#define ALIGN(var,imm)					var __attribute__ ((aligned (imm)))
#elif defined(__GNUC__) // gcc toolchain	
	#define PRAGMA_DSECT_NO_LOAD(name)
	#define PRAGMA_DSECT_LOAD(name)
	#define PRAGMA_CSECT(name)
	#define ALIGN(var,imm)					var __attribute__ ((aligned (imm)))
	#define VECC_INIT()						do {} while(0)
#elif defined(WIN32)
	#define PRAGMA_DSECT_NO_LOAD(name)
	#define PRAGMA_DSECT_LOAD(name)
	#define PRAGMA_CSECT(name)
	#define ALIGN(var,imm)					__declspec(align(imm)) var
	#define VECC_INIT()						do {} while(0)
#endif 
#endif 


#ifdef XM4 
	#define MEM_BLOCK(num)					__attribute__ ((mem_block (num)))
	#define ALWAYS_INLINE					__attribute__((always_inline))
	#define RESTRICT						restrict
	#define DSP_CEVA_UNROLL(x)  DO_PRAGMA(dsp_ceva_unroll = x)
	#define DO_PRAGMA(x)		_Pragma ( #x )
#else 
	#define MEM_BLOCK(num)					
	#define ALWAYS_INLINE					
	#define RESTRICT				
	#define DSP_CEVA_UNROLL(num)					
#endif 

#define INIT_PSH_VAL	0
#define NUM_FILTER		6
#define SRC_OFFSET		8
#define COEFF_OFFSET	16
#define STEP			21
#define PATTERN_OFFSET	24
#define SW_CONFIG(init_psh,num_filter,src_offset,coeff_offset,step,pattern)		(((init_psh) & 0x3f) << INIT_PSH_VAL | ((num_filter) & 0x7) << NUM_FILTER     | ((src_offset) & 0x3f) << SRC_OFFSET | ((coeff_offset) & 0x1f) << COEFF_OFFSET | ((step) & 0x7) << STEP | ((pattern) & 0xff) << PATTERN_OFFSET )				


#define NUM_ABSOLUTE_DIFF	6
#define SATURATION_VAL		24
#define SWSUBCMP_CONFIG(a,b,c,e)	(((a) & 0x3f) << INIT_PSH_VAL | ((b) & 0x3) << NUM_ABSOLUTE_DIFF  | ((c) & 0x3f) << SRC_OFFSET | ((e) & 0xff) << SATURATION_VAL )				
#define SWSUB_CONFIG(c,e)			(((c) & 0x3f) << SRC_OFFSET   | ((e) & 0xff) << SATURATION_VAL )

#define STRIDE_IN_BYTES(a)		        MAX(68,(((a + 59) >> 6) << 6) + 4)
#define STRIDE_IN_WORDS(a)		        (MAX(68,((((a<<1) + 59) >> 6) << 6) + 4)>>1)

#undef MAX_S16
#define MAX_S16(a,b)  (((short)(a))>((short)(b)) ? (int)(a) : (int)(b))

#undef MIN_S16
#define MIN_S16(a,b)  (((short)(a))<((short)(b)) ? (int)(a) : (int)(b))

#undef CLIPU8
#define CLIPU8(a) (uchar)MIN(MAX(0,a),255)

#undef CLIPS8
#define CLIPS8(a) (char)MIN(MAX(-128,a),127)

#undef CLIPU16
#define CLIPU16(a) (ushort)MIN(MAX(0,a),65535)

#undef CLIPS16
#define CLIPS16(a) (short)MIN(MAX(-32768,a),32767)

#define STRIDE_BYTES_FROM_WIDTH(myWidth)  ((((myWidth)+3)>>2)<<2)
#define STRIDE_WORDS_FROM_WIDTH(myWidth)  ((((myWidth)+1)>>1)<<1)

#endif //__XM4_DEFINES_H__
