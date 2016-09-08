/***************************************************************************
**  
**  DebugFiles.h
**  
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/09/08 16:12:44 version 1.0
**  
**	version 1.0 	
**   
**
** Copyright 2016, rockchip.
**
***************************************************************************/

//#pragma once
#ifndef _RK_DEBUG_H
#define _RK_DEBUG_H


//////////////////////////////////////////////////////////////////////////
////-------- Header files
// 
#include "rk_typedef.h"                 // Type definition

#include <assert.h>
#include <vec-c.h>
#include "XM4_defines.h" 
#ifdef __XM4__
#include "profiler.h"
#include <asm-dsp.h>
#endif

#define     HDR_DEBUG_ENABLE          1

#ifdef WIN32
#define PRINT_C_GROUP(namestr,var,start_pos,num,fp,...) \
    {\
        if (fp) \
        fprintf(fp,"[%8s]: ",namestr);\
        else \
        fprintf(stderr,"[%8s]: ",namestr); \
        for (int elem = start_pos; elem < (start_pos+num); elem++) \
        { \
            if (fp) \
                fprintf(fp,"0x%04x ",var[elem]);\
            else \
                fprintf(stderr,"0x%04x ",var[elem]); \
        } \
        if (fp) \
        fprintf(fp,"\n");\
        else \
        fprintf(stderr,"\n"); \
    }

#define PRINT_CEVA_VRF(namestr,vReg,fp,...) \
    {\
        if (fp) \
        fprintf(fp,"[%12s]: ",namestr);\
        else \
        fprintf(stderr,"[%12s]: ",namestr); \
        for (int elem = 0; elem < vReg.num_of_elements; elem++) \
        { \
            if (fp) \
                fprintf(fp,"%-4d ",vReg[elem]);\
            else \
                fprintf(stderr,"%-4d ",vReg[elem]); \
        } \
        if (fp) \
        fprintf(fp,"\n");\
        else \
        fprintf(stderr,"\n"); \
    }
#else
    #define PRINT_C_GROUP(namestr,var,start_pos,num,fp,...)
    #define PRINT_CEVA_VRF(namestr,vReg,fp,...)  

#endif

void writeFile(uint8_t *data, int cols, int rows, int stride, char* FileName);

void writeFile(uint16_t *data, int cols, int rows, int stride, char* FileName);
void writeBinFile(uint16_t *data, int Num, char* FileName);

// Copy Block Data
int CopyBlockData(uint16_t* pSrc, uint16_t* pDst, int nWid, int nHgt, int nSrcStride, int nDstStride);

#endif // _RK_DEBUG_H




