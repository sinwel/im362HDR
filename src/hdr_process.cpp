#include <string.h>
#include "rk_typedef.h"              // Type definition
#include "rk_bayerwdr.h"
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
void Max3x3(	RK_U16 *p_u16Src, 
				RK_U16 *p_u16Dst, 
				int s32SrcStep, 
				int s32DstStep, 
				unsigned int u32Rows, 
				unsigned int u32Cols);

void zigzagDebayer(	RK_U16 *p_u16Src, 
						RK_U16 *p_u16Tab, 
						RK_U16 width,
						RK_U16 stride,
						RK_U16 normValue,
						RK_U16 *p_u16Dst	//<<! [out]
						) ;



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



RK_U16		p_u16Src[8*52] =
{
	#include "../data/data8x52.dat"
};
RK_U16		p_u16Tab[961] =
{
	#include "../table/tone_mapping_961.dat"
};

RK_U16		pL_S_ImageBuffA[2*4*32];
RK_U16		pL_S_ImageBuffB[2*4*32];

RK_U16		pWeightBuffA[4*32];
RK_U16		pWeightBuffB[4*32];

RK_U16		pHDRout[4*32];

void hdr_block_process(RK_U16 *pRawInBuff, RK_U16 *pHDRoutBuff, int offset_pi, int offset_pd)
{
	int 	ret,i,j,pingpang = 0; 
	int 	stride = 32;
	RK_U16  normValue = 1024 - 64;
	RK_U16	*plongshortBuff;
	RK_U16	*pWeightBuff;
	
	if (pingpang & 1)
	{
		plongshortBuff	= pL_S_ImageBuffA;
		pWeightBuff		= pWeightBuffA;
	}
	else
	{
		plongshortBuff	= pL_S_ImageBuffB;
		pWeightBuff		= pWeightBuffB;
	}
	pingpang++;
	zigzagDebayer(	p_u16Src,
					p_u16Tab,
					32,
					52,
					1024-64,
					plongshortBuff	//<<! [out]
						); 	

	residualLUT(plongshortBuff, 		//<<! [in] long time image.
				plongshortBuff + 4*32, 	//<<! [in] short time image.
				p_u16Tab, 				//<<! [in] table
				pWeightBuff,			//<<! [out] bilinear weight
				stride, 				//<<! [in] 
				normValue, 				//<<! [in] refValue
				32);	
	
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
	int padding	   = 2;
	int blkWid	   = 64;
	int blkHgt	   = 32;
	int buffIdx	   = 0;
	int nHdrBufWid = blkWid + 2*padding;
	int rows	   = ((H+blkHgt-1)/blkHgt)*blkHgt;
	int cols	   = ((W+blkWid-1)/blkWid)*blkWid;

	
	RK_U16 	pHdrRawBlockBuf[2][(32+4)*(64+2*2)]= {0};
	RK_U16 	pHdrOutBuf[2][32*64]		= {0};
	RK_U16	pHdrRawRowBuf[4*4096] 	= {0};		// 2* Line
	RK_U16	pHdrRawColBuf[2*32]		= {0};	  	// 2  col

	
	for (int y = 0; y < rows; y += blkHgt)
	{

		for (int x = 0; x < cols; x += blkWid)
		{

			// Fill Block32x64 from TemporalDenoise
		    CopyBlockData(src+x, 		  pHdrRawBlockBuf[buffIdx]+4*nHdrBufWid+2, min_(blkWid,W-x), min_(blkHgt,H-y), W*2, nHdrBufWid*2);

		    // Fill 4-TopExternalRows from RowBuf
		    CopyBlockData(pHdrRawRowBuf+x,pHdrRawBlockBuf[buffIdx],                nHdrBufWid, 4,    4096*2, nHdrBufWid*2);

		    // Fill 2-LeftCol from ColBuf
		    CopyBlockData(pHdrRawColBuf,  pHdrRawBlockBuf[buffIdx]+4*nHdrBufWid,   2,     blkHgt, 	 2*2, nHdrBufWid*2);

		    // Update 2-RightCol
		    CopyBlockData(pHdrRawBlockBuf[buffIdx]+5*nHdrBufWid-4, pHdrRawColBuf,  2, 	  blkHgt,    nHdrBufWid*2, 2*2);


			
			buffIdx = (buffIdx + 1) & 0x1; // odd-even for LoadData

		    if (x==0 && y==0)
		    {
		        // First Block of Image
		        continue;
		    }
		    else
		    {
		        // Fill 2-RightCol from AnotherBuf
				CopyBlockData(pHdrRawBlockBuf[(buffIdx+1)&1] + 4*nHdrBufWid + 2, 
				              pHdrRawBlockBuf[buffIdx] + 5*nHdrBufWid - 2, 
				              2, blkHgt, nHdrBufWid*2, nHdrBufWid*2);

				// Update 4-BottomRows to RowBuf, waiting bottom-right corner data.
				CopyBlockData(pHdrRawBlockBuf[buffIdx] + blkHgt*nHdrBufWid, 
				              pHdrRawRowBuf + x - blkWid, 
				              nHdrBufWid, 4, nHdrBufWid*2, 4096*2);

				;//hdr_block_process(pHdrRawBlockBuf[buffIdx], pHdrOutBuf[buffIdx], x+32, x);
		    }

			
		}


	}



#endif

}
