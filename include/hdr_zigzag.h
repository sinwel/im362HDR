/***************************************************************************
**  
**  hdr_zigzag.h
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**
**  2016/09/30 10:12:44 version 1.0
**  
**	version 1.0 	 
**  Copyright 2016, rockchip.
**
***************************************************************************/

#pragma once
#ifndef _HDR_ZIGZAG_H_
#define _HDR_ZIGZAG_H_

#include "rk_bayerhdr.h"


class ZigZagHDRInterface 
{

public:

	uint16_t		    pWdrTable[16*962];	                //<<! 961 ?	
	//uint16_t		    pFushionTable[16*962];	 
	uint8_t		        pFushionTable[16*962];	            //<<! fushion by bilinear for long and short
    
	//<<!  RawSrc
	uint32_t 		    mRawWid;							//<<! RawSrcs data width
	uint32_t			mRawHgt;							//<<! RawSrcs data height
	uint32_t			mRawStride;						 	//<<! RawSrcs data Stride (Bytes, 10bit 4ByteAlign)
	uint16_t*		    pRawSrc; 		                    //<<! RawSrcs data pointers

	//<<!  ThumbSrcs
	uint32_t			mThumbWid; 						 	//<<! ThumbSrcs data width
	uint32_t			mThumbHgt; 						 	//<<! ThumbSrcs data height
	uint32_t			mThumbStride;						//<<! ThumbSrcs data Stride (Bytes, 16bit 4ByteAlign)
	uint16_t*		    pThumbSrcs;	                        //<<! ThumbSrcs data pointers

  	//<<!  RawDst
  	uint16_t*	        pRawDst;                            //<<! RawDst data pointer


	//<<!  RawInfo
    float32_t           mRedGain;           				//<<! AWB-RedGain
	float32_t           mBlueGain;          				//<<! AWB-BlueGain
	float32_t  	        mWdrGain;          			        //<<!  
	
	uint16_t  	        mBits;          			        //<<! defalut 10bit
	uint16_t  	        mNoiseIntensity;          			//<<!
	uint16_t  	        mExpTimes;          				//<<!
	uint16_t  	        mBlackLevel;     				    //<<! Black Level

    //<<!
    void hdrprocess_sony_raw( uint16_t *src, 
    								uint16_t *dst, 
    								uint16_t *simage, 
    								uint32_t W, 
    								uint32_t H, 
    								uint32_t expTimes, 
    								uint32_t noise_thred);

};


//////////////////////////////////////////////////////////////////////////

#endif // _HDR_ZIGZAG_H_



