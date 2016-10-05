/***************************************************************************
**  
**  hdr_process.h
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**
**  2016/09/30 10:55:12 version 1.0
**  
**	version 1.0 	 
**  Copyright 2016, rockchip.
**
***************************************************************************/

#pragma once
#ifndef _HDR_PROCESS_H_
#define _HDR_PROCESS_H_

#include "rk_bayerhdr.h"
#include "Debugfiles.h"

class HDRprocess 
{
public:
    int mFrameNum;
#if HDR_DEBUG_ENABLE
    int x_pos ;
    int y_pos ;
#endif
public:
     HDRprocess();
    ~HDRprocess();

    //<<!
    void FilterdLUTBilinear ( uint16_t*	p_u16Weight, 		//<<! [in] 0-1024 scale tab.
							 uint16_t*	p_u16TabLS, 		//<<! [in] 0-1024 scale tab, may be can use char type for 0-255
							 uint16_t*	p_u16ImageL_S,	//<<! [in] long and short image[block32x64]
							 uint16_t*	p_u16PrevThumb,	//<<! [in] previous frame thumb image for WDR scale.
							 uint16_t*	p_u16CurrThumb,	//<<! [in] current frame thumb image for WDR scale.
							 uint16_t*  pScaleTab16banks,
							 uint16_t  	thumbStride,
							 int32_t 	weightStep, 		//<<! [in] weight stride which add padding.
							 int32_t 	imageStep, 		//<<! [in] 16 align
							 uint32_t 	u32Rows, 			//<<! [in] 
							 uint32_t 	u32Cols,			//<<! [in]  
	 						 uint16_t 	normValue,
	 						 uint16_t   frameNum,
							 uint16_t*	p_u16Dst);		//<<! [out] HDR out 16bit,have not do WDR.	//<<! [out] HDR out 16bit,have not do WDR.

    void zigzagDebayer(	uint16_t *p_u16Src, 
    						uint16_t *p_u16Tab, 
    						uint16_t blockW,
    						uint16_t blockH,
    						uint16_t stride,
    						uint16_t normValue,
    						uint16_t *buff,		//<<! [out] 32x64 short16  long and short
    						uint16_t *scale); 	//<<! [out] 32x64 char32  table



    void dma_2Dtransf(unsigned short *dst, 
    						unsigned short *src, 
    						int yoffset, 
    						int lines, 
    						int W, 
    						int stride_o, 
    						int stride_i);

    void hdr_block_process(int 		  x,
							int 	  y,
							int		  thumbStride,
							int 	  frameNum,
							uint16_t *pRawInBuff, 
							uint16_t *pHDRoutBuff, 
							bool 	  bFristCTUline,
							int 	  validW, 
							int 	  validH);
};


//////////////////////////////////////////////////////////////////////////

#endif // _HDR_PROCESS_H_



