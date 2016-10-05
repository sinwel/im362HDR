/***************************************************************************
**  
**  hdr_process_block.cpp
**  
** 	Do hdr for block, once store 4x32 block short data out.
** 	need to set the width and height with corrsponding Stride.
**
**  NOTE: NULL
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/30 16:12:44 version 1.0
**  
**	version 1.0 	init. slide window do not suit for HDR interpolation.
**	version 2.0 	do residual scale and clip ,then store to DTCM align
**					for next stage operation[max(3,3) filter and LUT].
**
**   	------------
**		--	G R
**			B G ---
**		------------
** Copyright 2016, rockchip.
**
***************************************************************************/
#include "rk_bayerhdr.h"
#include "hdr_process.h"
#include "hdr_zigzag.h"

PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_2") uint16_t 		g_HdrBlkBuf[2][(HDR_BLOCK_H+2*HDR_PADDING)*(HDR_BLOCK_W+2*HDR_PADDING)] = {0};// 36x68x2x2 = 10K
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_2") uint16_t 		g_HdrOutBuf[2][HDR_BLOCK_H*HDR_BLOCK_W]									= {0};// 32x64x2x2 = 8K
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	  uint16_t		g_HdrRowBuf[2*HDR_PADDING*4096] 										= {0};		// 2* Line
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	  uint16_t		g_HdrColBuf[HDR_PADDING*HDR_BLOCK_H]								 	= {0};	  	// 2  col

/*
HDRInterface::HDRInterface()
: m_hdrIf(NULL)
{
}


HDRInterface::~HDRInterface()
{
}
*/

PRAGMA_CSECT("zzhdr_sect")
void HDRInterface::hdrprocess_sony_raw()
{
	uint16_t *src	= m_hdrIf->pRawSrc; 
	uint16_t *dst	= m_hdrIf->pRawDst;  

	uint32_t W		= m_hdrIf->mRawWid;  
	uint32_t H		= m_hdrIf->mRawHgt;
	
	
	int rows	   	= ALIGN_CLIP(H,HDR_BLOCK_H);
	int cols	   	= ALIGN_CLIP(W,HDR_BLOCK_W);   
	
	int thumbStride = m_hdrIf->mThumbStride;// thumb = (w/64,h/32)
	int frameNum   	= 0;
	
	// process video frames.
	
	do
	{
		int buffIdx	   	= 0;
		int blkOutCnt  	= 0;
		int x_prev	   	= 0;
		int y_valid	   	= 0;
		int x		   	= 0;
		bool bFristCTUline = 0;
		memset(g_HdrBlkBuf,0,sizeof(g_HdrBlkBuf));
		memset(g_HdrRowBuf,0,sizeof(g_HdrRowBuf));
		memset(g_HdrColBuf,0,sizeof(g_HdrColBuf));
	
		for (int y = 0; y < rows; y += HDR_BLOCK_H)
		{
		#if HDR_DEBUG_ENABLE
			if  ( y_pos == 33  )
				y_pos = y_pos;
		#endif
			for ( x = 0; x < cols; x += HDR_BLOCK_W)
			{
			#if HDR_DEBUG_ENABLE
				x_pos = x_prev/HDR_BLOCK_W;
			#endif
				// Fill Blockdata

				dma_2Dtransf(g_HdrBlkBuf[buffIdx]+2,
							 src+x+y*W	,
							 4,
							 min_(HDR_BLOCK_H,H-y),
							 min_(HDR_BLOCK_W,W-x),
							 HDR_SRC_STRIDE,
							 W);
							 

			    // Fill 4-TopExternalRows from RowBuf

				dma_2Dtransf(	g_HdrBlkBuf[buffIdx],
								g_HdrRowBuf+x,
								0,
								4,
								HDR_SRC_STRIDE,
								HDR_SRC_STRIDE,
								4096);
				

			    // Fill 2-LeftCol from ColBuf

				dma_2Dtransf(	g_HdrBlkBuf[buffIdx],
								g_HdrColBuf	,
								4,							
								HDR_BLOCK_H,		// line
								2,					// width
								HDR_SRC_STRIDE,
								2);
				

			    // Update 2-RightCol back.

				dma_2Dtransf(	g_HdrColBuf,
								g_HdrBlkBuf[buffIdx]+5*HDR_SRC_STRIDE-4	,
								0,
								HDR_BLOCK_H,	// line
								2,				// width
								2,
								HDR_SRC_STRIDE);

				
				buffIdx = (buffIdx + 1) & 0x1; // odd-even for LoadData

			    if (x==0 && y==0)
			    {
			        // First Block of Image
			        continue;
			    }
			    else
			    {
			        // Fill 2-RightCol from AnotherBuf
					dma_2Dtransf(	g_HdrBlkBuf[buffIdx] - 2, 
									g_HdrBlkBuf[(buffIdx+1)&1] + 4*HDR_SRC_STRIDE + 2, 
									5, 
									HDR_BLOCK_H, 	// line
									2, 				// width
									HDR_SRC_STRIDE, 
									HDR_SRC_STRIDE);
					

					// Update 4-BottomRows to RowBuf, waiting bottom-right corner data.
					dma_2Dtransf(	g_HdrRowBuf + x_prev, 
									g_HdrBlkBuf[buffIdx] + HDR_BLOCK_H*HDR_SRC_STRIDE, 
									0, 
									4, 				// line
									HDR_SRC_STRIDE, // width
									4096, 
									HDR_SRC_STRIDE);
					

					
					y_valid = (x == 0)? y - HDR_BLOCK_H : y;
				#if HDR_DEBUG_ENABLE
					y_pos   = y_valid/HDR_BLOCK_H;
				#endif
					bFristCTUline = y_valid < HDR_BLOCK_H;
					hdr_block_process(x_prev,y_valid,thumbStride,frameNum,g_HdrBlkBuf[buffIdx], g_HdrOutBuf[buffIdx], bFristCTUline ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));

					if (bFristCTUline) // skip the first "HDR_PADDING" line data.
				        dma_2Dtransf(	dst+x_prev, 
							       		g_HdrOutBuf[buffIdx] + HDR_PADDING*HDR_BLOCK_W, 
							        	y_valid , 
							        	HDR_BLOCK_H - HDR_PADDING, 
							        	min_(HDR_BLOCK_W,W-x_prev),  
							        	W , 
							        	HDR_BLOCK_W);
					else
				        dma_2Dtransf(	dst+x_prev, 
								        g_HdrOutBuf[buffIdx], 
								        y_valid - HDR_PADDING, 
								        HDR_BLOCK_H, 
								        min_(HDR_BLOCK_W,W-x_prev),  
								        W ,
								        HDR_BLOCK_W);

				}
				x_prev = x;	
			}

		}

		// do last block
		hdr_block_process(x_prev,y_valid,thumbStride,frameNum,g_HdrBlkBuf[(buffIdx+1)&1], g_HdrOutBuf[(buffIdx+1)&1], y_valid < HDR_BLOCK_H ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));
	    dma_2Dtransf(dst+x_prev, g_HdrOutBuf[(buffIdx+1)&1], y_valid - HDR_PADDING, min_(HDR_BLOCK_H,H-y_valid), min_(HDR_BLOCK_W,W-x_prev),  W , HDR_BLOCK_W);

		// 2 padding line data miss


		frameNum++;
		mFrameNum++;
	}while(frameNum < 2);
}



