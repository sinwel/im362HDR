


#include <string.h>
#include "rk_bayerhdr.h"

#if HDR_DEBUG_ENABLE
static int x_pos ;
static int y_pos ;
#endif
static int countFiles = 0;
// define the block parameters.


PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_2") uint16_t		p_u16TabLongShort[962*16]   =
{
	#include "../table/longshort_mapping_16banks.dat"
};

PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t		pL_S_ImageBuff[2][2*HDR_BLOCK_H*HDR_BLOCK_W] 	 = {0};// 4kx2 store long and short image.
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t 		pWeightBuff[(HDR_BLOCK_H+2)*(HDR_BLOCK_W+2)] 	 = {0};// 4k store weight  
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t		pWeightFilter[HDR_BLOCK_H*HDR_BLOCK_W] 			 = {0};// 4k store weight fileterd.

PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint8_t 		pWeightBuff_bak[(HDR_BLOCK_H+2)*(HDR_BLOCK_W+2)] 	 = {0};// 4k store weight  
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint8_t		pWeightFilter_bak[HDR_BLOCK_H*HDR_BLOCK_W] 			 = {0};// 4k store weight fileterd.



PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")   uint16_t		pPrevThumb[THUMB_SIZE_W*THUMB_SIZE_W] 			 = {0};// 32k store in DDR.

PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_3") uint16_t 		g_HdrBlkBuf[2][(HDR_BLOCK_H+2*HDR_PADDING)*(HDR_BLOCK_W+2*HDR_PADDING)] = {0};// 36x68x2x2 = 10K
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_3") uint16_t 		g_HdrOutBuf[2][HDR_BLOCK_H*HDR_BLOCK_W]									= {0};// 32x64x2x2 = 8K
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	  uint16_t		g_HdrRowBuf[2*HDR_PADDING*4096] 										= {0};		// 2* Line
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	  uint16_t		g_HdrColBuf[HDR_PADDING*HDR_BLOCK_H]								 	= {0};	  	// 2  col


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: CopyBlockData()
// Desc: Copy Block Data
//   In: pSrc               - [in] Src data pointer
//       nWid               - [in] Src data width
//       nHgt               - [in] Src data height
//       nSrcStride         - [in] Src data stride
//       nDstStride         - [in] Dst data stride
//  Out: pDst               - [in/out] Dst data pointer
// 
// Date: Revised by yousf 20160812
// 
/*************************************************************************/
int CopyBlockData(uint16_t* pSrc, uint16_t* pDst, int nWid, int nHgt, int nSrcStride, int nDstStride)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    uint8_t*     p_src = (uint8_t*)pSrc; // temp pointer
    uint8_t*     p_dst = (uint8_t*)pDst; // temp pointer
    for (int i=0; i < nHgt; i++)
    {
        memcpy(p_dst, p_src, nWid*sizeof(uint16_t));
        p_src += nSrcStride;
        p_dst += nDstStride;
    }

    //
    return ret;

#else
    //DMA
#endif
}

void FilterdLUTBilinear ( uint16_t *p_u16Weight, 		//<<! [in] 0-1024 scale tab.
							 uint16_t *p_u16TabLS, 		//<<! [in] 0-1024 scale tab, may be can use char type for 0-255
							 uint16_t *p_u16ImageL_S,	//<<! [in] long and short image[block32x64]
							 uint16_t *p_u16PrevThumb,	//<<! [in] previous frame thumb image for WDR scale.
							 int32_t weightStep, 		//<<! [in] weight stride which add padding.
							 int32_t imageStep, 		//<<! [in] 16 align
							 uint32_t u32Rows, 			//<<! [in] 
							 uint32_t u32Cols,			//<<! [in]  
	 						 uint16_t normValue,
							 uint16_t *p_u16Dst);		//<<! [out] HDR out 16bit,have not do WDR.

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
						int stride_i)
{
	dst = dst + stride_o*yoffset;
	for (int y = 0; y < lines; y++)
	{
		for (int x = 0; x < W; x++)
		{
			dst[x] = src[x];
		}
		src += stride_i;
		dst += (stride_o);
	}
}




void hdr_block_process(uint16_t *pRawInBuff, 
							uint16_t *pHDRoutBuff, 
							bool 	  bFristCTUline,
							int 	  validW, 
							int 	  validH)
{
	int 	buffIdx = 0; 
	int 	stride = 32;
	uint16_t  normValue = 1024 - 64;
	
	zigzagDebayer(	pRawInBuff,
					p_u16TabLongShort,
					validW,
					validH,
					HDR_SRC_STRIDE,
					1024-64,
					pL_S_ImageBuff[buffIdx],
					pWeightBuff+1+HDR_FILTER_W); // need be clear to zeros.	
#if 1
	FilterdLUTBilinear ( pWeightBuff				, 	//<<! [in] 0-1024 scale tab.
						 p_u16TabLongShort			,	//<<! [in] 0-1024 scale tab, may be can use char type for 0-255
						 pL_S_ImageBuff[buffIdx]	,	//<<! [in] long and short image[block32x64]
						 pPrevThumb					,	//<<! [in] previous frame thumb image for WDR scale.
						 HDR_FILTER_W				, 	//<<! [in] weight stride which add padding.
						 HDR_BLOCK_W				, 	//<<! [in] 16 align
						 validH						, 	//<<! [in] 
						 validW						,	//<<! [in]  
 						 normValue					,
						 pHDRoutBuff);					//<<! [out] HDR out 16bit,have not do WDR.


#else
 	Max3x3AndBilinear(	pWeightBuff_bak,
						pL_S_ImageBuff[buffIdx], 
						pPrevThumb,
						HDR_FILTER_W, 
						HDR_BLOCK_W, 
						validH, 
						validW,		
					#if HDR_DEBUG_ENABLE
						pHDRoutBuff,
						pWeightFilter_bak,
						x_pos,
						y_pos);
					#else
						pHDRoutBuff);
					#endif
#endif				

#if HDR_DEBUG_ENABLE
    char name_hdr[512],name_w[512];
    char name_image[512], name_weight[512];;

	if  ( y_pos == 1 && x_pos == 30  )
	{
#if __XM4__
		sprintf(name_image, "data/%s_%04d-%04d.dat", "LongShortImg_ceva", 	y_pos,	x_pos);
		sprintf(name_weight,"data/%s_%04d-%04d.dat", "weight_ceva", 		y_pos,	x_pos);
		sprintf(name_w,   	"data/%s_%04d-%04d.dat", "wFiltered_ceva", 		y_pos,x_pos);
		sprintf(name_hdr, 	"data/%s_%04d-%04d.dat", "hdr_ceva", 			y_pos,x_pos);
#else
		sprintf(name_image, "../../data/%s_%04d-%04d.dat", "LongShortImg_vs",y_pos,	x_pos);
		sprintf(name_weight,"../../data/%s_%04d-%04d.dat", "weightvs", 		y_pos,	x_pos);
		sprintf(name_w,   	"../../data/%s_%04d-%04d.dat", "wFiltered_vs", 	y_pos,x_pos);
		sprintf(name_hdr, 	"../../data/%s_%04d-%04d.dat", "hdr_vs", 		y_pos,x_pos);
#endif
		writeFile(pL_S_ImageBuff[buffIdx], 	HDR_BLOCK_W,	2*HDR_BLOCK_H, 	HDR_BLOCK_W,  name_image);
		writeFile(pWeightBuff,  			HDR_BLOCK_W+2,	HDR_BLOCK_H+2, 	HDR_FILTER_W, name_weight);
		writeFile(pWeightFilter,			HDR_BLOCK_W, 	HDR_BLOCK_H, 	HDR_BLOCK_W,  name_w);
		writeFile(pHDRoutBuff, 				HDR_BLOCK_W, 	HDR_BLOCK_H, 	HDR_BLOCK_W,  name_hdr);
		countFiles++;
	}
#endif


	buffIdx++;
}

void hdrprocess_sony_raw(uint16_t 	*src, 
								uint16_t 	*dst, 
								uint16_t 	*simage, 
								int32_t 	W, 
								int32_t 	H, 
								int32_t 	times, 
								int32_t 	noise_thred)
{

	int buffIdx	   = 0;
	int rows	   = ALIGN_CLIP(H,HDR_BLOCK_H);
	int cols	   = ALIGN_CLIP(W,HDR_BLOCK_W);   

	int blkOutCnt  = 0;
	int x_prev	   = 0;
	int y_valid	   = 0;
	int x		   = 0;
	bool bFristCTUline = 0;
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
				hdr_block_process(g_HdrBlkBuf[buffIdx], g_HdrOutBuf[buffIdx], bFristCTUline ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));

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
	hdr_block_process(g_HdrBlkBuf[(buffIdx+1)&1], g_HdrOutBuf[(buffIdx+1)&1], y_valid < HDR_BLOCK_H ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));
    dma_2Dtransf(dst+x_prev, g_HdrOutBuf[(buffIdx+1)&1], y_valid - HDR_PADDING, min_(HDR_BLOCK_H,H-y_valid), min_(HDR_BLOCK_W,W-x_prev),  W , HDR_BLOCK_W);

	// 2 padding line data miss
	
}
