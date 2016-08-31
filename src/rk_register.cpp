//
/////////////////////////////////////////////////////////////////////////
// File: rk_mfnr.c
// Desc: Implementation of Register
// 
// Date: Revised by yousf 20160824
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "../include/rk_register.h"     // Register


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: FeatureDetect()
// Desc: Feature Detect
//   In: pThumbDspChunk         - [in] Thumb data pointer
//       nWid                   - [in] Thumb data width
//       nHgt                   - [in] Thumb data height
//       nStride                - [in] Thumb data stride
//       rowSeg                 - [in] Seg(rowSeg,m)
//       numFeature             - [in] num Feature in input Thumb data 
//       pThumbFilterDspChunk   - [in] Thumb Filter data pointer: 9x256*2B
//       pWdrWeightMat          - [in] Weight Mat data pointer: 9x256*4B
//  Out: pFeatPoints            - [out] Feature Points: [1xNx2] * 2Byte
//       pFeatValues            - [out] Feature Values: [1xN] * 2Byte
//       pWdrThumbWgtTable      - [out] Thumb Weight Table: 9x256*2B
// 
// Date: Revised by yousf 20160824
// 
/*************************************************************************/
int FeatureDetect(RK_U16* pThumbDspChunk, int nWid, int nHgt, int nStride, int rowSeg, int numFeature, 
    RK_U16* pThumbFilterDspChunk, RK_U32* pWdrWeightMat, RK_U16* pFeatPoints[], RK_U16* pFeatValues, RK_U16* pWdrThumbWgtTable)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureDetect()\n");
// #endif    
    //
    int Grdx, Grdy, Grad;
    int maxGrad, maxGrdRow, maxGrdCol;

    int wid = nStride/2;

    // 
    RK_U16* pTmp = NULL;
    int     nThumbFeatWinSize = DIV_FIXED_WIN_SIZE;
    for (int m=0; m < numFeature; m++)
    {
        // init
        maxGrad   = 0; // Max
        maxGrdRow = 0; // row
        maxGrdCol = 0; // col

        // 
        pTmp = pThumbDspChunk + 2*wid + 1 + m * nThumbFeatWinSize + 1; // top-1,left-1, block(1,1)
        for (int i=1; i<nThumbFeatWinSize-1; i++)
        {
            for (int j=1; j <nThumbFeatWinSize-1; j++)
            {
                // grad
//                 Grdx = *(pThumbDspChunk +   i   * wid + j-1 + m * nThumbFeatWinSize) 
//                      - *(pThumbDspChunk +   i   * wid + j+1 + m * nThumbFeatWinSize);
//                 Grdy = *(pThumbDspChunk + (i-1) * wid + j   + m * nThumbFeatWinSize) 
//                      - *(pThumbDspChunk + (i+1) * wid + j   + m * nThumbFeatWinSize);
                Grdx = *(pTmp-1) - *(pTmp+1);
                Grdy = *(pTmp-nWid) - *(pTmp+nWid);

                // next col
                pTmp++; 

                // sum grad
                Grdx = ABS_U16(Grdx);
                Grdy = ABS_U16(Grdy);
                Grad = Grdx + Grdy; // overflow ?

#if USE_MAX_GRAD == 1
                Grad = MIN(Grad, MAX_GRAD); // truncate 12bit: // max grad 0xFFF= 2^12-1
#endif

                // maxGrad
                if (Grad > maxGrad)
                {
                    maxGrad   = Grad;
                    maxGrdRow = rowSeg * nThumbFeatWinSize + i;
                    maxGrdCol = m * nThumbFeatWinSize + j;
                }

            } // for j

            // next row
            pTmp += (nWid - nThumbFeatWinSize + 2); 

        } // for i

        // Feature
        pFeatPoints[0][rowSeg * numFeature + m] = maxGrdRow;
        pFeatPoints[1][rowSeg * numFeature + m] = maxGrdCol;
        pFeatValues[rowSeg * numFeature + m]    = maxGrad;
    }

    //
    return ret;

#else


#endif
} // FeatureDetect()


/************************************************************************/
// Func: FeatureFilter()
// Desc: Feature Filter
//   In: pFeatPoints        - [in/out] Feature Points: [1xNx2] * 2Byte
//       pFeatValues        - [in/out] Feature Values: [1xN] * 2Byte
//       numFeature         - [in] num Feature in input Thumb data 
//       nThumbWid          - [in] Thumb data width
//       nThumbHgt          - [in] Thumb data height
//  Out: numValidFeature    - [out] num of Valid Feature
// 
// Date: Revised by yousf 2016804
// 
/*************************************************************************/
int FeatureFilter(RK_U16* pFeatPoints[], RK_U16* pFeatValues, int numFeature, int nThumbWid, int nThumbHgt, int& numValidFeature)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureFilter()\n");
// #endif      
#if FEATURE_TH_METHOD == USE_MIN2_TH
    // sharpness threshold
    RK_U16      SharpTh; 
    // first min & second min
    RK_U16      FirstMinSegSharp    = 0xFFFF;   // init First Min Segment Sharpness
    RK_U16      SecondMinSegSharp   = 0xFFFF;   // init Second Min Segment Sharpness
    RK_U16      fstMinRow           = 0;        // row
    RK_U16      fstMinCol           = 0;        // col
    RK_U16      sndMinRow           = 0;        // row
    RK_U16      sndMinCol           = 0;        // col
    for (int n=0; n < numFeature; n++)
    {
        if (FirstMinSegSharp >= pFeatValues[n])
        {
            // second min
            sndMinRow         = fstMinRow;
            sndMinCol         = fstMinCol;
            SecondMinSegSharp = FirstMinSegSharp;
            // first min
            fstMinRow         = pFeatPoints[0][n];  // maxGrdRow
            fstMinCol         = pFeatPoints[1][n];  // maxGrdCol
            FirstMinSegSharp  = pFeatValues[n];     // maxGrad
        }
        else if(SecondMinSegSharp >= pFeatValues[n])
        {
            // second min 
            sndMinRow         = pFeatPoints[0][n];  // maxGrdRow
            sndMinCol         = pFeatPoints[1][n];  // maxGrdCol
            SecondMinSegSharp = pFeatValues[n];     // maxGrad 
        }
    } // for n
    // first min + second min
    SharpTh = SecondMinSegSharp + FirstMinSegSharp; // sharpness threshold

#elif FEATURE_TH_METHOD == USE_AVE_TH
    // sharpness threshold
    RK_U16      SharpTh; 
    int         sumGrd = 0;
    for (int n=0; n < numFeature; n++)
    {
        sumGrd += pFeatValues[n];
    }
    // ave
    SharpTh = sumGrd / numFeature;

#endif
    //SharpTh = 0; // 0823 test

    // Valid Features
    numValidFeature = 0;            // num of Valid Feature
    for (int n=0; n < numFeature; n++)
    {
        if (pFeatValues[n] >= SharpTh) // Segment
        {
            if (   pFeatPoints[0][n] < COARSE_MATCH_WIN_SIZE
                || pFeatPoints[1][n] < COARSE_MATCH_WIN_SIZE 
                || pFeatPoints[0][n] >= nThumbHgt - COARSE_MATCH_WIN_SIZE 
                || pFeatPoints[1][n] >= nThumbWid - COARSE_MATCH_WIN_SIZE)
            { 
                // Invalid Feature
            }
            else
            {
                // Valid Feature
                pFeatPoints[0][numValidFeature] = pFeatPoints[0][n];    // maxGrdRow
                pFeatPoints[1][numValidFeature] = pFeatPoints[1][n];    // maxGrdCol
                pFeatValues[numValidFeature]    = pFeatValues[n];       // maxGrad 
                numValidFeature++;
            }
        }        
    }
    // Invalid Feature
    pFeatPoints[0][numValidFeature] = 0;  // maxGrdRow
    pFeatPoints[1][numValidFeature] = 0;  // maxGrdCol
    pFeatValues[numValidFeature]    = 0;  // maxGrad 

    //
    return ret;

#else


#endif
} // FeatureFilter()


/************************************************************************/
// Func: FeatureCoarseMatching()
// Desc: Feature Coarse Matching
//   In: pThumbBase         - [in] ThumbBase data pointer
//       hgt0               - [in] ThumbBase data height
//       wid0               - [in] ThumbBase data width
//       pThumbRef          - [in] ThumbRef data pointer
//       hgt1               - [in] ThumbRef data height
//       wid1               - [in] ThumbRef data width
//  Out: row                - [out] Match Result Row
//       col                - [out] Match Result Col
//       cost               - [out] Match Result Cost
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int FeatureCoarseMatching(
    RK_U16* pThumbBase, RK_U16 hgt0, RK_U16 wid0, 
    RK_U16* pThumbRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16& row, RK_U16& col, RK_U16& cost)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureCoarseMatching()\n");
// #endif  
    // init vars
    RK_U16*         pTmpBase        = NULL;             // temp pointer
    RK_U16*         pTmpRef         = NULL;             // temp pointer
    RK_U32          minSAD;
    RK_U32          curSAD;

    // init min SAD
    minSAD = 0xFFFFFFFF; // 2^32 - 1

    //
    if (hgt0 != COARSE_MATCH_WIN_SIZE || wid0 != COARSE_MATCH_WIN_SIZE)
    {
        ret = -1;
        return ret;
    }


    // Matching
    for (int i=0; i < hgt1 - hgt0 + 1; i++)
    {
        for (int j=0; j < wid1 - wid0; j++)
        {
            pTmpBase = pThumbBase;                  // Base data
            pTmpRef  = pThumbRef + i*wid1+ j;    // Ref data
            curSAD   = 0;
            for (int m=0; m < hgt0; m++)
            {
                for (int n=0; n < wid0; n++)
                {
                    //curSAD += ABS_U16(*pTmpBase - *pTmpRef);
                    curSAD += MIN(ABS_U16(*pTmpBase - *pTmpRef), 0xFF); // truncate 8bit: // 16x16 * 8bitSAD -> 16bit
                    pTmpBase++;
                    pTmpRef++;
                }
                pTmpRef += (wid1 - wid0);
            }
            if (curSAD < minSAD)
            {
                minSAD = curSAD;
                row    = i;
                col    = j;
            }
        }
    }

    // Matching Min SAD
    cost = minSAD;


    //
    return ret;
#else


#endif
} // FeatureCoarseMatching()


/************************************************************************/
// Func: Scaler_Raw2Luma()
// Desc: Scaler Raw to Luma
//   In: pRawData       - [in] Raw data pointer
//       nRawWid        - [in] Raw data width
//       nRawHgt        - [in] Raw data height
//       nThumbWid      - [in] Thumb data width
//       nThumbHgt      - [out] Thumb data height
//  Out: pThumbData     - [out] Thumb data pointer
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int Scaler_Raw2Luma(RK_U16* pRawData, int nRawWid, int nRawHgt, int nLumaWid, int nLumaHgt, RK_U16* pLumaData)
{
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("Scaler_Raw2Luma()\n");
// #endif   
    // init vars
    RK_U16*         pTmp0           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp1           = NULL;             // temp pointer for Raw data
    RK_U16*         pTmp2           = NULL;             // temp pointer for Luma data
    RK_U16          LumaValue       = 0;                // Luma Value
    int             nScaleRaw2Luma = 2;
    //
    if (nRawHgt/nScaleRaw2Luma != nLumaHgt)
    {
        ret = -1;
        return ret;
    }

    // Raw to Luma -- 1/(2x2)
    pTmp2 = pLumaData;
    for (int i=0; i < nLumaHgt; i++)
    {
        for (int j=0; j <nLumaWid; j++)
        {
            // 2x2 Rect left-top
            pTmp0 = pRawData + (i * nRawWid + j) * nScaleRaw2Luma;

            // average(2x2Raw) -> LumaValue
            LumaValue = 0;
            for (int m=0; m<nScaleRaw2Luma; m++)
            {
                // m-th line in 2x2 Rect
                pTmp1 = pTmp0 + m * nRawWid;
                for (int n=0; n<nScaleRaw2Luma; n++)
                {
                    LumaValue += *(pTmp1++);
                }
            }
            *pTmp2 = LumaValue;
            pTmp2++;
        }
    }

    //
    return ret;

} // Scaler_Raw2Luma()


/************************************************************************/
// Func: FeatureFineMatching()
// Desc: Feature Fine Matching
//   In: pLumaBase          - [in] LumaBase data pointer
//       hgt0               - [in] LumaBase data height
//       wid0               - [in] LumaBase data width
//       pLumaRef           - [in] LumaRef data pointer
//       hgt1               - [in] LumaRef data height
//       wid1               - [in] LumaRef data width
//       col_st             - [in] LumaRef Block Col Start
//       wid_ref            - [in] LumaRef Block Width
//  Out: row                - [out] Match Result Row
//       col                - [out] Match Result Col
//       cost               - [out] Match Result Cost
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int FeatureFineMatching(
    RK_U16* pLumaBase, RK_U16 hgt0, RK_U16 wid0, 
    RK_U16* pLumaRef, RK_U16 hgt1, RK_U16 wid1, 
    RK_U16 col_st, RK_U16 wid_ref, 
    RK_U16& row, RK_U16& col, RK_U16& cost)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("FeatureFineMatching()\n");
// #endif   
    // init vars
    RK_U16*         pTmpBase        = NULL;             // temp pointer
    RK_U16*         pTmpRef         = NULL;             // temp pointer
    RK_U32          minSAD;
    RK_U32          curSAD;

    // init min SAD
    minSAD = 0xFFFFFFFF; // 2^32 - 1

    //
    if (hgt0 != FINE_MATCH_WIN_SIZE/2 || wid0 != FINE_MATCH_WIN_SIZE/2)
    {
        ret = -1;
        return ret;
    }


    // Matching
    int cnt=0;
    for (int i=0; i < hgt1 - hgt0 + 1; i++)
    {
        //for (int j=0; j < wid1 - wid0; j++)
        //for (int j=col_st; j < MIN(col_st + 2*FINE_LUMA_RADIUS+1, wid1-FINE_MATCH_WIN_SIZE/2+1); j++)
        for (int j=col_st; j < col_st + wid_ref - wid0 + 1; j++)
        {
            cnt++;
            pTmpBase = pLumaBase;               // Base data
            pTmpRef  = pLumaRef + i*wid1+ j;    // Ref data
            curSAD   = 0;
            for (int m=0; m < hgt0; m++)
            {
                for (int n=0; n < wid0; n++)
                {
                    //curSAD += ABS_U16(*pTmpBase - *pTmpRef);
                    curSAD += MIN(ABS_U16(*pTmpBase - *pTmpRef), 0x3F); // truncate 6bit: // 32x32 * 6bitSAD -> 16bit
                    pTmpBase++;
                    pTmpRef++;
                }
                pTmpRef += (wid1 - wid0);
            }
            if (curSAD < minSAD)
            {
                minSAD = curSAD;
                row    = i;
                col    = j;
            }
        }
    }

    // Matching Min SAD
    cost = minSAD;

    //
    return ret;
#else


#endif
} // FeatureFineMatching()


/************************************************************************/
// Func: MvHistFilter()
// Desc: MV Hist Filter
//   In: pMatchPtsY         - [in] Matching Points Y
//       pMatchPtsX         - [in] Matching Points X
//       numValidFeature    - [in] num of Valid Feature
//       pRowMvHist         - [in] RowMV Hist
//       pColMvHist         - [in] ColMV Hist
//  Out: pMarkMatchFeature  - [out] Marks of Match Feature in BaseFrame & RefFrame#k
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int MvHistFilter(RK_U16* pMatchPtsY[], RK_U16* pMatchPtsX[], int numValidFeature,
    RK_U8* pRowMvHist, RK_U8* pColMvHist, int nBasePicNum, int nRefPicNum,
    RK_U8* pMarkMatchFeature)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("MvHistFilter()\n");
// #endif   
#if USE_MV_HIST_FILTRATE == 1
    //-- Filtrate: Match Feature in BaseFrame & RefFrame#k
    int     MVy, MVx;
    int     minNumValidFeat = MAX((int)(numValidFeature * VALID_FEATURE_RATIO), 1);

    // MV Hist
    memset(pRowMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
    memset(pColMvHist, 0, sizeof(RK_U8) * LEN_MV_HIST);
    for (int n=0; n < numValidFeature; n++)
    {
        // MVy
        MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
        MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];
        pRowMvHist[HALF_LEN_MV_HIST + MVy] += 1; // hist +1
        pColMvHist[HALF_LEN_MV_HIST + MVx] += 1; // hist +1
    }

    // MV Hist --> HistMark(0 or 1)
    for (int h=0; h < LEN_MV_HIST; h++)
    {
        // MVy
        if (pRowMvHist[h] > minNumValidFeat)
        {
            pRowMvHist[h] = 1;  // Valid MVy
        }
        else
        {
            pRowMvHist[h] = 0;  // Invalid MVy
        }
        // MVx
        if (pColMvHist[h] > minNumValidFeat)
        {
            pColMvHist[h] = 1;  // Valid MVx
        }
        else
        {
            pColMvHist[h] = 0;  // Invalid MVx
        }
    }
    // HistMark(0 or 1) --> pMarkMatchFeature
    memset(pMarkMatchFeature, 0, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
    if (numValidFeature > MAX_NUM_MATCH_FEATURE)
    {
        ret = -1;
        printf("Param Setting Error: MAX_NUM_MATCH_FEATURE is too small !\n");
        return ret;
    }
    for (int n=0; n < numValidFeature; n++)
    {
        // MVy
        MVy = pMatchPtsY[nBasePicNum][n] - pMatchPtsY[nRefPicNum][n];
        MVx = pMatchPtsX[nBasePicNum][n] - pMatchPtsX[nRefPicNum][n];

        // Valid Feature
        if (pRowMvHist[HALF_LEN_MV_HIST + MVy] == 1 && pColMvHist[HALF_LEN_MV_HIST + MVx] == 1)
        {
            pMarkMatchFeature[n] = 1; // Valid Feature
        }
    }
#else
    memset(pMarkMatchFeature, 1, sizeof(RK_U8) * MAX_NUM_MATCH_FEATURE);
#endif

    //
    return ret;
#else


#endif
} // MvHistFilter()

/************************************************************************/
// Func: GetRegion4Points()
// Desc: Get 4 points in 4 Regions
//   In: pAgents_Marks      - [in] Agents in 4x4 Region [RegMark4x4] * 16
//       pAgents_PtYs       - [in] Agents in 4x4 Region [Y] * RawFileNum * 16
//       pAgents_PtXs       - [in] Agents in 4x4 Region [X] * RawFileNum * 16
//       pTable             - Region4 Index Table
//       idx                - Region4 Index Table idx
//       numBase            - number of Base Frame
//       numRef             - number of Ref Frame
//  Out: pPoints4           - 4 points in 4 Regions
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int GetRegion4Points(RK_U8* pAgents_Marks, RK_U16* pAgents_PtYs[], RK_U16* pAgents_PtXs[],
    RK_U8* pTable, int idx, int numBase, int numRef, RK_U16* pPoints4)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("GetRegion4Points()\n");
// #endif   
    // Region4 Index Table[idx]
    RK_U8* pTableItem = NULL;
    pTableItem = pTable + idx * 4; 

    // Get 4 points in 4 Regions
    for (int i=0; i < 4; i++)
    {
        if (pAgents_Marks[pTableItem[i]] == MARK_EXIST_AGENT)     // mark
        {
            // BaseFrame Point
            *(pPoints4 + i*4 + 0) = pAgents_PtYs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 0);
            *(pPoints4 + i*4 + 1) = pAgents_PtXs[numBase][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numBase * 2 + 1);
            // RefFrame Matching Point
            *(pPoints4 + i*4 + 2) = pAgents_PtYs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 0);
            *(pPoints4 + i*4 + 3) = pAgents_PtXs[numRef][pTableItem[i]]; // *(pAgents + *(pTableItem+i) * wid + 2 + numRef * 2 + 1);
        }
        else
        {
            ret = -1;
            return ret;
        }
    }

    //
    return ret;
#else


#endif
} // GetRegion4Points()


/************************************************************************/
// Func: classMFNR::CreateCoefficient()
// Desc: Create Coefficient MatrixA & VectorB
//   In: pPoints4       - [in] 4 points in 4 Regions
//  Out: pMatA          - [out] Coefficient MatrixA for A*X = B
//       pVecB          - [out] Coefficient VectorB for A*X = B
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int CreateCoefficient(RK_U16* pPoints4, RK_F32* pMatA, RK_F32* pVecB)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("CreateCoefficient()\n");
// #endif  
    // pPoints4: Base & Ref
    /*
    RK_U16 x00, y00, x01, y01; // 0-pair Matching Points
    RK_U16 x10, y10, x11, y11; // 1-pair Matching Points
    RK_U16 x20, y20, x21, y21; // 2-pair Matching Points
    RK_U16 x30, y30, x31, y31; // 3-pair Matching Points
    */

    //              MatrixA                    VectorB
    /*
    x00 y00 1  0   0   0  -x00*x01 - y00*y01        x01    
    0   0   0  x00 y00 1  -x00*y01 - y00*y01        x01 
    ......
    */

    // Create Coefficient MatrixA & VectorB
    for (int i=0; i < 8; i++)
    {
        if (i<4)
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = *(pPoints4 + i*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 1) = *(pPoints4 + i*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 2) = 1;
            *(pMatA + i * 8 + 3) = 0;
            *(pMatA + i * 8 + 4) = 0;
            *(pMatA + i * 8 + 5) = 0;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + i*4 + 0) * *(pPoints4 + i*4 + 2)); // p[i,0] * p[i,2]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + i*4 + 1) * *(pPoints4 + i*4 + 2)); // p[i,1] * p[i,2]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + i*4 + 2); // p[i,2] = x[i,1]
        }
        else
        {
            //-- MatrixA
            *(pMatA + i * 8 + 0) = 0;
            *(pMatA + i * 8 + 1) = 0;
            *(pMatA + i * 8 + 2) = 0;
            *(pMatA + i * 8 + 3) = *(pPoints4 + (i-4)*4 + 0); // p[i,0]
            *(pMatA + i * 8 + 4) = *(pPoints4 + (i-4)*4 + 1); // p[i,1]
            *(pMatA + i * 8 + 5) = 1;
            *(pMatA + i * 8 + 6) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 0) * *(pPoints4 + (i-4)*4 + 3)); // p[i,0] * p[i,3]
            *(pMatA + i * 8 + 7) = (RK_F32)(-1 * *(pPoints4 + (i-4)*4 + 1) * *(pPoints4 + (i-4)*4 + 3)); // p[i,1] * p[i,3]
            //-- VectorB
            *(pVecB + i)         =  *(pPoints4 + (i-4)*4 + 3); // p[i,3] = y[i,1]
        }
    }
    
    //
    return ret;
#else


#endif
} // CreateCoefficient()


/************************************************************************/
// Func: GetPerspectMatrix()
// Desc: Get a PersPective Matrix
//   In: pMatA          - [in] Coefficient MatrixA for A*X = B
//       pVecB          - [in] Coefficient VectorB for A*X = B
//  Out: pVecX          - [out] Coefficient VectorX for A*X = B
// 
// Date: Revised by yousf 20160804
// 
/*************************************************************************/
int ComputePerspectMatrix(RK_F32* pMatA, RK_F32* pVecB, RK_F32* pVecX)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("ComputePerspectMatrix()\n");
// #endif  
    // init vars
    int     i, j, k;
    float   s, t;
    int     n = 8;

    // Gaussian Elimination with Complete Pivoting    st. Ax=b -> Ux=b'
    for (k = 0; k < n; k++)
    {
        // Select Pivot Element: max_i(a[i,j])  
        j = k;  
        t = FABS(pMatA[k*n + k]);
        for (i = k + 1; i<n; i++)
        {
            if ((s = FABS(pMatA[i*n + k])) > t)
            {
                t = s; // max_i{pMatA(i,k)}
                j = i; // argmax_i{pMatA(i,k)}
            }
        }

        //  Ill-conditioned Equation
        if (t < 1.0e-30) 
        {
            ret = -1;
            return ret;  
        }

        // Exchange: pMatA(j,:) - pMatA(k,:)
        if (j != k)
        {
            for (i = k; i < n; i++)
            {
                t              = pMatA[j*n + i];
                pMatA[j*n + i] = pMatA[k*n + i];
                pMatA[k*n + i] = t;
            }
            t        = pVecB[j];
            pVecB[j] = pVecB[k];
            pVecB[k] = t;
        }

        // Recalculation pMatA(k,k:n-1): <- pMatA(k,k) = 1
        t = (RK_F32)(1.0 / pMatA[k*n + k]); 
        //for (i = k + 1; i < n; i++) 
        for (i = k ; i < n; i++) 
        {
            pMatA[k*n + i] *= t;
        }
        pVecB[k] *= t;

        // Elimination: pMatA(k+1:n-1,:)
        for (i = k + 1; i < n; i++)
        {
            t = pMatA[i*n + k];
            //for (j = k + 1; j < n; j++) 
            for (j = k; j < n; j++) 
            {
                pMatA[i*n + j] -= pMatA[k*n + j] * t;
            }
            pVecB[i] -= pVecB[k] * t;
        }
    }

    // U * X = b'  -> X 
    pVecX[n-1] = pVecB[n-1];
    for (i = n - 2; i >= 0; i--)
    {
        for (j = i + 1; j < n; j++) 
        {
            pVecB[i] -= pMatA[i*n + j] * pVecB[j];
        }
        pVecX[i] = pVecB[i];
    }

    //
    return ret;
#else


#endif
} // GetPerspectMatrix()


/************************************************************************/
// Func: PerspectProject()
// Desc: Perspective Project: pVecX * pBasePoint = pProjPoint
//   In: pVecX          - [in] Coefficient VectorX for A*X = B
//       pBasePoint     - [in] PointB(r,c) in BaseFrame
//  Out: pProjPoint     - [out] Perspective Projected PointP(r,c) in RefFrame
// 
// Date: Revised by yousf 20160808
// 
/*************************************************************************/
int PerspectProject(RK_F32* pVecX, RK_F32* pBasePoint, RK_F32* pProjPoint)
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value
// #if MY_DEBUG_PRINTF == 1
//     printf("PerspectProject()\n");
// #endif   
    // Perspective Projected Coordinate
    RK_F32          X;
    RK_F32          Y;
    RK_F32          Z;
    // Perspective Projected
    //        A(0,0) * Bx                        A(0,1) * By                   A(0,2) * 1
    X = *(pVecX + 0) * *(pBasePoint + 0) + *(pVecX + 1) * *(pBasePoint + 1) + *(pVecX + 2);
    //        A(1,0) * Bx                        A(1,1) * By                   A(1,2) * 1
    Y = *(pVecX + 3) * *(pBasePoint + 0) + *(pVecX + 4) * *(pBasePoint + 1) + *(pVecX + 5);
    //        A(2,0) * Bx                        A(2,1) * By                   A(2,2) * 1
    Z = *(pVecX + 6) * *(pBasePoint + 0) + *(pVecX + 7) * *(pBasePoint + 1) + *(pVecX + 8);

    // Normalization
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
    *(pProjPoint + 0) = (RK_F32)ROUND_I32(X / Z);
    *(pProjPoint + 1) = (RK_F32)ROUND_I32(Y / Z);
#else
    *(pProjPoint + 0) = X / Z; // RK_F32
    *(pProjPoint + 1) = Y / Z; // RK_F32
#endif

    //
    return ret;
#else


#endif
} // PerspectProject()


/************************************************************************/
// Func: ComputeHomographyError()
// Desc: Compute Homography's Error
//   In: 
//  Out: 
// 
// Date: Revised by yousf 20160818
// 
/*************************************************************************/
int ComputeHomographyError( 
    int     type,        // [in] type=0(AgentsFeatures), type=1(AllFeatures)
    RK_U8*  pMarks,      // [in] Agents'/Features' Marks
    int     numFeature,  // [in] Num of Agents/Features
    RK_U16* pPointYs[],  // [in] Agents'/Features' Rows
    RK_U16* pPointXs[],  // [in] Agents'/Features' Cols
    int     nBasePicNum, // [in] Base #0
    int     nRefPicNum,  // [in] Ref #k
    RK_F32* pVectorX,    // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pBasePoint,  // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pProjPoint,  // [in] pVecX * pBasePoint = pProjectPoint
    RK_F32* pRefPoint,   // [in] abs(pRefPoint - pProjPoint)
    RK_U32& error)       // [out] Correct Project Count / Sum Project Errors
{
#ifndef CEVA_CHIP_CODE
    //
    int     ret = 0; // return value

    // type = 0
    RK_U32      corrCnt = 0;                // Correct Project Count <-- Error Threshold of Valid Homography
#if USE_EARLY_STOP_H == 1 // 1-use Early Stop Compute Homography
    int         goodCnt;                    // Correct Project Count <-- Error Threshold of Good Homography
#endif

    // type = 1
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
    RK_S32      errRow, errCol;             // Project Errors: row error & col error
    RK_U32      errSum_H = 0;               // Sum Project Errors: sum error of all features
    //RK_U32      errMin_H;                   // Min Project Errors: min error of best Homography for BaseFrame--RefFrame#k
#else
    RK_F32      errRow, errCol;             // Project Errors: row error & col error
    RK_U32      errSum_H = 0;               // Sum Project Errors: sum error of all features
    //RK_U32      errMin_H;                   // Min Project Errors: min error of best Homography for BaseFrame--RefFrame#k
#endif

    //
    for (int n=0; n < numFeature; n++)
    {
        if (pMarks[n] == 1)
        {
            // Perspective Project: pVecX * pBasePoint = pProjectPoint
            pBasePoint[0] = pPointYs[nBasePicNum][n];
            pBasePoint[1] = pPointXs[nBasePicNum][n];
            pRefPoint[0]  = pPointYs[nRefPicNum][n];
            pRefPoint[1]  = pPointXs[nRefPicNum][n];

            ret = PerspectProject(pVectorX, pBasePoint, pProjPoint);
            if (ret)
            {
                printf("Failed to PerspectProject !\n");
                //return ret;
                continue;
            }

            // Project Errors
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
            errRow = ABS_U16(pRefPoint[0] - pProjPoint[0]);
            errCol = ABS_U16(pRefPoint[1] - pProjPoint[1]);   
#else
            errRow = FABS(pRefPoint[0] - pProjPoint[0]);
            errCol = FABS(pRefPoint[1] - pProjPoint[1]); 
#endif


            //////////////////////////////////////////////////////////////////////////
            if (type==0)
            {
                // Error Threshold of Valid Homography
                if (errRow <= ERR_TH_VALID_H && errCol <= ERR_TH_VALID_H)
                {
                    corrCnt++; // Correct Project Count <-- Error Threshold of Valid Homography
                }

#if USE_EARLY_STOP_H == 1 // 1-use Early Stop Compute Homography
                // Error Threshold of Good Homography
                if (errRow <= ERR_TH_GOOD_H && errCol <= ERR_TH_GOOD_H)
                {
                    goodCnt++; // Correct Project Count <-- Error Threshold of Good Homography
                }
#endif
                
            }
            else // type = 1
            {
#if USE_FLOAT_ERROR == 0 // 0-RK_U16, 1-RK_F32
                errRow = MIN(errRow, MAX_PROJECT_ERROR);
                errCol = MIN(errCol, MAX_PROJECT_ERROR);
#else
                errRow = (RK_F32)ROUND_U32(MIN(errRow*1000000, 0xFFFFFFFF));
                errCol = (RK_F32)ROUND_U32(MIN(errCol*1000000, 0xFFFFFFFF));
#endif
                // Sum Project Errors: sum error of all features
                errSum_H += (RK_U32)(errRow + errCol);
            }
            
           
        }
    } // for n

    if (type==0)
    {
        // output
        error = corrCnt;    // Correct Project Count
    }
    else
    {
        // output
        error = errSum_H;    // Sum Project Errors
    }

    //
    return ret;
#else


#endif
} // ComputeHomographyError()


//////////////////////////////////////////////////////////////////////////

#include "rk_bayerwdr.h"
void GetWdrWeightTable(RK_U16* pThumbData, // <<! [in]
                            int 	nWid,           // <<! [in]    
                            int 	nHgt,           // <<! [in]
                            int 	nStride,        // <<! [in]
                            int 	rowSeg,         // <<! [in]     
                            RK_U16* pThumbFilterDspChunk, // <<! [in]
                            RK_U32* pWdrWeightMat,        // <<! [in]  
                            RK_U16* pWdrThumbWgtTable,
                            int 	statisticWidth,
                            int 	statisticHeight)   // <<! [out]  
{
	// add by zxy @ 2016 08 25 

	wdrPreFilterBlock( pThumbData, pThumbFilterDspChunk, nHgt, nWid );
	//hist
	CalcuHist( pThumbFilterDspChunk, pWdrThumbWgtTable, pWdrWeightMat, nHgt, nWid, statisticWidth, rowSeg );

}

