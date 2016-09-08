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

//#include "ceva_types.h"

typedef struct
{
	struct
	{
		unsigned char				ver_major_u8;
		unsigned char				ver_minor_u8;
		unsigned short				ver_build_u16;
} version_t;
	struct
	{
		unsigned char				day_u8;
		unsigned char				month_u8;
		unsigned short				year_u16;
	} date_t;
} ceva_version_t;


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
#elif defined(_WIN32)
    #define PRAGMA_DSECT_NO_LOAD(name)
	#define PRAGMA_DSECT_LOAD(name)
	#define PRAGMA_CSECT(name)
	#define ALIGN(var,imm)					__declspec(align(imm)) var
	#define VECC_INIT()						do {} while(0)
#endif 
#endif 

#ifdef _MSC_VER 
#define __XM4_FUNC_SIGNATURE__			__FUNCSIG__
#else
#define __XM4_FUNC_SIGNATURE__			__PRETTY_FUNCTION__
#endif

//#define _ENABLE_PROFILING_
#if defined (_ENABLE_PROFILING_) 
#define GET_FUNCTION_NAME(name)				name = __XM4_FUNC_SIGNATURE__
namespace ceva {
	extern volatile const char*	g_p_function_name;
}
#else 
#define GET_FUNCTION_NAME(name) 
#endif


#ifdef XM4 
	#define MEM_BLOCK(num)					__attribute__ ((mem_block (num)))
	#define ALWAYS_INLINE					__attribute__((always_inline))
	#define RESTRICT						restrict
	#define DSP_CEVA_UNROLL(x)  DO_PRAGMA(dsp_ceva_unroll = x)
	#define DSP_CEVA_TRIP_COUNT(x)  DO_PRAGMA(dsp_ceva_trip_count = x)
	#define DSP_CEVA_TRIP_COUNT_FACTOR(x)  DO_PRAGMA(dsp_ceva_trip_count_factor = x)
	#define DSP_CEVA_TRIP_COUNT_MIN(x)  DO_PRAGMA(dsp_ceva_trip_count_min = x)
	#define DO_PRAGMA(x)		_Pragma ( #x )
#else 
	#define MEM_BLOCK(num)					
	#define ALWAYS_INLINE					
	#define RESTRICT	
	#define DSP_CEVA_TRIP_COUNT(x)
	#define DSP_CEVA_TRIP_COUNT_FACTOR(x)
	#define DSP_CEVA_TRIP_COUNT_MIN(x)
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
#define SWSUBCMP_CONFIG(psh_val,num_absolute_diff,src_offset,sat_val)	(((psh_val) & 0x3f) << INIT_PSH_VAL | ((num_absolute_diff) & 0x3) << NUM_ABSOLUTE_DIFF  | ((src_offset) & 0x3f) << SRC_OFFSET | ((sat_val) & 0xff) << SATURATION_VAL )				
#define SWSUB_CONFIG(c,e)			(((c) & 0x3f) << SRC_OFFSET   | ((e) & 0xff) << SATURATION_VAL )

#define STRIDE_IN_BYTES(a)		     MAX(68,((((a) + 59) >> 6) << 6) + 4)
#define STRIDE_IN_WORDS(a)		    (MAX(68,(((((a) << 1) + 59) >> 6) << 6) + 4) >> 1)
#define STRIDE_IN_DWORDS(a)		    (MAX(68,(((((a) << 2) + 59) >> 6) << 6) + 4) >> 2)

#ifdef TRUE
#undef	TRUE
#endif 

#ifdef FALSE
#undef	FALSE
#endif
#define TRUE	1
#define FALSE	0


#undef MAX
#define MAX(a,b) ((a)>(b) ? (a) : (b))

#undef MIN
#define MIN(a,b) ((a)<(b) ? (a) : (b))

#undef ABS
#define ABS(a)	 ((a)>(0) ? (a) : (-(a)))

#undef MAX_S16
#define MAX_S16(a,b)  (((int16_t)(a))>((int16_t)(b)) ? (int32_t)(a) : (int32_t)(b))

#undef MIN_S16
#define MIN_S16(a,b)  (((int16_t)(a))<((int16_t)(b)) ? (int32_t)(a) : (int32_t)(b))

#undef CLIPU8
#define CLIPU8(a) (uint8_t)MIN(MAX(0,a),255)

#undef CLIPS8
#define CLIPS8(a) (int8_t)MIN(MAX(-128,a),127)

#undef CLIPU16
#define CLIPU16(a) (uint16_t)MIN(MAX(0,a),65535)

#undef CLIPS16
#define CLIPS16(a) (int16_t)MIN(MAX(-32768,a),32767)

#define STRIDE_BYTES_FROM_WIDTH(myWidth)  ((((myWidth)+3)>>2)<<2)
#define STRIDE_WORDS_FROM_WIDTH(myWidth)  ((((myWidth)+1)>>1)<<1)

#define S8_MIN_VALUE	(-0x7f - 1)
#define S16_MIN_VALUE	(-0x7fff - 1)
#define S32_MIN_VALUE	(-0x7fffffff - 1)

#define S8_MAX_VALUE	(0x7f)
#define S16_MAX_VALUE	(0x7fff)
#define S32_MAX_VALUE	(0x7fffffff)
#define U8_MAX_VALUE	(0xff)
#define U16_MAX_VALUE	(0xffff)
#define U32_MAX_VALUE	(0xffffffff)

#define MAX3(a, b, c) ((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c))
#define MIN3(a, b, c) ((a) < (b)) ? (((a) < (c)) ? (a) : (c)) : (((b) < (c)) ? (b) : (c))
#define ABSDIFF(a,b) ((a)>(b) ? ((a)-(b)) : ((b)-(a)))
#define SATURATE_WORD_TO_BYTE_HIGH(a) ((a) > 255 ? 255 : (a))
#define SATURATE_WORD_TO_BYTE_LOW(a) ((a) < 0 ? 0 : (a))
#define SATURATE_WORD_TO_BYTE(a) SATURATE_WORD_TO_BYTE_HIGH(SATURATE_WORD_TO_BYTE_LOW(a))
#define ADDS16(a,b) ((a)+(b))
#define SUBTRACTS16(a,b) ((a)-(b))
#define ADDU8(a,b) SATURATE_WORD_TO_BYTE((a)+(b))
#define SUBTRACTU8(a,b) SATURATE_WORD_TO_BYTE((a)-(b))
#define SWAP(a, b)  do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while (0)
#define CAS(a,b) if((a)<(b)) SWAP(a,b) //Compare and Swap

#define ROUND_UP_2(a)   ((((a) + 1  )>>1)<<1)
#define ROUND_UP_4(a)   ((((a) + 3  )>>2)<<2)
#define ROUND_UP_8(a)   ((((a) + 7  )>>3)<<3)
#define ROUND_UP_16(a)  ((((a) + 15 )>>4)<<4)
#define ROUND_UP_32(a)  ((((a) + 31 )>>5)<<5)
#define ROUND_UP_64(a)  ((((a) + 63 )>>6)<<6)
#define ROUND_UP_128(a) ((((a) + 127)>>7)<<7)


#define XM4_NUM_OF_BANKS 16
#define XM4_BANK_WIDTH 4

#ifdef XM4
#define SET_REGION_CACHABLE(a, b) setRegionCachable(a, b) 
#else
#define SET_REGION_CACHABLE(a, b)
#endif

#endif //__XM4_DEFINES_H__
