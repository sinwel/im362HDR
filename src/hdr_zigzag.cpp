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
#define     CODE_SCATTER 		0

void zigzagDebayer(	uint16_t *p_u16Src, 
						uint16_t *p_u16Tab, 
						uint16_t blockW,
						uint16_t blockH,
						uint16_t stride,
						uint16_t normValue,
						uint16_t *buff,		//<<! [out] 32x64 short16  long and short
						uint16_t *scale) 	//<<! [out] 32x64 short16  table
						
{
#ifdef __XM4__
	PROFILER_START(HDR_BLOCK_H, HDR_BLOCK_W);
#endif
	 uint8_t log2_expTimes = 3;// log2(8)	
	uint16_t i,j,SecMask,blacklevel=64;
	ushort16 vG0,vR0,vB1,vG1,vG2,vR2,vB3,vG3,vG4,vR4,vB5,vG5,vG6,vR6,vB7,vG7;
	ushort16 vGR0,vBG1,vGR2,vBG3,vGR4,vBG5,vGR6,vBG7;
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13;
	uchar16  vt0,vt1,vt2,vt3,vt4,vt5,vt6,vt7;
	ushort16 vBayerL0Seg0,vBayerL1Seg0,vBayerL2Seg0,vBayerL3Seg0;
	ushort16 vBayerL0Seg1,vBayerL1Seg1,vBayerL2Seg1,vBayerL3Seg1;	
	
	ushort16 vR0offset,vR2offset,vR2offset_,vR4offset,vR4offset_,vR6offset;
	ushort16 vB1offset,vB3offset,vB3offset_,vB5offset,vB5offset_,vB7offset;
	ushort16 vG2offset,vG2offset_,vG4offset,vG4offset_;
	ushort16 vG1offset,vG3offset,vG3offset_;
	ushort16 vG6offset,vG6offset_,vG5offset,vG5offset_;	
	
	ushort16 vRL0best,vBL0best,vG0L0best,vG1L0best;
	ushort16 vRL1best,vBL1best,vG0L1best,vG1L1best;
	
	ushort16 vG2packed,vR2packed,vB3packed,vG3packed;
	ushort16 vG4packed,vR4packed,vB5packed,vG5packed;

	ushort16 vRL0Long,vRL0Short,vBL0Long,vBL0Short;
	ushort16 vG0L0Long,vG0L0Short,vG1L0Long,vG1L0Short;

	ushort16 vRL1Long,vRL1Short,vBL1Long,vBL1Short;
	ushort16 vG0L1Long,vG0L1Short,vG1L1Long,vG1L1Short;

	ushort16 vRL0diff,vBL0diff,vG0L0diff,vG1L0diff;
	ushort16 vRL1diff,vBL1diff,vG0L1diff,vG1L1diff;	

	uint16_t* pL0G0		= buff 									;                              
	uint16_t* pL0Red 	= buff+1									;                              
	uint16_t* pL0Blu	= buff	 + HDR_BLOCK_W 	;                              
	uint16_t* pL0G1		= buff+1 + HDR_BLOCK_W 	;                              
	uint16_t* pL1G0		= buff 	 + 2*HDR_BLOCK_W ;                              
	uint16_t* pL1Red 	= buff+1 + 2*HDR_BLOCK_W ;                              
	uint16_t* pL1Blu 	= buff   + 3*HDR_BLOCK_W ;                              
	uint16_t* pL1G1		= buff+1 + 3*HDR_BLOCK_W ;                              
                                                                                 
	uint16_t* pL0G0_s	= buff 	 + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL0Red_s 	= buff+1 + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL0Blu_s 	= buff	 + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL0G1_s	= buff+1 + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL1G0_s	= buff   + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL1Red_s 	= buff+1 + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL1Blu_s 	= buff   + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	uint16_t* pL1G1_s	= buff+1 + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   

	uint16_t* pScaleL0G0	= scale 									;                              
	uint16_t* pScaleL0Red  	= scale+1									;                              
	uint16_t* pScaleL0Blu	= scale	  + HDR_FILTER_W 	;                              
	uint16_t* pScaleL0G1	= scale+1 + HDR_FILTER_W 	;                              
	uint16_t* pScaleL1G0	= scale   + 2*HDR_FILTER_W ;                              
	uint16_t* pScaleL1Red  	= scale+1 + 2*HDR_FILTER_W ;                              
	uint16_t* pScaleL1Blu  	= scale   + 3*HDR_FILTER_W ;                              
	uint16_t* pScaleL1G1	= scale+1 + 3*HDR_FILTER_W ;                              
      

	uint16_t OffsetGap2[16]  = {0, 2, 4, 6, 8,10,12,14, 16,18,20,22,24,26,28,30};
	short16 vOffsetGap2		 = *(short16*)(&OffsetGap2);	

	uint16_t OffsetGap4[16]  = {0, 4, 8, 12, 16,20,24,28, 32,36,40, 44, 48,52,56,60};
	short16 vOffsetGap4		 = *(short16*)(&OffsetGap4);	

	
	uint16_t* pInLine0 = p_u16Src ;				// + 3,Load R 
	uint16_t* pInLine1 = p_u16Src + stride 	;	// + 1 load G & B
	uint16_t* pInLine2 = p_u16Src + 2*stride 	;	// + 1 LOAD R & G
	uint16_t* pInLine3 = p_u16Src + 3*stride;		//     load B & G
	uint16_t* pInLine4 = p_u16Src + 4*stride	;	// + 2 LOAD G & R
	uint16_t* pInLine5 = p_u16Src + 5*stride	;	// + 2 LOAD B 
	uint16_t* pInLine6 = p_u16Src + 6*stride	;	// + 2 LOAD B 
	uint16_t* pInLine7 = p_u16Src + 7*stride	;	// + 2 LOAD B 
	// g r
	// b g
	// perm cfg:
	// g0 is same with b
	// g1 is same with r
	uint8_t cfg_adj_red1[32] 		= { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_red2[32] 		= { 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,16+3,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_red3[32] 		= { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
	 									0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uchar32 vcfgAdjRed1				= *(uchar32*)(&cfg_adj_red1);
	uchar32 vcfgAdjRed2				= *(uchar32*)(&cfg_adj_red2);
	uchar32 vcfgAdjRedPack			= *(uchar32*)(&cfg_adj_red3);

	
	uint8_t cfg_adj_blu1[32] 		= { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_blu2[32] 		= { 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjBlu1				= *(uchar32*)(&cfg_adj_blu1);
	uchar32 vcfgAdjBlu2				= *(uchar32*)(&cfg_adj_blu2);

		
	uint8_t cfg_adj_gre1[32]  		= { 1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
									0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_gre2[32]  		= { 2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
									0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjGre1				= *(uchar32*)(&cfg_adj_gre1);
	uchar32 vcfgAdjGre2				= *(uchar32*)(&cfg_adj_gre2);


	//  6 x 3 = 15 reg for one pixel interpolation.		
	for ( i = 0 ; i < blockW/32 ; i++ )
	{
		vldchk(pInLine0, vG0, vR0);   vGR0 = *(ushort16*)(pInLine0+32);   //pInLine0 += 8*stride;
		vldchk(pInLine1, vB1, vG1);   vBG1 = *(ushort16*)(pInLine1+32);   //pInLine1 += 8*stride;
		vldchk(pInLine2, vG2, vR2);   vGR2 = *(ushort16*)(pInLine2+32);   //pInLine2 += 8*stride;
		vldchk(pInLine3, vB3, vG3);   vBG3 = *(ushort16*)(pInLine3+32);   //pInLine3 += 8*stride;
		// subtract blacklevel
		vG0		= vsubsat(vG0, blacklevel);
		vR0		= vsubsat(vR0, blacklevel);
		vB1		= vsubsat(vB1, blacklevel);
		vG1		= vsubsat(vG1, blacklevel);

		vG2		= vsubsat(vG2, blacklevel);
		vR2		= vsubsat(vR2, blacklevel);
		vB3		= vsubsat(vB3, blacklevel);
		vG3		= vsubsat(vG3, blacklevel);

		vGR0	= vsubsat(vGR0, blacklevel);
		vBG1	= vsubsat(vBG1, blacklevel);
		vGR2	= vsubsat(vGR2, blacklevel);
		vBG3	= vsubsat(vBG3, blacklevel);


		vR0offset	= (ushort16)vperm(vR0,vGR0,vcfgAdjRed1);	
		vR2offset_	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed2);

		vB1offset	= (ushort16)vperm(vB1,vBG1,vcfgAdjBlu1);
		vB3offset_	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu2);

		vG2offset	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre1);
		vG2offset_	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre2);

		vG1offset	= (ushort16)vperm(vG1,vBG1,vcfgAdjRed1);
		vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);

		vR2offset	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed1);
		vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu1);

		vR2packed	= (ushort16)vperm(vR2,vGR2,vcfgAdjRedPack); 

		for ( j = 0 ; j < blockH/4; j++ )
		{
			vldchk(pInLine4, vG4, vR4);   vGR4 = *(ushort16*)(pInLine4+32);   pInLine4 += 4*stride;
			vldchk(pInLine5, vB5, vG5);   vBG5 = *(ushort16*)(pInLine5+32);   pInLine5 += 4*stride;
			vldchk(pInLine6, vG6, vR6);   vGR6 = *(ushort16*)(pInLine6+32);   pInLine6 += 4*stride;
			vldchk(pInLine7, vB7, vG7);   vBG7 = *(ushort16*)(pInLine7+32);   pInLine7 += 4*stride;

			vG4		= vsubsat(vG4, blacklevel);
			vR4		= vsubsat(vR4, blacklevel);
			vB5		= vsubsat(vB5, blacklevel);
			vG5		= vsubsat(vG5, blacklevel);
			
			vG6		= vsubsat(vG6, blacklevel);
			vR6		= vsubsat(vR6, blacklevel);
			vB7		= vsubsat(vB7, blacklevel);
			vG7		= vsubsat(vG7, blacklevel);

			vGR4	= vsubsat(vGR4, blacklevel);
			vBG5	= vsubsat(vBG5, blacklevel);
			vGR6	= vsubsat(vGR6, blacklevel);
			vBG7	= vsubsat(vBG7, blacklevel);
			
			// ========= line 0 GRBG ==============
			// -------------r-----------------
			// abs(R0-R4),abs(R2 - R2+4)
			// ----
			//vR0offset	= (ushort16)vperm(vR0,vGR0,vcfgAdjRed1);
			vR4offset	= (ushort16)vperm(vR4,vGR4,vcfgAdjRed1);
			//vR2offset_	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed2);
#if CODE_SCATTER
			v0			= vabssub(vR0offset, vR4offset);
			v1			= vabssub(vR2, vR2offset_);
			v2			= (ushort16)vadd(vR0offset, vR4offset);
			v3			= (ushort16)vadd(vR2, vR2offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vRL0best	= vselect(v2,v3,SecMask);
#else
			SecMask 	= vcmp(lt,vabssub(vR0offset, vR4offset),vabssub(vR2, vR2offset_));
			vRL0best 	= vselect( (ushort16)vadd(vR0offset, vR4offset), (ushort16)vadd(vR2, vR2offset_), SecMask); 
#endif

			//PRINT_CEVA_VRF("vRL0best", vRL0best, stderr);

			// -------------b-----------------
			// abs(B1-B5),abs(B3 - B3+4)
			// ----
			//vB1offset	= (ushort16)vperm(vB1,vBG1,vcfgAdjBlu1);
			vB5offset	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
			//vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu2);
#if CODE_SCATTER
			v0			= vabssub(vB1offset, vB5offset);
			v1			= vabssub(vB3, vB3offset_);
			v2			= (ushort16)vadd(vB1offset, vB5offset);
			v3			= (ushort16)vadd(vB3, vB3offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vBL0best	= vselect(v2,v3,SecMask);
#else
			
			SecMask 	= vcmp(lt,vabssub(vB1offset, vB5offset),vabssub(vB3, vB3offset_));
			vBL0best 	= vselect( (ushort16)vadd(vB1offset, vB5offset), (ushort16)vadd(vB3, vB3offset_), SecMask); 
#endif
			//PRINT_CEVA_VRF("vBL0best", vBL0best, stderr);
			// -------------G-----------------
			// abs(G1-G3),abs(G2 - G4)
			//  G1  G3 
			//vG1offset	= (ushort16)vperm(vG1,vBG1,vcfgAdjRed1);
			//vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);
#if CODE_SCATTER
			v0			= vabssub(vG1, vG3offset);
			v1			= vabssub(vG3, vG1offset);
			v2			= (ushort16)vadd(vG1, vG3offset);
			v3			= (ushort16)vadd(vG3, vG1offset); 
			SecMask 	= vcmp(lt,v0,v1);
			vG0L0best	= vselect(v2,v3,SecMask);
#else
			SecMask		= vcmp(lt,vabssub(vG1, vG3offset),vabssub(vG3, vG1offset));
			vG0L0best 	= vselect( (ushort16)vadd(vG1, vG3offset), (ushort16)vadd(vG3, vG1offset), SecMask); 
#endif
			//PRINT_CEVA_VRF("vG0L0best", vG0L0best, stderr);

			//  G2  G4  
			//vG2offset	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre1);
			vG4offset	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre1);
			//vG2offset_	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre2);
			vG4offset_	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre2);
#if CODE_SCATTER
			v0			= vabssub(vG2offset, vG4offset_);
			v1			= vabssub(vG4offset, vG2offset_);
			v2			= (ushort16)vadd(vG2offset, vG4offset_);
			v3			= (ushort16)vadd(vG4offset, vG2offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vG1L0best	= vselect(v2,v3,SecMask);
#else
			SecMask		= vcmp(lt,vabssub(vG2offset, vG4offset_),vabssub(vG4offset, vG2offset_));
			vG1L0best 	= vselect( (ushort16)vadd(vG2offset, vG4offset_), (ushort16)vadd(vG4offset, vG2offset_), SecMask); 
#endif
			//PRINT_CEVA_VRF("vG1L0best", vG1L0best, stderr);
			// ========= line 1 GRBG ==============
			// -------------r-----------------
			// abs(R2-R6),abs(R4 - R4+4)
			// ----
			//vR2offset	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed1);
			vR6offset	= (ushort16)vperm(vR6,vGR6,vcfgAdjRed1);
			vR4offset_	= (ushort16)vperm(vR4,vGR4,vcfgAdjRed2);
#if CODE_SCATTER
			v0			= vabssub(vR2offset, vR6offset);
			v1			= vabssub(vR4, vR4offset_);
			v2			= (ushort16)vadd(vR2offset, vR6offset);
			v3			= (ushort16)vadd(vR4, vR4offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vRL1best	= vselect(v2,v3,SecMask);
#else
			SecMask 	= vcmp(lt,vabssub(vR2offset, vR6offset),vabssub(vR4, vR4offset_));
			vRL1best 	= vselect( (ushort16)vadd(vR2offset, vR6offset), (ushort16)vadd(vR4, vR4offset_), SecMask); 
#endif
			//PRINT_CEVA_VRF("vRL1best", vRL1best, stderr);

			// -------------b-----------------
			// abs(B3-B7),abs(B5 - B5+4)
			// ----
			//vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu1);
			vB7offset	= (ushort16)vperm(vB7,vBG7,vcfgAdjBlu1);
			vB5offset_	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu2);
#if CODE_SCATTER
			v0			= vabssub(vB3offset, vB7offset);
			v1			= vabssub(vB5, vB5offset_);
			v2			= (ushort16)vadd(vB3offset, vB7offset);
			v3			= (ushort16)vadd(vB5, vB5offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vBL1best	= vselect(v2,v3,SecMask);
#else
			SecMask 	= vcmp(lt,vabssub(vB3offset, vB7offset),vabssub(vB5, vB5offset_));
			vBL1best 	= vselect( (ushort16)vadd(vB3offset, vB7offset), (ushort16)vadd(vB5, vB5offset_), SecMask); 
#endif
			//PRINT_CEVA_VRF("vBL1best", vBL1best, stderr);


			// -------------G-----------------
			// abs(G3-G5),abs(G4 - G6)
			//  G3  G5 
			vG5offset	= (ushort16)vperm(vG5,vBG5,vcfgAdjRed1);
			//vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);
#if CODE_SCATTER
			v0			= vabssub(vG3, vG5offset);
			v1			= vabssub(vG5, vG3offset);
			v2			= (ushort16)vadd(vG3, vG5offset);
			v3			= (ushort16)vadd(vG5, vG3offset); 
			SecMask 	= vcmp(lt,v0,v1);
			vG0L1best	= vselect(v2,v3,SecMask);
#else
			
			SecMask		= vcmp(lt,vabssub(vG3, vG5offset),vabssub(vG5, vG3offset));
			vG0L1best 	= vselect( (ushort16)vadd(vG3, vG5offset), (ushort16)vadd(vG5, vG3offset), SecMask); 
#endif
			//PRINT_CEVA_VRF("vG0L1best", vG0L1best, stderr);
			//  G4   G6 
			vG6offset	= (ushort16)vperm(vG6,vGR6,vcfgAdjGre1);
			//vG4offset	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre1);
			vG6offset_	= (ushort16)vperm(vG6,vGR6,vcfgAdjGre2);
			//vG4offset_	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre2);
#if CODE_SCATTER
			v0			= vabssub(vG4offset, vG6offset_);
			v1			= vabssub(vG6offset, vG4offset_);
			v2			= (ushort16)vadd(vG4offset, vG6offset_);
			v3			= (ushort16)vadd(vG6offset, vG4offset_); 
			SecMask 	= vcmp(lt,v0,v1);
			vG1L1best	= vselect(v2,v3,SecMask);
#else
			SecMask		= vcmp(lt,vabssub(vG4offset, vG6offset_),vabssub(vG6offset, vG4offset_));
			vG1L1best 	= vselect( (ushort16)vadd(vG4offset, vG6offset_), (ushort16)vadd(vG6offset, vG4offset_), SecMask); 
#endif
			//PRINT_CEVA_VRF("vG1L1best", vG1L1best, stderr);

			// ------------------------------------------
			// overlap the long time and short time image
			// 4 Line x 32 for long and short.
			// ------------------------------------------
			// line 0 GRBG
			//vR2packed	= (ushort16)vperm(vR2,vGR2,vcfgAdjRedPack); // orignal
			vRL0best	= (ushort16)vshiftr(vRL0best, (unsigned char)1);// times// interpoaltion		
			vRL0Long	= vselect(vR2packed, vRL0best, R_B_LONG_PATTERN);

			vRL0Short	= vselect(vR2packed, vRL0best, R_B_SHORT_PATTERN);
			vRL0Short	= (ushort16)vshiftl(vRL0Short, log2_expTimes);// times



			//vB3packed	= vB3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vBL0best	= (ushort16)vshiftr(vBL0best, (unsigned char)1);// times// interpoaltion		
			vBL0Long	= vselect(vB3offset/*vB3packed*/, vBL0best, R_B_LONG_PATTERN);

			vBL0Short	= vselect(vB3offset/*vB3packed*/, vBL0best, R_B_SHORT_PATTERN);
			vBL0Short	= (ushort16)vshiftl(vBL0Short, log2_expTimes);// times



			//vG2packed	= vG2offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L0best	= (ushort16)vshiftr(vG0L0best, (unsigned char)1);// times// interpoaltion		
			vG0L0Long	= vselect(vG2offset/*vG2packed*/, vG0L0best, G_LONG_PATTERN);

			vG0L0Short	= vselect(vG2offset/*vG2packed*/, vG0L0best, G_SHORT_PATTERN);
			vG0L0Short	= (ushort16)vshiftl(vG0L0Short, log2_expTimes);// times


			//vG3packed	= vG3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L0best	= (ushort16)vshiftr(vG1L0best, (unsigned char)1);// times// interpoaltion		
			vG1L0Long	= vselect(vG3offset/*vG3packed*/, vG1L0best, G_SHORT_PATTERN);

			vG1L0Short	= vselect(vG3offset/*vG3packed*/, vG1L0best, G_LONG_PATTERN);
			vG1L0Short	= (ushort16)vshiftl(vG1L0Short, log2_expTimes);// times


			// line 1 GRBG
			vR4packed	= (ushort16)vperm(vR4,vGR4,vcfgAdjRedPack); // orignal
			vRL1best	= (ushort16)vshiftr(vRL1best, (unsigned char)1);// times// interpoaltion		
			vRL1Long	= vselect(vR4packed, vRL1best, R_B_SHORT_PATTERN);

			vRL1Short	= vselect(vR4packed, vRL1best, R_B_LONG_PATTERN);
			vRL1Short	= (ushort16)vshiftl(vRL1Short, log2_expTimes);// times

			vB5packed	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
			vBL1best	= (ushort16)vshiftr(vBL1best, (unsigned char)1);// times// interpoaltion		
			vBL1Long	= vselect(vB5packed, vBL1best, R_B_SHORT_PATTERN );

			vBL1Short	= vselect(vB5packed, vBL1best, R_B_LONG_PATTERN);
			vBL1Short	= (ushort16)vshiftl(vBL1Short, log2_expTimes);// times

			//vG4packed	= vG4offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L1best	= (ushort16)vshiftr(vG0L1best, (unsigned char)1);// times// interpoaltion		
			vG0L1Long	= vselect(vG4offset/*vG4packed*/, vG0L1best, G_LONG_PATTERN);

			vG0L1Short	= vselect(vG4offset/*vG4packed*/, vG0L1best, G_SHORT_PATTERN);
			vG0L1Short	= (ushort16)vshiftl(vG0L1Short, log2_expTimes);// times


			//vG5packed	= vG5offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L1best	= (ushort16)vshiftr(vG1L1best, (unsigned char)1);// times// interpoaltion		
			vG1L1Long	= vselect(vG5offset/*vG5packed*/, vG1L1best, G_SHORT_PATTERN);

			vG1L1Short	= vselect(vG5offset/*vG5packed*/, vG1L0best, G_LONG_PATTERN);
			vG1L1Short	= (ushort16)vshiftl(vG1L1Short, log2_expTimes);// times


			vpst(vRL0Long, 	pL0Red, vOffsetGap2);
			vpst(vBL0Long,  pL0Blu, vOffsetGap2);
			vpst(vG0L0Long, pL0G0,  vOffsetGap2);
			vpst(vG1L0Long, pL0G1,  vOffsetGap2);

			vpst(vRL0Short,	 pL0Red_s, vOffsetGap2);
			vpst(vBL0Short,  pL0Blu_s, vOffsetGap2);
			vpst(vG0L0Short, pL0G0_s,  vOffsetGap2);
			vpst(vG1L0Short, pL0G1_s,  vOffsetGap2);

			vpst(vRL1Long, 	pL1Red, vOffsetGap2);
			vpst(vBL1Long,  pL1Blu, vOffsetGap2);
			vpst(vG0L1Long, pL1G0,  vOffsetGap2);
			vpst(vG1L1Long, pL1G1,  vOffsetGap2);

			vpst(vRL1Short,	 pL1Red_s, vOffsetGap2);
			vpst(vBL1Short,  pL1Blu_s, vOffsetGap2);
			vpst(vG0L1Short, pL1G0_s,  vOffsetGap2);
			vpst(vG1L1Short, pL1G1_s,  vOffsetGap2);

			//CONNECT_LUT
			// ------------------------------------------
			// use difference to LUT
			// ------------------------------------------
			v0	= vabssub(vG0L0Long, vG0L0Short);
			v1	= vabssub(vRL0Long,  vRL0Short);
			v2	= vabssub(vBL0Long,  vBL0Short);
			v3	= vabssub(vG1L0Long, vG1L0Short);

			v4	= vabssub(vG0L1Long, vG0L1Short);
			v5	= vabssub(vRL1Long,  vRL1Short);
			v6	= vabssub(vBL1Long,  vBL1Short);
			v7	= vabssub(vG1L1Long, vG1L1Short);

			// scale the diff by [2^param.bits/(param.noise*param.exptimes)] = 1024/(64*8)
			v0 	= (ushort16)vshiftl(v0 , 1); 
			v1 	= (ushort16)vshiftl(v1 , 1); 
			v2	= (ushort16)vshiftl(v2 , 1); 
			v3	= (ushort16)vshiftl(v3 , 1); 
			v4 	= (ushort16)vshiftl(v4 , 1); 
			v5 	= (ushort16)vshiftl(v5 , 1); 
			v6	= (ushort16)vshiftl(v6 , 1); 
			v7	= (ushort16)vshiftl(v7 , 1); 

			// and min(x,ref)
			v0 	= vmin(v0 , (ushort16) normValue);         
			v1 	= vmin(v1 , (ushort16) normValue);         
			v2	= vmin(v2 , (ushort16) normValue);         
			v3	= vmin(v3 , (ushort16) normValue);         
			v4 	= vmin(v4 , (ushort16) normValue);         
			v5 	= vmin(v5 , (ushort16) normValue);         
			v6	= vmin(v6 , (ushort16) normValue);         
			v7	= vmin(v7 , (ushort16) normValue);         
			// store the sad to DTCM.	
			vpst(v0, 	pScaleL0G0,  vOffsetGap2);
			vpst(v1,	pScaleL0Red, vOffsetGap2);
			vpst(v2, 	pScaleL0Blu, vOffsetGap2);
			vpst(v3, 	pScaleL0G1,  vOffsetGap2);

			vpst(v4, 	pScaleL1G0,  vOffsetGap2);
			vpst(v5,	pScaleL1Red, vOffsetGap2);
			vpst(v6, 	pScaleL1Blu, vOffsetGap2);
			vpst(v7, 	pScaleL1G1,  vOffsetGap2);

			pScaleL0G0   += 4*HDR_FILTER_W;	
			pScaleL0Red  += 4*HDR_FILTER_W;	
			pScaleL0Blu  += 4*HDR_FILTER_W;	
			pScaleL0G1   += 4*HDR_FILTER_W;	
			                                
			pScaleL1G0   += 4*HDR_FILTER_W;	
			pScaleL1Red  += 4*HDR_FILTER_W;	
			pScaleL1Blu  += 4*HDR_FILTER_W;	
			pScaleL1G1   += 4*HDR_FILTER_W;	

			pL0G0	  += 4*HDR_BLOCK_W;	                        
			pL0Red 	  += 4*HDR_BLOCK_W;	                        
			pL0Blu	  += 4*HDR_BLOCK_W;	                      
			pL0G1	  += 4*HDR_BLOCK_W;	                   
			pL1G0	  += 4*HDR_BLOCK_W;	                    
			pL1Red 	  += 4*HDR_BLOCK_W;	                       
			pL1Blu 	  += 4*HDR_BLOCK_W;	                       
			pL1G1	  += 4*HDR_BLOCK_W;	                   
			                                               
			pL0G0_s	  += 4*HDR_BLOCK_W;	
			pL0Red_s  += 4*HDR_BLOCK_W;	
			pL0Blu_s  += 4*HDR_BLOCK_W;	
			pL0G1_s	  += 4*HDR_BLOCK_W;	
			pL1G0_s	  += 4*HDR_BLOCK_W;	
			pL1Red_s  += 4*HDR_BLOCK_W;	
			pL1Blu_s  += 4*HDR_BLOCK_W;	
			pL1G1_s	  += 4*HDR_BLOCK_W;	

			
			// move the 4 line data up.
			vR0offset	= vR4offset;		
			vR2offset_	= (ushort16)vperm(vR6,vGR6,vcfgAdjRed2);//line 6 for Red hori data

			vB1offset	= vB5offset;
			vB3offset_	= (ushort16)vperm(vB7,vBG7,vcfgAdjBlu2);//line 6 for blue hori data

			vG2offset	= vG6offset;//(ushort16)vperm(vG2,vGR2,vcfgAdjGre1);
			vG2offset_	= vG6offset_;//(ushort16)vperm(vG2,vGR2,vcfgAdjGre2);

			vG1offset	= (ushort16)vperm(vG5,vBG5,vcfgAdjRed1);
			vG3offset	= (ushort16)vperm(vG7,vBG7,vcfgAdjRed1);

			vR2offset	= vR6offset;//(ushort16)vperm(vR6,vGR6,vcfgAdjRed1);
			vB3offset	= vB7offset;//(ushort16)vperm(vB7,vBG7,vcfgAdjBlu1);

			vR2packed	= (ushort16)vperm(vR6,vGR6,vcfgAdjRedPack); // orignal
			vR2			= vR6;
			vB3			= vB7;
			vG1			= vG5;
			vG3			= vG7;

		}
		// snd col
		pInLine0 = p_u16Src + 32 ;					// + 3,Load R 
		pInLine1 = p_u16Src + 32  + stride 		;	// + 1 load G & B
		pInLine2 = p_u16Src + 32  + 2*stride 	;	// + 1 LOAD R & G
		pInLine3 = p_u16Src + 32  + 3*stride	;	//     load B & G
		
		pInLine4 = p_u16Src + 32  + 4*stride	;	// + 2 LOAD G & R
		pInLine5 = p_u16Src + 32  + 5*stride	;	// + 2 LOAD B 
		pInLine6 = p_u16Src + 32  + 6*stride	;	// + 2 LOAD B 
		pInLine7 = p_u16Src + 32  + 7*stride	;	// + 2 LOAD B 



		pL0G0		= buff + 32									;                              
		pL0Red 		= buff + 32	+1									;                              
		pL0Blu		= buff + 32	   + HDR_BLOCK_W 	;                              
		pL0G1		= buff + 32	+1 + HDR_BLOCK_W 	;                              
		pL1G0		= buff + 32	   + 2*HDR_BLOCK_W ;                              
		pL1Red 		= buff + 32	+1 + 2*HDR_BLOCK_W ;                              
		pL1Blu 		= buff + 32	   + 3*HDR_BLOCK_W ;                              
		pL1G1		= buff + 32	+1 + 3*HDR_BLOCK_W ;                              
		                                                               
		pL0G0_s	 	= buff + 32	 			 	   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0Red_s 	= buff + 32	+1			 	   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0Blu_s 	= buff + 32	   + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0G1_s	 	= buff + 32	+1 + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1G0_s	 	= buff + 32	   + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1Red_s 	= buff + 32	+1 + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1Blu_s 	= buff + 32	   + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1G1_s	 	= buff + 32	+1 + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   

		pScaleL0G0	= scale 	+ 32 				;     
		pScaleL0Red = scale+1	+ 32 				;   
		pScaleL0Blu	= scale	  	+ 32 + HDR_FILTER_W ;   
		pScaleL0G1	= scale+1 	+ 32 + HDR_FILTER_W ;   
		pScaleL1G0	= scale   	+ 32 + 2*HDR_FILTER_W ;   
		pScaleL1Red = scale+1	+ 32 + 2*HDR_FILTER_W ;  
		pScaleL1Blu = scale  	+ 32 + 3*HDR_FILTER_W ;  
		pScaleL1G1	= scale+1	+ 32 + 3*HDR_FILTER_W ;   

	}
#ifdef __XM4__
	PROFILER_END();
#endif	
}

