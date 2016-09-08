


#include <string.h>
#include "DebugFiles.h"
#include "rk_bayerwdr.h"

#if HDR_DEBUG_ENABLE
static int x_pos ;
static int y_pos ;
#endif
static int countFiles = 0;
// define the block parameters.

RK_U16		p_u16Src[8*52] PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") =
{
	#include "../data/data8x52.dat"
};
RK_U8		p_u16Tab[961]  PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") =
{
	#include "../table/tone_mapping_961.dat"
};

RK_U16		pL_S_ImageBuff[2][2*HDR_BLOCK_H*HDR_BLOCK_W] 	PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA") = {0};
RK_U8 		pWeightBuff[(HDR_BLOCK_H+2)*(HDR_BLOCK_W+2)] 	PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA") = {0};
RK_U8		pWeightFilter[HDR_BLOCK_H*HDR_BLOCK_W] 			PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA") = {0};

RK_U16 		g_HdrBlkBuf[2][(HDR_BLOCK_H+2*HDR_PADDING)*(HDR_BLOCK_W+2*HDR_PADDING)] 	PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	= {0};
RK_U16 		g_HdrOutBuf[2][HDR_BLOCK_H*HDR_BLOCK_W]										PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	= {0};
RK_U16		g_HdrRowBuf[2*HDR_PADDING*4096] 											PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	= {0};		// 2* Line
RK_U16		g_HdrColBuf[HDR_PADDING*HDR_BLOCK_H]										PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA")	= {0};	  	// 2  col


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
int CopyBlockData(RK_U16* pSrc, RK_U16* pDst, int nWid, int nHgt, int nSrcStride, int nDstStride)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    RK_U8*     p_src = (RK_U8*)pSrc; // temp pointer
    RK_U8*     p_dst = (RK_U8*)pDst; // temp pointer
    for (int i=0; i < nHgt; i++)
    {
        memcpy(p_dst, p_src, nWid*sizeof(RK_U16));
        p_src += nSrcStride;
        p_dst += nDstStride;
    }

    //
    return ret;

#else
    //
    int     ret = 0; // return value

    RK_U8*     p_src = (RK_U8*)pSrc; // temp pointer
    RK_U8*     p_dst = (RK_U8*)pDst; // temp pointer
    for (int i=0; i < nHgt; i++)
    {
        memcpy(p_dst, p_src, nWid*sizeof(RK_U16));
        p_src += nSrcStride;
        p_dst += nDstStride;
    }

    //
    return ret;

#endif
}

void residualLUT(RK_U16 *p_u16Long, 	//<<! [in] long time image.
					RK_U16 *p_u16Short, //<<! [in] short time image.
					RK_U16 *p_u16Tab, 	//<<! [in] table
					RK_U16 *p_u16Weight,//<<! [out] bilinear weight
					int stride, 		//<<! [in] residual scale factor
					RK_U16  normValue, 	//<<! [in] refValue
					unsigned int u32Cols);
void Max3x3AndBilinear (RK_U8  *p_u8Weight, 	//<<! [in] 0-255 scale tab.
							 RK_U16 *p_u16ImageL_S,	//<<! [in] long and short image
							 int s32SrcStep, 
							 int s32DstStep, 
							 int u32Rows, 
							 int u32Cols,
						#if HDR_DEBUG_ENABLE
							 RK_U16 *p_u16Dst,
							 RK_U8  *p_u8FilterW,
							 int xPos,
							 int yPos);
						#else
							 RK_U16 *p_u16Dst);
						#endif

void zigzagDebayer(	RK_U16 *p_u16Src, 
						RK_U8  *p_u16Tab, 
						RK_U16 blockW,
						RK_U16 blockH,
						RK_U16 stride,
						RK_U16 normValue,
						RK_U16 *buff,	//<<! [out] 32x64 short16  long and short
						RK_U8  *scale);



unsigned short	cacheline[12*4096];
unsigned short  dstline[8*4096];

void dma_cachetransf(unsigned short *frame, unsigned short *cache, int yoffset, int lines, int W, int stride_o, int stride_i)
{
	frame = frame + stride_o*yoffset;
	for (int y = 0; y < lines; y++)
	{
		for (int x = 0; x < W; x++)
		{
			cache[x] = frame[x];
		}
		cache += stride_i;
		frame += (stride_o);
	}
}

void dma_outtransf(unsigned short *frame, unsigned short *cache, int yoffset, int lines, int W, int stride_o, int stride_i)
{
	frame = frame + stride_o*yoffset;
	for (int y = 0; y < lines; y++)
	{
		for (int x = 0; x < W; x++)
		{
			frame[x] = cache[x];
		}
		cache += stride_i;
		frame += (stride_o);
	}
}




void hdr_block_process(RK_U16 *pRawInBuff, RK_U16 *pHDRoutBuff, bool bFristCTUline,int validW, int validH)
{
	int 	ret,i,j,buffIdx = 0; 
	int 	stride = 32;
	RK_U16  normValue = 1024 - 64;
	
	// bFristCTUline = true, store out only HDR_BLOCK_H - 2 valid.
	
	zigzagDebayer(	pRawInBuff,
					p_u16Tab,
					validW,
					validH,
					HDR_SRC_STRIDE,
					1024-64,
					pL_S_ImageBuff[buffIdx],
					pWeightBuff+1+HDR_FILTER_W); // need be clear to zeros.	

#if HDR_DEBUG_ENABLE
    char name_image[512], name_weight[512];;
	if  ( y_pos == 6 && x_pos == 10  )
	{
#if __XM4__
		sprintf(name_image, "%s_%04d-%04d.dat", "LongShortImg_ceva", y_pos,x_pos);
		sprintf(name_weight, "%s_%04d-%04d.dat", "weight_ceva", y_pos,x_pos);
#else
		sprintf(name_image, "%s_%04d-%04d.dat", "LongShortImg_vs", y_pos,x_pos);
		sprintf(name_weight, "%s_%04d-%04d.dat", "weightvs", y_pos,x_pos);
#endif
		writeFile(pL_S_ImageBuff[buffIdx], 	HDR_BLOCK_W,2*HDR_BLOCK_H, HDR_BLOCK_W,  name_image);
		writeFile(pWeightBuff,  			HDR_BLOCK_W+2,HDR_BLOCK_H+2, HDR_FILTER_W, name_weight);
		//countFiles++;
	}
#endif


	Max3x3AndBilinear(	pWeightBuff,
						pL_S_ImageBuff[buffIdx], 						 
						HDR_FILTER_W, 
						HDR_BLOCK_W, 
						HDR_BLOCK_H, 
						HDR_BLOCK_W,		
					#if HDR_DEBUG_ENABLE
						pHDRoutBuff,
						pWeightFilter,
						x_pos,
						y_pos);
					#else
						pHDRoutBuff);
					#endif
				

#if HDR_DEBUG_ENABLE
    char name_hdr[512],name_w[512];
	if  ( y_pos == 6 && x_pos == 10  )
	{
#if __XM4__
		sprintf(name_w,   "%s_%04d-%04d.dat", "wFiltered_ceva", y_pos,x_pos);
		sprintf(name_hdr, "%s_%04d-%04d.dat", "hdr_ceva", y_pos,x_pos);
#else
		sprintf(name_w,   "%s_%04d-%04d.dat", "wFiltered_vs", y_pos,x_pos);
		sprintf(name_hdr, "%s_%04d-%04d.dat", "hdr_vs", y_pos,x_pos);
#endif
		writeFile(pWeightFilter, HDR_BLOCK_W, HDR_BLOCK_H, HDR_BLOCK_W, name_w);
		writeFile(pHDRoutBuff, HDR_BLOCK_W, HDR_BLOCK_H, HDR_BLOCK_W, name_hdr);
		countFiles++;
	}
#endif


	buffIdx++;
}

void hdrprocess_sony_raw(unsigned short *src, unsigned short *dst, unsigned short *simage, int W, int H, int times, int noise_thred)
{
#if 0
	unsigned short	*psrc[12];
	unsigned short	*pdst[8];
	int				y_offset;

	for (int i = 0; i < 12; i++)
		psrc[i] = cacheline[i];

	for (int i = 0; i < 8; i++)
		pdst[i] = dstline[i];


	dma_cachetransf(src, psrc[2]+32, 0, 6, W, W, 4096);
	y_offset = 6;
	for (int y = 0; y < H; y += 4)
	{
		int	read_lines = 4;
		if (y_offset > H - 4)
		{
			read_lines = H - y_offset;
		}
		if (read_lines > 0)
		{
			dma_cachetransf(src, psrc[8]+32, y_offset, read_lines, W, W, 4096);
			y_offset += read_lines;
		}

		for (int x = 0; x < W; x += 32)
		{
			hdr_block_process(psrc, pdst, x+32, x);
		}

		dma_outtransf(dst, pdst[0], y, 4, W, W , 4096);


		for (int i = 0; i < 4; i++)
		{
			unsigned short *ptmp;

			ptmp = psrc[i];
			psrc[i] = psrc[i + 4];
			psrc[i + 4] = psrc[i + 8];
			psrc[i + 8] = ptmp;
		}

		for (int i = 0; i < 4; i++)
		{
			unsigned short *ptmp;

			ptmp = pdst[i];
			pdst[i] = pdst[i + 4];
			pdst[i + 4] = ptmp;
		}
	}
#else
	int padding	   = HDR_PADDING;
	int blkWid	   = HDR_BLOCK_W;
	int blkHgt	   = HDR_BLOCK_H;
	int buffIdx	   = 0;
	//int HDR_SRC_STRIDE = blkWid + 2*padding;
	int rows	   = ((H+blkHgt-1)/blkHgt)*blkHgt;
	int cols	   = ((W+blkWid-1)/blkWid)*blkWid;

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
		    CopyBlockData(src+x+y*W, 	g_HdrBlkBuf[buffIdx]+4*HDR_SRC_STRIDE+2, min_(HDR_BLOCK_W,W-x), min_(HDR_BLOCK_H,H-y), W*2, HDR_SRC_STRIDE*2);

		    // Fill 4-TopExternalRows from RowBuf
		    CopyBlockData(g_HdrRowBuf+x,g_HdrBlkBuf[buffIdx],                HDR_SRC_STRIDE, 4,    4096*2, HDR_SRC_STRIDE*2);

		    // Fill 2-LeftCol from ColBuf
		    CopyBlockData(g_HdrColBuf,  g_HdrBlkBuf[buffIdx]+4*HDR_SRC_STRIDE,   2,     HDR_BLOCK_H, 	 2*2, HDR_SRC_STRIDE*2);

		    // Update 2-RightCol back.
		    CopyBlockData(g_HdrBlkBuf[buffIdx]+5*HDR_SRC_STRIDE-4, g_HdrColBuf,  2, 	  HDR_BLOCK_H,    HDR_SRC_STRIDE*2, 2*2);


			
			buffIdx = (buffIdx + 1) & 0x1; // odd-even for LoadData

		    if (x==0 && y==0)
		    {
		        // First Block of Image
		        continue;
		    }
		    else
		    {
		        // Fill 2-RightCol from AnotherBuf
				CopyBlockData(g_HdrBlkBuf[(buffIdx+1)&1] + 4*HDR_SRC_STRIDE + 2, 
				              g_HdrBlkBuf[buffIdx] + 5*HDR_SRC_STRIDE - 2, 
				              2, HDR_BLOCK_H, HDR_SRC_STRIDE*2, HDR_SRC_STRIDE*2);

				// Update 4-BottomRows to RowBuf, waiting bottom-right corner data.
				CopyBlockData(g_HdrBlkBuf[buffIdx] + HDR_BLOCK_H*HDR_SRC_STRIDE, 
				              g_HdrRowBuf + x_prev, 
				              HDR_SRC_STRIDE, 4, HDR_SRC_STRIDE*2, 4096*2);

				
				y_valid = (x == 0)? y - HDR_BLOCK_H : y;
				y_pos   = y_valid/HDR_BLOCK_H;
				bFristCTUline = y_valid < HDR_BLOCK_H;
				hdr_block_process(g_HdrBlkBuf[buffIdx], g_HdrOutBuf[buffIdx], bFristCTUline ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));

				if (bFristCTUline) // skip the first "HDR_PADDING" line data.
			        dma_outtransf(dst+x_prev, g_HdrOutBuf[buffIdx] + HDR_PADDING*HDR_BLOCK_W, 
			        	y_valid , HDR_BLOCK_H - HDR_PADDING, min_(HDR_BLOCK_W,W-x_prev),  W , HDR_BLOCK_W);
				else
			        dma_outtransf(dst+x_prev, g_HdrOutBuf[buffIdx], y_valid - HDR_PADDING, HDR_BLOCK_H, min_(HDR_BLOCK_W,W-x_prev),  W , HDR_BLOCK_W);

			}
			x_prev = x;	
		}

	}

	// do last block
	hdr_block_process(g_HdrBlkBuf[(buffIdx+1)&1], g_HdrOutBuf[(buffIdx+1)&1], y_valid < HDR_BLOCK_H ,min_(HDR_BLOCK_W,W-x_prev), min_(HDR_BLOCK_H,H-y_valid));
    dma_outtransf(dst+x_prev, g_HdrOutBuf[(buffIdx+1)&1], y_valid - HDR_PADDING, min_(HDR_BLOCK_H,H-y_valid), min_(HDR_BLOCK_W,W-x_prev),  W , HDR_BLOCK_W);


#endif

}
