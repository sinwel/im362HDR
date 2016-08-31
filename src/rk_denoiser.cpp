//
/////////////////////////////////////////////////////////////////////////
// File: rk_mfnr.c
// Desc: Implementation of Register
// 
// Date: Revised by yousf 20160820
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "../include/rk_denoiser.h"     // Register
#include "vec-c.h"
#define INIT_PSH_VAL	0
#define NUM_FILTER		6
#define SRC_OFFSET		8
#define COEFF_OFFSET	16
#define STEP			21
#define PATTERN_OFFSET	24
#define SW_CONFIG(init_psh,num_filter,src_offset,coeff_offset,step,pattern)		(((init_psh) & 0x3f) << INIT_PSH_VAL | ((num_filter) & 0x7) << NUM_FILTER     | ((src_offset) & 0x3f) << SRC_OFFSET | ((coeff_offset) & 0x1f) << COEFF_OFFSET | ((step) & 0x7) << STEP | ((pattern) & 0xff) << PATTERN_OFFSET )				

//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: MotionDetectFilter()
// Desc: Motion Detect Filter
//   In: pSrc           - [in] Raw Src data pointer
//       rect           - [in] rects of Raw data
//  Out: pDst           - [out] Raw Filtered data pointer
// 
// Date: Revised by yousf 20160816
// 
/*************************************************************************/
int MotionDetectFilter(RK_U16* pSrc, RK_RectExt rect, RK_U16* pDst)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    // Padding
//     if (rect.rowUseful == rect.rowValid) // top-padding
//     {
// 
//     }
//     if (rect.colUseful == rect.colValid) // left-padding
//     {
// 
//     }
//     if (rect.hgtUseful < rect.hgtValid - (rect.rowUseful - rect.rowValid)) // down-padding
//     {
// 
//     }
//     if (rect.widUseful == rect.widValid - (rect.colUseful - rect.colValid)) // right-padding
//     {
// 
//     }

    int         i0, i1, j0, j1;
    RK_U16      sumFilt;
    int         wid = rect.widExtend; // 
    for (int i=rect.rowUseful - rect.rowValid; i < rect.rowUseful - rect.rowValid + rect.hgtUseful; i++)
    {
        for (int j=rect.colUseful - rect.colValid; j < rect.colUseful - rect.colValid + rect.widUseful; j++)
        {

            //
            i0 = MAX(i - 2, i%2);
            i1 = MIN(i + 2, rect.hgtValid - 1 - (i+1)%2);
            j0 = MAX(j - 2, j%2);
            j1 = MIN(j + 2, rect.widValid - 1 - (j+1)%2);
            //
//            printf("i=%d,j=%d ----i0=%d,i1=%d,j0=%d,j1=%d\n",i,j,i0,i1,j0,j1);
            //
            sumFilt = *(pSrc + (i0 + rect.rowValid - rect.rowExtend) * wid + j0 + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i0 + rect.rowValid - rect.rowExtend) * wid + j  + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i0 + rect.rowValid - rect.rowExtend) * wid + j1 + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i  + rect.rowValid - rect.rowExtend) * wid + j0 + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i  + rect.rowValid - rect.rowExtend) * wid + j  + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i  + rect.rowValid - rect.rowExtend) * wid + j1 + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i1 + rect.rowValid - rect.rowExtend) * wid + j0 + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i1 + rect.rowValid - rect.rowExtend) * wid + j  + rect.colValid - rect.colExtend) * 1
                    + *(pSrc + (i1 + rect.rowValid - rect.rowExtend) * wid + j1 + rect.colValid - rect.colExtend) * 1;
            //
            //*(pDst + (i + rect.rowValid - rect.rowExtend) * wid + j + rect.colValid - rect.colExtend) = (RK_U16)(sumFilt * 1.0 / 9);
            *(pDst + (i + rect.rowValid - rect.rowExtend) * wid + j + rect.colValid - rect.colExtend) = (RK_U16)(sumFilt);

        }
    }
    

    //
    return ret;

#else


#endif
} // MotionDetectFilter()


/************************************************************************/
// Func: LookupMotionDetectThresh()
// Desc: Lookup MotionDetect Threshold
//   In: pRawBase           - [in] Raw Base data pointer
//       rect               - [in] rects of Raw Base data
//       MotionDetectTable  - [in] Motion Detect Table
//  Out: pRawThresh         - [out] RawBaseThresh pointer
// 
// Date: Revised by yousf 20160820
// 
/*************************************************************************/
int LookupMotionDetectThresh(RK_U16* pRawBase, RK_RectExt rect, RK_U16 MotionDetectTable[], RK_U16* pRawThresh)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    //
    RK_U16  baseValue;
    RK_U16  offsetBaseY = rect.rowUseful - rect.rowExtend; 
    RK_U16  offsetBaseX = rect.colUseful - rect.colExtend; 
    for (int r=0; r < rect.hgtUseful; r++)
    {
        for (int c=0; c < rect.widUseful; c++)
        {
            baseValue = *(pRawBase + (r + offsetBaseY) * rect.widExtend + (c + offsetBaseX));// RawBase value
            *(pRawThresh + r * rect.widUseful + c) = MotionDetectTable[baseValue];
        }
    }

    //
    return ret;

#else



#endif
} // LookupMotionDetectThresh()


/************************************************************************/
// Func: RawDstInit()
// Desc: RawDstInit
//   In: rectBase           - [in] rects of RawBase data
//       pRawBaseBlocks     - [in] RawBase data pointer
//  Out: pRawDstSum         - [in/out] RawDstSum data pointer
//       pRawDstWgt         - [in/out] pRawDstWgt data pointer
// 
// Date: Revised by yousf 20160811
// 
/*************************************************************************/
int RawDstInit(RK_RectExt rectBase, RK_U16* pRawBaseBlocks, RK_U16* pRawDstSum, RK_U8* pRawDstWgt)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
    int     offsetBase; // offset

    // RawDstSum init
    offsetBase = (rectBase.rowUseful - rectBase.rowExtend) * rectBase.widExtend + (rectBase.colUseful - rectBase.colExtend);
    for(int r=0; r < rectBase.hgtUseful; r++)
    {
        memcpy(pRawDstSum + r * rectBase.widUseful,
            pRawBaseBlocks + r * rectBase.widExtend + offsetBase,
            rectBase.widUseful * sizeof(RK_U16));
    }

    // RawDstWgt init
    memset(pRawDstWgt, 1, rectBase.hgtUseful * rectBase.widUseful * sizeof(RK_U8));

    //
    return ret;

#else



#endif
} // RawDstSumInit()


/************************************************************************/
// Func: TemporalDenoise()
// Desc: Temporal Denoise
//   In: pRawBaseFilter     - [in] RawBase Filter data pointer
//       rectBase           - [in] rects of RawBase data
//       pBaseBlocksPoint   - [in] BaseBlocks Top-Left-Corners data pointer
//       pRawBaseThresh     - [in] RawBaseThresh data pointer
//       numBlocks          - [in] numBlocks of DSP Chunk
//       pRawRefFilter      - [in] RawRef Filter data pointer
//       rectRef            - [in] rects of RawRef data
//       pProjBlocksPoint   - [in] RefBlocks Top-Left-Corners data pointer
//       pRawRefBlocks      - [in] RawRef data pointer
//       nRawWid            - [in] RawSrcs data width
//       nRawHgt            - [in] RawSrcs data height
//       pRawDstSum         - [in/out] RawDstSum data pointer
//       pRawDstWgt         - [in/out] pRawDstWgt data pointer
//  Out: 
// 
// Date: Revised by yousf 20160820
// 
/*************************************************************************/
int TemporalDenoise(RK_U16* pRawBaseFilter, RK_RectExt rectBase, RK_F32* pBaseBlocksPoint[], RK_U16* pRawBaseThresh, 
    int numBlocks, RK_U16* pRawRefFilter, RK_RectExt rectRef, RK_F32* pProjBlocksPoint[], 
    RK_U16* pRawRefBlocks, RK_U16 nRawWid, RK_U16 nRawHgt,
    RK_U16* pRawDstSum, RK_U8* pRawDstWgt)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    // Motion Detect Result
    RK_U16      baseFilterValue;    // RawBase Filter value
    RK_U16      refFilterValue;     // RawRef Filter value
    RK_U16      refValue;           // RawRef value
    RK_U16      MD_Th;			    // Motion Detect Threshold

    // SubBlock Size of Chunk
    RK_U16      subBlkHgt;          // Sub Block Hgt
    RK_U16      subBlkWid;          // Sub Block Wid

    // SubBlock Offset of Chunk
    RK_U16      offsetBaseY;        // offset
    RK_U16      offsetBaseX;        // offset
    RK_U16      offsetRefY;         // offset
    RK_U16      offsetRefX;         // offset

    //// Base + Ref#k
    for (int n=0; n < numBlocks; n++) // Block#n in Chunk
    {
//         if (   pProjBlocksPoint[n][0] > -RAW_BLK_SIZE-1
//             && pProjBlocksPoint[n][0] <  nRawHgt-1
//             && pProjBlocksPoint[n][1] > -RAW_BLK_SIZE-1
//             && pProjBlocksPoint[n][1] <  nRawWid-1 ) 
        if (   pProjBlocksPoint[n][0] >= 0
            && pProjBlocksPoint[n][0] <= nRawHgt - RAW_BLK_SIZE
            && pProjBlocksPoint[n][1] >= 0
            && pProjBlocksPoint[n][1] <= nRawWid - RAW_BLK_SIZE ) 
        {
            subBlkHgt = (RK_U16)MIN(RAW_BLK_SIZE, rectBase.rowUseful + rectBase.hgtUseful - pBaseBlocksPoint[n][0]);// Sub Block Hgt
            subBlkWid = (RK_U16)MIN(RAW_BLK_SIZE, rectBase.colUseful + rectBase.widUseful - pBaseBlocksPoint[n][1]);// Sub Block Wid
            offsetBaseY = (RK_U16)(pBaseBlocksPoint[n][0] - rectBase.rowExtend); 
            offsetBaseX = (RK_U16)(pBaseBlocksPoint[n][1] - rectBase.colExtend); 
            offsetRefY  = (RK_U16)(pProjBlocksPoint[n][0] - rectRef.rowExtend); 
            offsetRefX  = (RK_U16)(pProjBlocksPoint[n][1] - rectRef.colExtend); 
            for (int r=0; r < subBlkHgt; r++)
            {
                for (int c=0; c < subBlkWid; c++)
                {
#if USE_MOTION_DETECT == 1
                    refValue        = *(pRawRefBlocks  + (r + offsetRefY)  * rectRef.widExtend  + (c + offsetRefX)); // RawRef value
                    refFilterValue  = *(pRawRefFilter  + (r + offsetRefY)  * rectRef.widExtend  + (c + offsetRefX)); // RawRef Filter value
                    baseFilterValue = *(pRawBaseFilter + (r + offsetBaseY) * rectBase.widExtend + (c + offsetBaseX));// RawBase Filter value
                    MD_Th           = *(pRawBaseThresh + r * rectBase.widUseful + c + n*RAW_BLK_SIZE);
                    if (ABS_U16(baseFilterValue - refFilterValue) < MD_Th)
                    {
                        *(pRawDstSum + r * rectBase.widUseful + c + n*RAW_BLK_SIZE) += refValue;
                        *(pRawDstWgt + r * rectBase.widUseful + c + n*RAW_BLK_SIZE) += 1;

                    }
#else
                    refValue  = *(pRawRefBlocks + (r + offsetRefY) * rectRef.widExtend + (c + offsetRefX)); // RawRef value
                    *(pRawDstSum + r * rectBase.widUseful + c + n*RAW_BLK_SIZE) += refValue;
                    *(pRawDstWgt + r * rectBase.widUseful + c + n*RAW_BLK_SIZE) += 1;
#endif

                } // for c
            } // for r

        } // if MV
    } // for n

    //
    return ret;

#else



#endif
} // TemporalDenoise()


/************************************************************************/
// Func: TemporalDenoise_Modify()
// Desc: Temporal Denoise (Modify)
//   In: pRawBlocksData     - [in] RawBlocks data pointer
//       numBlocks          - [in] num Block32x32 of Current Chunk
//       rects              - [in] rects of RawBlock32x32n data
//       nBasePicNum        - [in] Base Picture Num
//       pRawBlkPoints      - [in] RawBlocks Top-Left-Corners data pointer
//       MotionDetectTable  - [in] Motion Detect Table
//       fIspGain           - [in] ISP Gain
//       nBlackLevel        - [in] Black Level
//  Out: pRawDst            - [out] RawDst data pointer
// 
// Date: Revised by yousf 20160822
// 
/*************************************************************************/
int TemporalDenoise_Modify(RK_U16* pRawBlocksData[], int numBlocks, RK_RectExt rects[], 
    int nRawFileNum, int nBasePicNum, RK_F32* pRawBlkPoints[], 
    RK_U16 MotionDetectTable[], RK_F32 fIspGain, RK_S16 nBlackLevel[],
    RK_U16* pRawDst)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    //
    RK_U16*     pTmpSrc = NULL;
    RK_U16*     pTmpDst = NULL;
    int         cntPixel;

    // Motion Detect Result
    RK_U16      baseValue;          // RawBase value
    RK_U16      refValue;           // RawRef value
    RK_U16      baseFilterValue;    // RawBase Filter value
    RK_U16      refFilterValue;     // RawRef Filter value
    RK_U16      MD_Th;			    // Motion Detect Threshold
    RK_F32      dstValue;           // 

    // Block Chunk Hgt
    RK_U16      nBlkHgt;            

    // SubBlock Size of Chunk
    RK_U16      subBlkHgt;          // Sub Block Hgt
    RK_U16      subBlkWid;          // Sub Block Wid

    // SubBlock Offset of Chunk
    RK_U16      offsetBaseY;        // offset
    RK_U16      offsetBaseX;        // offset
    RK_U16      offsetRefY;         // offset
    RK_U16      offsetRefX;         // offset



    //// RawDst Init <- RawBase
    nBlkHgt = (RK_U16)MIN(RAW_BLK_SIZE, rects[nBasePicNum].rowUseful + rects[nBasePicNum].hgtUseful - pRawBlkPoints[nBasePicNum][0]);// Sub Block Hgt
    offsetBaseY = (RK_U16)(pRawBlkPoints[nBasePicNum][0] - rects[nBasePicNum].rowExtend); 
    offsetBaseX = (RK_U16)(pRawBlkPoints[nBasePicNum][1] - rects[nBasePicNum].colExtend); 
    pTmpSrc = pRawBlocksData[nBasePicNum] + offsetBaseY * rects[nBasePicNum].widExtend + offsetBaseX;
    pTmpDst = pRawDst;
    for (int r=0; r < nBlkHgt; r++)
    {
        memcpy(pTmpDst, pTmpSrc, sizeof(RK_U16) * rects[nBasePicNum].widUseful);
        pTmpSrc += rects[nBasePicNum].widExtend;
        pTmpDst += RAW_BLK_SIZE * RAW_WIN_NUM;
    }

    for (int n=0; n < numBlocks; n++) // Block#n in Chunk
    {
        subBlkHgt   = (RK_U16)MIN(RAW_BLK_SIZE, rects[nBasePicNum].rowUseful + rects[nBasePicNum].hgtUseful - pRawBlkPoints[nBasePicNum][n*2+0]);// Sub Block Hgt
        subBlkWid   = (RK_U16)MIN(RAW_BLK_SIZE, rects[nBasePicNum].colUseful + rects[nBasePicNum].widUseful - pRawBlkPoints[nBasePicNum][n*2+1]);// Sub Block Wid
        offsetBaseY = (RK_U16)(pRawBlkPoints[nBasePicNum][n*2+0] - rects[nBasePicNum].rowExtend); 
        offsetBaseX = (RK_U16)(pRawBlkPoints[nBasePicNum][n*2+1] - rects[nBasePicNum].colExtend); 
        for (int r=0; r < subBlkHgt; r++)
        {
            for (int c=0; c < subBlkWid; c++)
            {
                // MotionDetectTh
                baseValue = *(pRawBlocksData[nBasePicNum]  + (r + offsetBaseY) * rects[nBasePicNum].widExtend + (c + offsetBaseX));
                MD_Th     = MotionDetectTable[baseValue];
                
                // RawBase Filter
                baseFilterValue = *(pRawBlocksData[nBasePicNum] + (r-2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c-2 + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r-2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c   + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r-2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c+2 + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r   + offsetBaseY) * rects[nBasePicNum].widExtend + (c-2 + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r   + offsetBaseY) * rects[nBasePicNum].widExtend + (c   + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r   + offsetBaseY) * rects[nBasePicNum].widExtend + (c+2 + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r+2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c-2 + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r+2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c   + offsetBaseX))
                                + *(pRawBlocksData[nBasePicNum] + (r+2 + offsetBaseY) * rects[nBasePicNum].widExtend + (c+2 + offsetBaseX));
                // 
                cntPixel = 1; // init 
                for (int k=0; k < nRawFileNum; k++)
                {
                    if (k != nBasePicNum)
                    {
                        // RawRef Info
                        offsetRefY = (RK_U16)(pRawBlkPoints[k][n*2+0] - rects[k].rowExtend);
                        offsetRefX = (RK_U16)(pRawBlkPoints[k][n*2+1] - rects[k].colExtend); 

                        // RawRef
                        refValue = *(pRawBlocksData[k] + (r + offsetRefY) * rects[k].widExtend + (c + offsetRefX)); // RawRef value

                        // RawRef Filter
                        refFilterValue = *(pRawBlocksData[k] + (r-2 + offsetRefY) * rects[k].widExtend + (c-2 + offsetRefX))
                                       + *(pRawBlocksData[k] + (r-2 + offsetRefY) * rects[k].widExtend + (c   + offsetRefX))
                                       + *(pRawBlocksData[k] + (r-2 + offsetRefY) * rects[k].widExtend + (c+2 + offsetRefX))
                                       + *(pRawBlocksData[k] + (r   + offsetRefY) * rects[k].widExtend + (c-2 + offsetRefX))
                                       + *(pRawBlocksData[k] + (r   + offsetRefY) * rects[k].widExtend + (c   + offsetRefX))
                                       + *(pRawBlocksData[k] + (r   + offsetRefY) * rects[k].widExtend + (c+2 + offsetRefX))
                                       + *(pRawBlocksData[k] + (r+2 + offsetRefY) * rects[k].widExtend + (c-2 + offsetRefX))
                                       + *(pRawBlocksData[k] + (r+2 + offsetRefY) * rects[k].widExtend + (c   + offsetRefX))
                                       + *(pRawBlocksData[k] + (r+2 + offsetRefY) * rects[k].widExtend + (c+2 + offsetRefX));

                        if (ABS_U16(baseFilterValue - refFilterValue) < MD_Th)
                        {
                            *(pRawDst + r * RAW_BLK_SIZE * RAW_WIN_NUM + c + n*RAW_BLK_SIZE) += refValue;
                            cntPixel++;
                        }

                    } // if k
                } // for k
                
                // IspGain 
//                 dstValue  = *(pRawDst + r * RAW_BLK_SIZE * RAW_WIN_NUM + c + n*RAW_BLK_SIZE) * fIspGain;
//                 dstValue /= cntPixel;
//                 dstValue  = dstValue - (fIspGain - 1) * nBlackLevel[r%2 * 2 + c%2] / 4;
//                 *(pRawDst + r * RAW_BLK_SIZE * RAW_WIN_NUM + c + n*RAW_BLK_SIZE) = ROUND_U16(MIN(dstValue, 0x3FF)); // 2^10-1=0x3FF, 2^10*8-1=0x1FFF

                // Gain x8 for WDR-Input 
                dstValue  = *(pRawDst + r * RAW_BLK_SIZE * RAW_WIN_NUM + c + n*RAW_BLK_SIZE) * WDR_GAIN;
                dstValue /= cntPixel;
                //dstValue  = dstValue - (WDR_GAIN - 1) * nBlackLevel[r%2 * 2 + c%2] / 4.0;
                //dstValue  = MAX(dstValue, 0);
                *(pRawDst + r * RAW_BLK_SIZE * RAW_WIN_NUM + c + n*RAW_BLK_SIZE) = ROUND_U16(MIN(dstValue, 0x1FFF));

            } // for c
        } // for r

    } // for n
    


    //
    return ret;

#else

	int ret = 0; // return value

	RK_U8 cvcoeffs[32] = { 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0 };
	RK_U8 cperm1[32] = { 0, 16, 1, 17, 2, 18, 3, 19, 4, 20, 5, 21, 6, 22, 7, 23 };
	RK_U8 cperm2[32] = { 8, 24, 9, 25, 10, 26, 11, 27, 12, 28, 13, 29, 14, 30, 15, 31 };
	RK_U16 cxxx[16] = { 0, 65535, 32768, 21845, 16384, 13107, 10923, 0 };

	uchar32 vcoeffs = *(uchar32*)cvcoeffs;
	uchar32 perm1 = *(uchar32*)cperm1;
	uchar32 perm2 = *(uchar32*)cperm2;
	ushort16 xxx = *(ushort16*)cxxx;

	RK_U16* pAdjBase = NULL;
	RK_U16* pAdjRef = NULL;

	RK_U16      offsetRefY;         // offset
	RK_U16      offsetRefX;         // offset

	ushort16 v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11;
	ushort16 v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23;
	ushort16 v24, v25;
	RK_U16 *p, *p0, *p1, *p2, *p3, *p4, *p5;
	int16 vacc1, vacc2, vacc3, vacc4;
	short16 vabb1, vabb2, vabb3, vabb4;
	ushort16 vabssub1, vabssub2;
	RK_U32 vpr1, vpr2;
	ushort16 lookup1, lookup2;
	ushort16 fenzi1, fenzi2;
	char32 fenmu1, fenmu2;
	//ushort16 vmantissa1, vmantissa2;
	//short16  vexponent1, vexponent2;
	ushort16 lut1, lut2;
	//short blk1, blk2;
	short16 vout1, vout2;
	short16 results1, results2;


	for (int n = 0; n < numBlocks; n++) // Block#n in Chunk
	{
		//base frame 3 rows
		pAdjBase = pRawBlocksData[0] + n * 32 + 2;
		p0 = pAdjBase;
		pAdjBase += 2 * 72;
		p1 = pAdjBase;
		pAdjBase += 2 * 72;
		p2 = pAdjBase;
		pAdjBase += 2 * 72;
		vldchk(p0, v0, v1);
		vldchk(p1, v2, v3);
		vldchk(p2, v4, v5);

		p3 = p0 + 32;
		p4 = p1 + 32;
		p5 = p2 + 32;
		vldchk(p3, v6, v7);
		vldchk(p4, v8, v9);
		vldchk(p5, v10, v11);

		//lookup table
		p = p1 + 2;
		vldchk(p, v24, v25);
		lookup1 = vpld((RK_U16*)MotionDetectTable, (short16)v24);
		lookup2 = vpld((RK_U16*)MotionDetectTable, (short16)v25);

		//fenzi fenmu
		fenzi1 = v24;
		fenmu1 = (char32)1;
		fenzi2 = v25;
		fenmu2 = (char32)1;

		//base filter
		vacc1 = vswmpy5(v0, v6, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
		vacc1 = vswmac5(accumulate, v2, v8, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1);
		vabb1 = vswmac5(psl, v4, v10, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc1);
		vacc2 = vswmpy5(v1, v7, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
		vacc2 = vswmac5(accumulate, v3, v9, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc2);
		vabb2 = vswmac5(psl, v5, v11, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc2);



		//1st frame 3 rows
		offsetRefY = (RK_U16)(pRawBlkPoints[1][n * 2 + 0] - rects[1].rowExtend);
		offsetRefX = (RK_U16)(pRawBlkPoints[1][n * 2 + 1] - rects[1].colExtend);
		pAdjRef = pRawBlocksData[1] + (offsetRefY - 2) * rects[1].widExtend + (offsetRefX - 2);
		p0 = pAdjRef;
		pAdjRef += 2 * rects[1].widExtend;
		p1 = pAdjRef;
		pAdjRef += 2 * rects[1].widExtend;
		p2 = pAdjRef;
		pAdjRef += 2 * rects[1].widExtend;
		vldchk(p0, v12, v13);
		vldchk(p1, v14, v15);
		vldchk(p2, v16, v17);

		p3 = p0 + 32;
		p4 = p1 + 32;
		p5 = p2 + 32;
		vldchk(p3, v18, v19);
		vldchk(p4, v20, v21);
		vldchk(p5, v22, v23);

		for (RK_U32 row = 0; row < 32;)
		{
			/*
			if (row % 2){
				blk1 = (nBlackLevel[0] * 7 + 2) / 4;
				blk2 = (nBlackLevel[1] * 7 + 2) / 4;
			}
			else{
				blk1 = (nBlackLevel[2] * 7 + 2) / 4;
				blk2 = (nBlackLevel[3] * 7 + 2) / 4;
			}
			*/
			//1st filter
			vacc3 = vswmpy5(v12, v18, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc3 = vswmac5(accumulate, v14, v20, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc3);
			vabb3 = vswmac5(psl, v16, v22, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc3);
			vacc4 = vswmpy5(v13, v19, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc4 = vswmac5(accumulate, v15, v21, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc4);
			vabb4 = vswmac5(psl, v17, v23, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc4);

			//abssub
			vabssub1 = vabssub(vabb1, vabb3);
			vabssub2 = vabssub(vabb2, vabb4);

			//flag
			vpr1 = vcmp(le, vabssub1, lookup1);
			vpr2 = vcmp(le, vabssub2, lookup2);

			//overlap
			p = p1 + 2;
			vldchk(p, v24, v25);
			fenzi1 = vselect(vadd(fenzi1, v24), fenzi1, vpr1);
			fenmu1 = vselect(vadd(fenmu1, (char32)1), fenmu1, vpr1);
			fenzi2 = vselect(vadd(fenzi2, v25), fenzi2, vpr2);
			fenmu2 = vselect(vadd(fenmu2, (char32)1), fenmu2, vpr2);


			//2nd frame 3 rows
			offsetRefY = (RK_U16)(pRawBlkPoints[2][n * 2 + 0] - rects[2].rowExtend);
			offsetRefX = (RK_U16)(pRawBlkPoints[2][n * 2 + 1] - rects[2].colExtend);
			pAdjRef = pRawBlocksData[2] + (offsetRefY - 2 + row) * rects[2].widExtend + (offsetRefX - 2);
			p0 = pAdjRef;
			pAdjRef += 2 * rects[2].widExtend;
			p1 = pAdjRef;
			pAdjRef += 2 * rects[2].widExtend;
			p2 = pAdjRef;
			pAdjRef += 2 * rects[2].widExtend;
			vldchk(p0, v12, v13);
			vldchk(p1, v14, v15);
			vldchk(p2, v16, v17);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v18, v19);
			vldchk(p4, v20, v21);
			vldchk(p5, v22, v23);

			//2nd filter
			vacc3 = vswmpy5(v12, v18, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc3 = vswmac5(accumulate, v14, v20, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc3);
			vabb3 = vswmac5(psl, v16, v22, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc3);
			vacc4 = vswmpy5(v13, v19, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc4 = vswmac5(accumulate, v15, v21, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc4);
			vabb4 = vswmac5(psl, v17, v23, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc4);

			//abssub
			vabssub1 = vabssub(vabb1, vabb3);
			vabssub2 = vabssub(vabb2, vabb4);

			//flag
			vpr1 = vcmp(le, vabssub1, lookup1);
			vpr2 = vcmp(le, vabssub2, lookup2);

			//overlap
			p = p1 + 2;
			vldchk(p, v24, v25);
			fenzi1 = vselect(vadd(fenzi1, v24), fenzi1, vpr1);
			fenmu1 = vselect(vadd(fenmu1, (char32)1), fenmu1, vpr1);
			fenzi2 = vselect(vadd(fenzi2, v25), fenzi2, vpr2);
			fenmu2 = vselect(vadd(fenmu2, (char32)1), fenmu2, vpr2);


			//3rd frame 3 rows
			offsetRefY = (RK_U16)(pRawBlkPoints[3][n * 2 + 0] - rects[3].rowExtend);
			offsetRefX = (RK_U16)(pRawBlkPoints[3][n * 2 + 1] - rects[3].colExtend);
			pAdjRef = pRawBlocksData[3] + (offsetRefY - 2 + row) * rects[3].widExtend + (offsetRefX - 2);
			p0 = pAdjRef;
			pAdjRef += 2 * rects[3].widExtend;
			p1 = pAdjRef;
			pAdjRef += 2 * rects[3].widExtend;
			p2 = pAdjRef;
			pAdjRef += 2 * rects[3].widExtend;
			vldchk(p0, v12, v13);
			vldchk(p1, v14, v15);
			vldchk(p2, v16, v17);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v18, v19);
			vldchk(p4, v20, v21);
			vldchk(p5, v22, v23);

			//3rd filter
			vacc3 = vswmpy5(v12, v18, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc3 = vswmac5(accumulate, v14, v20, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc3);
			vabb3 = vswmac5(psl, v16, v22, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc3);
			vacc4 = vswmpy5(v13, v19, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc4 = vswmac5(accumulate, v15, v21, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc4);
			vabb4 = vswmac5(psl, v17, v23, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc4);

			//abssub
			vabssub1 = vabssub(vabb1, vabb3);
			vabssub2 = vabssub(vabb2, vabb4);

			//flag
			vpr1 = vcmp(le, vabssub1, lookup1);
			vpr2 = vcmp(le, vabssub2, lookup2);

			//overlap
			p = p1 + 2;
			vldchk(p, v24, v25);
			fenzi1 = vselect(vadd(fenzi1, v24), fenzi1, vpr1);
			fenmu1 = vselect(vadd(fenmu1, (char32)1), fenmu1, vpr1);
			fenzi2 = vselect(vadd(fenzi2, v25), fenzi2, vpr2);
			fenmu2 = vselect(vadd(fenmu2, (char32)1), fenmu2, vpr2);


			//4th frame 3 rows
			offsetRefY = (RK_U16)(pRawBlkPoints[4][n * 2 + 0] - rects[4].rowExtend);
			offsetRefX = (RK_U16)(pRawBlkPoints[4][n * 2 + 1] - rects[4].colExtend);
			pAdjRef = pRawBlocksData[4] + (offsetRefY - 2 + row) * rects[4].widExtend + (offsetRefX - 2);
			p0 = pAdjRef;
			pAdjRef += 2 * rects[4].widExtend;
			p1 = pAdjRef;
			pAdjRef += 2 * rects[4].widExtend;
			p2 = pAdjRef;
			pAdjRef += 2 * rects[4].widExtend;
			vldchk(p0, v12, v13);
			vldchk(p1, v14, v15);
			vldchk(p2, v16, v17);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v18, v19);
			vldchk(p4, v20, v21);
			vldchk(p5, v22, v23);

			//4th filter
			vacc3 = vswmpy5(v12, v18, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc3 = vswmac5(accumulate, v14, v20, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc3);
			vabb3 = vswmac5(psl, v16, v22, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc3);
			vacc4 = vswmpy5(v13, v19, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc4 = vswmac5(accumulate, v15, v21, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc4);
			vabb4 = vswmac5(psl, v17, v23, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc4);

			//abssub
			vabssub1 = vabssub(vabb1, vabb3);
			vabssub2 = vabssub(vabb2, vabb4);

			//flag
			vpr1 = vcmp(le, vabssub1, lookup1);
			vpr2 = vcmp(le, vabssub2, lookup2);

			//overlap
			p = p1 + 2;
			vldchk(p, v24, v25);
			fenzi1 = vselect(vadd(fenzi1, v24), fenzi1, vpr1);
			fenmu1 = vselect(vadd(fenmu1, (char32)1), fenmu1, vpr1);
			fenzi2 = vselect(vadd(fenzi2, v25), fenzi2, vpr2);
			fenmu2 = vselect(vadd(fenmu2, (char32)1), fenmu2, vpr2);


			//5th frame 3 rows
			offsetRefY = (RK_U16)(pRawBlkPoints[5][n * 2 + 0] - rects[5].rowExtend);
			offsetRefX = (RK_U16)(pRawBlkPoints[5][n * 2 + 1] - rects[5].colExtend);
			pAdjRef = pRawBlocksData[5] + (offsetRefY - 2 + row) * rects[5].widExtend + (offsetRefX - 2);
			p0 = pAdjRef;
			pAdjRef += 2 * rects[5].widExtend;
			p1 = pAdjRef;
			pAdjRef += 2 * rects[5].widExtend;
			p2 = pAdjRef;
			pAdjRef += 2 * rects[5].widExtend;
			vldchk(p0, v12, v13);
			vldchk(p1, v14, v15);
			vldchk(p2, v16, v17);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v18, v19);
			vldchk(p4, v20, v21);
			vldchk(p5, v22, v23);

			//5th filter
			vacc3 = vswmpy5(v12, v18, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc3 = vswmac5(accumulate, v14, v20, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc3);
			vabb3 = vswmac5(psl, v16, v22, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc3);
			vacc4 = vswmpy5(v13, v19, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc4 = vswmac5(accumulate, v15, v21, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc4);
			vabb4 = vswmac5(psl, v17, v23, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc4);

			//abssub
			vabssub1 = vabssub(vabb1, vabb3);
			vabssub2 = vabssub(vabb2, vabb4);

			//flag
			vpr1 = vcmp(le, vabssub1, lookup1);
			vpr2 = vcmp(le, vabssub2, lookup2);

			//overlap
			p = p1 + 2;
			vldchk(p, v24, v25);
			fenzi1 = vselect(vadd(fenzi1, v24), fenzi1, vpr1);
			fenmu1 = vselect(vadd(fenmu1, (char32)1), fenmu1, vpr1);
			fenzi2 = vselect(vadd(fenzi2, v25), fenzi2, vpr2);
			fenmu2 = vselect(vadd(fenmu2, (char32)1), fenmu2, vpr2);

			/*
			vinv(absexp, fenmu1, vmantissa1, vexponent1);
			vinv(absexp, fenmu2, vmantissa2, vexponent2);
			vout1 = vmpynorm(rnd, vmantissa1, fenzi1, ((ushort16)vexponent1 + (ushort16)16));
			vout2 = vmpynorm(rnd, vmantissa2, fenzi2, ((ushort16)vexponent2 + (ushort16)16));
			*/


			lut1 = (ushort16)vlut((ushort16)xxx, (ushort16)xxx, fenmu1, (short16)0);
			lut2 = (ushort16)vlut((ushort16)xxx, (ushort16)xxx, fenmu2, (short16)0);
			//vout1 = vmpynorm(rnd, lut1, fenzi1, (ushort16)16);
			//vout2 = vmpynorm(rnd, lut2, fenzi2, (ushort16)16);
			
			vout1 = vmpynorm(rnd, lut1, fenzi1, (ushort16)13);
			vout2 = vmpynorm(rnd, lut2, fenzi2, (ushort16)13);
			//vout1 = vsub(vout1, blk1);
			//vout2 = vsub(vout2, blk2);
			vout1 = vmin(vout1, (short16)8191);
			vout2 = vmin(vout2, (short16)8191);
			

			results1 = vperm(vout1, vout2, perm1);
			results2 = vperm(vout1, vout2, perm2);

			vst(results1, (short16*)(pRawDst + row * 64 + n * 32), 0xFFFF);
			vst(results2, (short16*)(pRawDst + row * 64 + n * 32 + 16), 0xFFFF);

			++row;
			//base frame 3 rows
			pAdjBase = pRawBlocksData[0] + row * 72 + n * 32 + 2;
			p0 = pAdjBase;
			pAdjBase += 2 * 72;
			p1 = pAdjBase;
			pAdjBase += 2 * 72;
			p2 = pAdjBase;
			pAdjBase += 2 * 72;
			vldchk(p0, v0, v1);
			vldchk(p1, v2, v3);
			vldchk(p2, v4, v5);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v6, v7);
			vldchk(p4, v8, v9);
			vldchk(p5, v10, v11);

			//lookup table
			p = p1 + 2;
			vldchk(p, v24, v25);
			lookup1 = vpld((RK_U16*)MotionDetectTable, (short16)v24);
			lookup2 = vpld((RK_U16*)MotionDetectTable, (short16)v25);

			//fenzi fenmu
			fenzi1 = v24;
			fenmu1 = (char32)1;
			fenzi2 = v25;
			fenmu2 = (char32)1;

			//base filter
			vacc1 = vswmpy5(v0, v6, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc1 = vswmac5(accumulate, v2, v8, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc1);
			vabb1 = vswmac5(psl, v4, v10, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc1);
			vacc2 = vswmpy5(v1, v7, vcoeffs, SW_CONFIG(0, 0, 0, 0, 0, 0));
			vacc2 = vswmac5(accumulate, v3, v9, vcoeffs, SW_CONFIG(0, 0, 0, 4, 0, 0), vacc2);
			vabb2 = vswmac5(psl, v5, v11, vcoeffs, SW_CONFIG(0, 0, 0, 8, 0, 0), vacc2);



			//1st frame 3 rows
			offsetRefY = (RK_U16)(pRawBlkPoints[1][n * 2 + 0] - rects[1].rowExtend);
			offsetRefX = (RK_U16)(pRawBlkPoints[1][n * 2 + 1] - rects[1].colExtend);
			pAdjRef = pRawBlocksData[1] + (offsetRefY - 2 + row) * rects[1].widExtend + (offsetRefX - 2);
			p0 = pAdjRef;
			pAdjRef += 2 * rects[1].widExtend;
			p1 = pAdjRef;
			pAdjRef += 2 * rects[1].widExtend;
			p2 = pAdjRef;
			pAdjRef += 2 * rects[1].widExtend;
			vldchk(p0, v12, v13);
			vldchk(p1, v14, v15);
			vldchk(p2, v16, v17);

			p3 = p0 + 32;
			p4 = p1 + 32;
			p5 = p2 + 32;
			vldchk(p3, v18, v19);
			vldchk(p4, v20, v21);
			vldchk(p5, v22, v23);
		}
	}

	return ret;

#endif
} // TemporalDenoise_Modify()


/************************************************************************/
// Func: RawDstNormalize()
// Desc: Normalization
//   In: rectBase           - [in] rects of RawBase data
//       ispGain            - [in] ISP Gain
//       pRawDstSum         - [in/out] RawDstSum data pointer
//       pRawDstWgt         - [in/out] pRawDstWgt data pointer
//  Out: 
// 
// Date: Revised by yousf 20160811
// 
/*************************************************************************/
int RawDstNormalize(RK_RectExt rectBase, RK_F32 ispGain, RK_U16* pRawDstSum, RK_U8* pRawDstWgt)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    RK_F32  tmp = 0.0; // 
    //
    for (int r=0; r < rectBase.hgtUseful; r++)
    {
        for (int c=0; c < rectBase.widUseful; c++)
        {
            /*/ LinearGain = 1
            *(pRawDstSum + r * rectBase.widUseful + c) /= *(pRawDstWgt + r * rectBase.widUseful + c);
            //*/

            /*/ LinearGain = ispGain
            tmp = *(pRawDstSum + r * rectBase.widUseful + c) * ispGain;
            tmp = tmp / *(pRawDstWgt + r * rectBase.widUseful + c);
            tmp = tmp - (ispGain - 1) * 64;
            *(pRawDstSum + r * rectBase.widUseful + c) = ROUND_U16(MIN(tmp, 0x3FF));
            //*/

            // WDR Gain = 8
            tmp  = *(pRawDstSum + r * rectBase.widUseful + c) * WDR_GAIN; // FixedGain = 8 = 2^3
            tmp /= *(pRawDstWgt + r * rectBase.widUseful + c);
            tmp = tmp - (WDR_GAIN - 1) * 64;
            *(pRawDstSum + r * rectBase.widUseful + c) = ROUND_U16(MIN(tmp, 0x1FFF)); // 1FFF = 2^13-1
            //*/

        }
    }

    //
    return ret;

#else



#endif
}
//////////////////////////////////////////////////////////////////////////



