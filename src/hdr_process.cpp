


#include <string.h>
#include "rk_bayerhdr.h"
#include "hdr_process.h"



static int countFiles = 0;
// define the block parameters.

// 32k
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_0") uint16_t		p_u16TabLongShort[962*16]   =
{
	#include "../table/longshort_mapping_16banks.dat"
};
// 32k
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_0") uint16_t		pWdrTab16banks[962*16]   =
{
	#include "../table/16banks.dat"
};



PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t		pL_S_ImageBuff[2][2*HDR_BLOCK_H*HDR_BLOCK_W] 	 = {0};// 4kx2 store long and short image.
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t 		pWeightBuff[(HDR_BLOCK_H+2)*(HDR_BLOCK_W+2)] 	 = {0};// 4k store weight  
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint16_t		pWeightFilter[HDR_BLOCK_H*HDR_BLOCK_W] 			 = {0};// 4k store weight fileterd.

PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint8_t 		pWeightBuff_bak[(HDR_BLOCK_H+2)*(HDR_BLOCK_W+2)] 	 = {0};// 4k store weight  
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_1") uint8_t		pWeightFilter_bak[HDR_BLOCK_H*HDR_BLOCK_W] 			 = {0};// 4k store weight fileterd.



extern uint16_t		pPrevThumb[THUMB_SIZE_W*THUMB_SIZE_W] ;// 32k store in DDR.
extern uint16_t		pCurrThumb[THUMB_SIZE_W*THUMB_SIZE_W] ;// 32k store in DDR.



HDRprocess::HDRprocess()
: mFrameNum(0)
{
}
HDRprocess::~HDRprocess()
{
}


PRAGMA_CSECT("zzhdr_sect")

void HDRprocess::FilterdLUTBilinear ( uint16_t*	p_u16Weight, 		//<<! [in] 0-1024 scale tab.
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
	 						 uint16_t 	frameNum,
							 uint16_t*	p_u16Dst)		//<<! [out] HDR out 16bit,have not do WDR.
{
#ifdef __XM4__
	if ( 0 == x_pos  && 0 == y_pos && mFrameNum > 0 )
		PROFILER_START(HDR_BLOCK_H, HDR_BLOCK_W);
#endif	
	uint16_t log2ExpTimes = 3;
	unsigned short* p_imgL 	= p_u16ImageL_S;
	unsigned short* p_imgS 	= p_u16ImageL_S + HDR_BLOCK_H*HDR_BLOCK_W;
	unsigned int 	row, col;
	unsigned short* p_src 	= p_u16Weight;// need to do max(3,3) filter
	unsigned short* p_dst 	= p_u16Dst;
	ushort16 v0a, v0b, v0c;
	ushort16 v1a, v1b, v1c;
	ushort16 v2a, v2b, v2c;
	ushort16 v3a, v3b, v3c;
	ushort16 v4a, v4b, v4c;
	ushort16 v5a, v5b, v5c;

	ushort16 vL0, vL1, vL2, vL3;
	ushort16 vS0, vS1, vS2, vS3;

	uint8_t  xinter[16] = {64 ,63 ,62 ,61 ,60 ,59 ,58 ,57 ,56 ,55 ,54 ,53 ,52 ,51 ,50 ,49 };  
	uchar16  biXref = *(uchar16*)xinter;
	uchar16  biXf0;
	uchar16	 biXf1;
	
	ushort16 v0, v1, v2, v3, v4, v5, v6, v7 ;
	ushort16 v8, v9, v10,v11,v12,v13,v14,v15;
	ushort16 vr0,vr1,vr2,vr3;
	ushort16 vdummy;
	ushort16 vtL0,vtL1,vtL2,vtL3;
	ushort16 vtS0,vtS1,vtS2,vtS3;
	ushort16 vOut0,vOut1,vOut2,vOut3;
	uchar32  vbi0,vbi1,vbi2,vbi3;
 	int16 	 vaccThumb  = (int16)0;
	ushort16 vAvgSeg;
	uint16_t thumb = 0;
	unsigned int vprMask;
	unsigned int vprRightMask;

	uint16_t point0Top = *p_u16PrevThumb;
	uint16_t point1Top = *(p_u16PrevThumb+1);

	uint16_t point0Bot = *(p_u16PrevThumb+thumbStride);
	uint16_t point1Bot = *(p_u16PrevThumb+thumbStride+1);
	
	vprMask = 0xFFFFFFFF;
	vprRightMask = 0xFFFFFFFF;
	if ((u32Cols & 31) != 0)
		vprRightMask = ((1 << (u32Cols & 31)) - 1);

	for(col = 0; col < u32Cols; col += 16) 
	{
		if (u32Cols - col < 32)
			vprMask = vprRightMask;

		p_src 	= p_u16Weight 	+ col ;
		p_dst 	= p_u16Dst 		+ col ;
		p_imgL 	= p_u16ImageL_S + col ;
		p_imgS 	= p_u16ImageL_S + col + HDR_BLOCK_H*HDR_BLOCK_W;
		
		vldov((ushort16*)(p_src), v0a, v0b, v0c, vdummy);// line0
		p_src += weightStep;
		vldov((ushort16*)(p_src), v1a, v1b, v1c, vdummy);// line1
		p_src += weightStep;
		vldov((ushort16*)(p_src), v2a, v2b, v2c, vdummy);// line2
		p_src += weightStep;
		vldov((ushort16*)(p_src), v3a, v3b, v3c, vdummy);// line3
		p_src += weightStep;
		vldov((ushort16*)(p_src), v4a, v4b, v4c, vdummy);// line4
		p_src += weightStep;
		vldov((ushort16*)(p_src), v5a, v5b, v5c, vdummy);// line5
		p_src += weightStep;


		v0 = vmax(v0a, v0b, v0c);
		v1 = vmax(v1a, v1b, v1c);
	#if ENABLE_WDR
		biXf0 = (uchar16)vsub((ushort16)biXref,		(ushort16)col);
		biXf1 = (uchar16)vsub((ushort16)(col+64),	(ushort16)biXref);
		// x aixs interpolation
		v6 = (ushort16)vmac3(biXf0, point0Top, biXf1, point1Top, (uint16) 0, (unsigned char)6);
		v7 = (ushort16)vmac3(biXf0, point0Bot, biXf1, point1Bot, (uint16) 0, (unsigned char)6);
	#endif
		for(row = 0; row < u32Rows; row+=4) 
		{
			vL0 = *(ushort16*) p_imgL;
			vL1 = *(ushort16*)(p_imgL+imageStep);
			vL2 = *(ushort16*)(p_imgL+2*imageStep);
			vL3 = *(ushort16*)(p_imgL+3*imageStep);
			p_imgL += 4*imageStep;
			
			vS0 = *(ushort16*) p_imgS;
			vS1 = *(ushort16*)(p_imgS+imageStep);
			vS2 = *(ushort16*)(p_imgS+2*imageStep);
			vS3 = *(ushort16*)(p_imgS+3*imageStep);
			p_imgS += 4*imageStep;
			
			v2  = vmax(v2a, v2b, v2c);
			v3  = vmax(v3a, v3b, v3c);
			v4  = vmax(v4a, v4b, v4c);
			v5  = vmax(v5a, v5b, v5c);
			
			// TODO: 2.  MAX(3,3)
			vr0 = vmax(v0, v1, v2);
			vr1 = vmax(v1, v2, v3);
			vr2 = vmax(v2, v3, v4);
			vr3 = vmax(v3, v4, v5);

			v0 = v4;
			v1 = v5;
			
			vldov((ushort16*)(p_src), v2a, v2b, v2c, vdummy);
			p_src += weightStep;
			vldov((ushort16*)(p_src), v3a, v3b, v3c, vdummy);
			p_src += weightStep;
			vldov((ushort16*)(p_src), v4a, v4b, v4c, vdummy);
			p_src += weightStep;
			vldov((ushort16*)(p_src), v5a, v5b, v5c, vdummy);
			p_src += weightStep;


			// TODO: 3. LUT by r_filter
			vtS0 = vpld(rel,p_u16TabLS , (short16)vr0);
			vtS1 = vpld(rel,p_u16TabLS , (short16)vr1);
			vtS2 = vpld(rel,p_u16TabLS , (short16)vr2);
			vtS3 = vpld(rel,p_u16TabLS , (short16)vr3);

			vtL0 = (ushort16)vsub((unsigned short)HDR_LS_QUANT, vtS0);
			vtL1 = (ushort16)vsub((unsigned short)HDR_LS_QUANT, vtS1);
			vtL2 = (ushort16)vsub((unsigned short)HDR_LS_QUANT, vtS2);
			vtL3 = (ushort16)vsub((unsigned short)HDR_LS_QUANT, vtS3);

			vbi0 = vcast(vtS0,vtL0);
			vbi1 = vcast(vtS1,vtL1);
			vbi2 = vcast(vtS2,vtL2);
			vbi3 = vcast(vtS3,vtL3);
			
			// TODO:  4.  mac(L,S,r)
			
			vOut0 = (ushort16)vmac3(splitsrc, psl, vS0, vL0, vbi0, (uint16) HDR_LS_ROUND, (unsigned char)HDR_LS_BITS); 
			vOut1 = (ushort16)vmac3(splitsrc, psl, vS1, vL1, vbi1, (uint16) HDR_LS_ROUND, (unsigned char)HDR_LS_BITS); 
			vOut2 = (ushort16)vmac3(splitsrc, psl, vS2, vL2, vbi2, (uint16) HDR_LS_ROUND, (unsigned char)HDR_LS_BITS); 
			vOut3 = (ushort16)vmac3(splitsrc, psl, vS3, vL3, vbi3, (uint16) HDR_LS_ROUND, (unsigned char)HDR_LS_BITS); 


			// TODO:  add 64x64 block to thumb a pixel.
			
			vaccThumb = vaccadd(vaccThumb, vOut0);
			vaccThumb = vaccadd(vaccThumb, vOut1);
			vaccThumb = vaccadd(vaccThumb, vOut2);
			vaccThumb = vaccadd(vaccThumb, vOut3);

			if ( frameNum )
			{
			#if ENABLE_WDR

				// TODO:  read Thumb to bilinear the map, LUT table for mac.

				// y aixs interpolation
				v8  = (ushort16)vmac3(v6, (unsigned char)(32 - row  ), v7, (unsigned char)row,     (int16) 0, (unsigned char)8);//(5+log2ExpTimes));
				v9  = (ushort16)vmac3(v6, (unsigned char)(32 - row-1), v7, (unsigned char)(row+1), (int16) 0, (unsigned char)8);//(5+log2ExpTimes));
				v10 = (ushort16)vmac3(v6, (unsigned char)(32 - row-2), v7, (unsigned char)(row+2), (int16) 0, (unsigned char)8);//(5+log2ExpTimes));
				v11 = (ushort16)vmac3(v6, (unsigned char)(32 - row-3), v7, (unsigned char)(row+3), (int16) 0, (unsigned char)8);//(5+log2ExpTimes));


				// LUT wdr table for scale.
				v8  = vpld(rel,pScaleTab16banks , (short16)v8 );
				v9  = vpld(rel,pScaleTab16banks , (short16)v9 );
				v10 = vpld(rel,pScaleTab16banks , (short16)v10);
				v11 = vpld(rel,pScaleTab16banks , (short16)v11);
				// TODO: do wdr by prev thumb image to bilinear hdrout.
				// mpy and clip
				vOut0 = (ushort16)vmpy(psl, vOut0, v8,  (unsigned char)(6+log2ExpTimes));
				vOut1 = (ushort16)vmpy(psl, vOut1, v9,  (unsigned char)(6+log2ExpTimes));
				vOut2 = (ushort16)vmpy(psl, vOut2, v10, (unsigned char)(6+log2ExpTimes));
				vOut3 = (ushort16)vmpy(psl, vOut3, v11, (unsigned char)(6+log2ExpTimes));
				
			#endif
			}
			
			vst(vOut0,(ushort16*)(p_dst)   				,			0xffff);  
			vst(vOut1,(ushort16*)(p_dst+imageStep)		,			0xffff);  
			vst(vOut2,(ushort16*)(p_dst+2*imageStep)	,			0xffff);  
			vst(vOut3,(ushort16*)(p_dst+3*imageStep)	,			0xffff);  
			p_dst += 4*imageStep;
			
			
		}
		
	}
	uint8_t	bitpsl	= (u32Rows == 32) ? 5 : 4;					// 3024/32 = 94x32 + 16;
	bitpsl			= (u32Cols == 64) ? (bitpsl+2) : (bitpsl+1);// 1/4 or 1/2

	vAvgSeg			= (ushort16)vaccshiftr(vaccThumb, (ushort16) bitpsl);//4x32
	thumb 			= (vintrasum(vintrasum(vAvgSeg))>>4);			// scale 32x64 block to one pixel.
	*p_u16CurrThumb = thumb;
	
#ifdef __XM4__
	if ( 0 == x_pos  && 0 == y_pos && mFrameNum > 0 )
		PROFILER_END();
#endif	

}
PRAGMA_CSECT("zzhdr_sect")

void HDRprocess::zigzagDebayer(	uint16_t *p_u16Src, 
						uint16_t *p_u16Tab, 
						uint16_t blockW,
						uint16_t blockH,
						uint16_t stride,
						uint16_t normValue,
						uint16_t *buff,		//<<! [out] 32x64 short16  long and short
						uint16_t *scale) 	//<<! [out] 32x64 short16  table
						
{
#ifdef __XM4__
	if ( 0 == x_pos  && 0 == y_pos && mFrameNum > 0 )
		PROFILER_START(HDR_BLOCK_H, HDR_BLOCK_W);
#endif
	 uint8_t log2_expTimes 	= 3;// log2(8)	
	 uint8_t log2_resiScale = 1;//2^param.bits/(param.noise*param.exptimes)
	uint16_t i,j,blacklevel=64;
	uint16_t SecMaskR,SecMaskB,SecMaskG0,SecMaskG1;
	ushort16 vG0,vR0,vB1,vG1,vG2,vR2,vB3,vG3,vG4,vR4,vB5,vG5,vG6,vR6,vB7,vG7;
	ushort16 vGR0,vBG1,vGR2,vBG3,vGR4,vBG5,vGR6,vBG7;
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7;
	ushort16 vRTmp0,vRTmp1,vRTmp2,vRTmp3;
	ushort16 vBTmp0,vBTmp1,vBTmp2,vBTmp3;
	ushort16 vG0Tmp0,vG0Tmp1,vG0Tmp2,vG0Tmp3;
	ushort16 vG1Tmp0,vG1Tmp1,vG1Tmp2,vG1Tmp3;
	
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

			//vB1offset	= (ushort16)vperm(vB1,vBG1,vcfgAdjBlu1);
			vB5offset	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
			//vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu2);
			//vG1offset	= (ushort16)vperm(vG1,vBG1,vcfgAdjRed1);
			//vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);
			//vG2offset	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre1);
			vG4offset	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre1);
			//vG2offset_	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre2);
			vG4offset_	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre2);

			
		#if CODE_SCATTER
			vRTmp0		= vabssub(vR0offset, vR4offset);
			vBTmp0		= vabssub(vB1offset, vB5offset);			
			vG0Tmp0		= vabssub(vG1, vG3offset);		
			vG1Tmp0		= vabssub(vG2offset, vG4offset_);		

			vRTmp1		= vabssub(vR2, vR2offset_);
			vBTmp1		= vabssub(vB3, vB3offset_);		
			vG0Tmp1		= vabssub(vG3, vG1offset);		
			vG1Tmp1		= vabssub(vG4offset, vG2offset_);


			vRTmp2		= (ushort16)vadd(vR0offset, vR4offset);
			vBTmp2		= (ushort16)vadd(vB1offset, vB5offset);			
			vG0Tmp2		= (ushort16)vadd(vG1, vG3offset);
			vG1Tmp2		= (ushort16)vadd(vG2offset, vG4offset_);


			vRTmp3		= (ushort16)vadd(vR2, vR2offset_); 
			vBTmp3		= (ushort16)vadd(vB3, vB3offset_); 
			vG0Tmp3		= (ushort16)vadd(vG3, vG1offset); 
			vG1Tmp3		= (ushort16)vadd(vG4offset, vG2offset_); 


			SecMaskR 	= vcmp(lt,vRTmp0,vRTmp1);
			SecMaskB 	= vcmp(lt,vBTmp0,vBTmp1);
			SecMaskG0 	= vcmp(lt,vG0Tmp0,vG0Tmp1);
			SecMaskG1 	= vcmp(lt,vG1Tmp0,vG1Tmp1);


			vRL0best	= vselect(vRTmp2,vRTmp3,SecMaskR);
			vBL0best	= vselect(vBTmp2,vBTmp3,SecMaskB);		
			vG0L0best	= vselect(vG0Tmp2,vG0Tmp3,SecMaskG0);
			vG1L0best	= vselect(vG1Tmp2,vG1Tmp3,SecMaskG1);

		#else
			SecMaskR 	= vcmp(lt,vabssub(vR0offset, vR4offset),vabssub(vR2, vR2offset_));
			SecMaskB 	= vcmp(lt,vabssub(vB1offset, vB5offset),vabssub(vB3, vB3offset_));
			SecMaskG0	= vcmp(lt,vabssub(vG1, vG3offset),vabssub(vG3, vG1offset));
			SecMaskG1	= vcmp(lt,vabssub(vG2offset, vG4offset_),vabssub(vG4offset, vG2offset_));

			vRL0best 	= vselect( (ushort16)vadd(vR0offset, vR4offset), (ushort16)vadd(vR2, vR2offset_), SecMaskR); 
			vBL0best 	= vselect( (ushort16)vadd(vB1offset, vB5offset), (ushort16)vadd(vB3, vB3offset_), SecMaskB); 
			vG0L0best 	= vselect( (ushort16)vadd(vG1, vG3offset), (ushort16)vadd(vG3, vG1offset), SecMaskG0); 
			vG1L0best 	= vselect( (ushort16)vadd(vG2offset, vG4offset_), (ushort16)vadd(vG4offset, vG2offset_), SecMaskG1); 
		#endif


			// line 0 GRBG
			//vR2packed	= (ushort16)vperm(vR2,vGR2,vcfgAdjRedPack); // orignal
			vRL0best	= (ushort16)vshiftr(vRL0best, 	(unsigned char)1);// times// interpoaltion		
			vBL0best	= (ushort16)vshiftr(vBL0best, 	(unsigned char)1);// times// interpoaltion		
			vG0L0best	= (ushort16)vshiftr(vG0L0best, 	(unsigned char)1);// times// interpoaltion		
			vG1L0best	= (ushort16)vshiftr(vG1L0best, 	(unsigned char)1);// times// interpoaltion		

			vRL0Long	= vselect(vR2packed, vRL0best, R_B_LONG_PATTERN);
			vBL0Long	= vselect(vB3offset/*vB3packed*/, vBL0best, R_B_LONG_PATTERN);
			vG0L0Long	= vselect(vG2offset/*vG2packed*/, vG0L0best, G_LONG_PATTERN);
			vG1L0Long	= vselect(vG3offset/*vG3packed*/, vG1L0best, G_SHORT_PATTERN);


			vRL0Short	= vselect(vR2packed, vRL0best, R_B_SHORT_PATTERN);
			vBL0Short	= vselect(vB3offset/*vB3packed*/, vBL0best, R_B_SHORT_PATTERN);
			vG0L0Short	= vselect(vG2offset/*vG2packed*/, vG0L0best, G_SHORT_PATTERN);
			vG1L0Short	= vselect(vG3offset/*vG3packed*/, vG1L0best, G_LONG_PATTERN);

			vRL0Short	= (ushort16)vshiftl(vRL0Short, log2_expTimes);// times
			vBL0Short	= (ushort16)vshiftl(vBL0Short, log2_expTimes);// times
			vG0L0Short	= (ushort16)vshiftl(vG0L0Short, log2_expTimes);// times
			vG1L0Short	= (ushort16)vshiftl(vG1L0Short, log2_expTimes);// times

			v0	= vabssub(vG0L0Long, vG0L0Short);
			v1	= vabssub(vRL0Long,  vRL0Short);
			v2	= vabssub(vBL0Long,  vBL0Short);
			v3	= vabssub(vG1L0Long, vG1L0Short);

			// scale the diff by [2^param.bits/(param.noise*param.exptimes)] = 1024/(64*8)
			v0 	= (ushort16)vshiftl(v0 , log2_resiScale); 
			v1 	= (ushort16)vshiftl(v1 , log2_resiScale); 
			v2	= (ushort16)vshiftl(v2 , log2_resiScale); 
			v3	= (ushort16)vshiftl(v3 , log2_resiScale);

			v0 	= vmin(v0 , (ushort16) normValue);         
			v1 	= vmin(v1 , (ushort16) normValue);         
			v2	= vmin(v2 , (ushort16) normValue);         
			v3	= vmin(v3 , (ushort16) normValue);         

			vpst(v0, 	pScaleL0G0,  vOffsetGap2);
			vpst(v1,	pScaleL0Red, vOffsetGap2);
			vpst(v2, 	pScaleL0Blu, vOffsetGap2);
			vpst(v3, 	pScaleL0G1,  vOffsetGap2);






			//PRINT_CEVA_VRF("vG1L0best", vG1L0best, stderr);
			// ========= line 1 GRBG ==============
			// -------------r-----------------
			// abs(R2-R6),abs(R4 - R4+4)
			// ----
			//vR2offset	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed1);
			vR6offset	= (ushort16)vperm(vR6,vGR6,vcfgAdjRed1);
			vR4offset_	= (ushort16)vperm(vR4,vGR4,vcfgAdjRed2);
			// -------------b-----------------
			// abs(B3-B7),abs(B5 - B5+4)
			// ----
			//vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu1);
			vB7offset	= (ushort16)vperm(vB7,vBG7,vcfgAdjBlu1);
			vB5offset_	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu2);
			// -------------G-----------------
			// abs(G3-G5),abs(G4 - G6)
			//  G3  G5 
			vG5offset	= (ushort16)vperm(vG5,vBG5,vcfgAdjRed1);
			//vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);
			//PRINT_CEVA_VRF("vG0L1best", vG0L1best, stderr);
			//  G4   G6 
			vG6offset	= (ushort16)vperm(vG6,vGR6,vcfgAdjGre1);
			//vG4offset	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre1);
			vG6offset_	= (ushort16)vperm(vG6,vGR6,vcfgAdjGre2);
			//vG4offset_	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre2);
			
		#if CODE_SCATTER

			vRTmp0		  	= vabssub(vR2offset, vR6offset);       
			vBTmp0		  	= vabssub(vB3offset, vB7offset);		
			vG0Tmp0			= vabssub(vG3, vG5offset);		
			vG1Tmp0			= vabssub(vG4offset, vG6offset_);						
			     
			vRTmp1		  	= vabssub(vR4, vR4offset_);
			vBTmp1		  	= vabssub(vB5, vB5offset_);			
			vG0Tmp1			= vabssub(vG5, vG3offset);		
			vG1Tmp1			= vabssub(vG6offset, vG4offset_);						
			     
			vRTmp2		  	= (ushort16)vadd(vR2offset, vR6offset);
			vBTmp2		  	= (ushort16)vadd(vB3offset, vB7offset);			
			vG0Tmp2			= (ushort16)vadd(vG3, vG5offset);			
			vG1Tmp2			= (ushort16)vadd(vG4offset, vG6offset_);
			     
			vRTmp3		  	= (ushort16)vadd(vR4, vR4offset_); 
			vBTmp3		  	= (ushort16)vadd(vB5, vB5offset_); 			
			vG0Tmp3			= (ushort16)vadd(vG5, vG3offset); 			
			vG1Tmp3			= (ushort16)vadd(vG6offset, vG4offset_); 			
					
			SecMaskR 		= vcmp(lt,vRTmp0,vRTmp1		);
			SecMaskB 		= vcmp(lt,vBTmp0,vBTmp1		);
			SecMaskG0		= vcmp(lt,vG0Tmp0,vG0Tmp1	);
			SecMaskG1 		= vcmp(lt,vG1Tmp0,vG1Tmp1	);				
					
			vRL1best		= vselect(vRTmp2,	vRTmp3,	SecMaskR);
			vBL1best		= vselect(vBTmp2,	vBTmp3,	SecMaskB);
			vG0L1best		= vselect(vG0Tmp2,vG0Tmp3,SecMaskG0);
			vG1L1best		= vselect(vG1Tmp2,vG1Tmp3,SecMaskG1);

		#else
			SecMaskR 	= vcmp(lt,vabssub(vR2offset, vR6offset),vabssub(vR4, vR4offset_));
			SecMaskB 	= vcmp(lt,vabssub(vB3offset, vB7offset),vabssub(vB5, vB5offset_));
			SecMaskG0	= vcmp(lt,vabssub(vG3, vG5offset),vabssub(vG5, vG3offset));
			SecMaskG1	= vcmp(lt,vabssub(vG4offset, vG6offset_),vabssub(vG6offset, vG4offset_));

			vRL1best 	= vselect( (ushort16)vadd(vR2offset, vR6offset), (ushort16)vadd(vR4, vR4offset_), SecMaskR); 
			vBL1best 	= vselect( (ushort16)vadd(vB3offset, vB7offset), (ushort16)vadd(vB5, vB5offset_), SecMaskB); 
			vG0L1best 	= vselect( (ushort16)vadd(vG3, vG5offset), (ushort16)vadd(vG5, vG3offset), SecMaskG0); 
			vG1L1best 	= vselect( (ushort16)vadd(vG4offset, vG6offset_), (ushort16)vadd(vG6offset, vG4offset_), SecMaskG1); 

		#endif

			//PRINT_CEVA_VRF("vG1L1best", vG1L1best, stderr);

			// ------------------------------------------
			// overlap the long time and short time image
			// 4 Line x 32 for long and short.
			// ------------------------------------------


			// line 1 GRBG
			vR4packed	= (ushort16)vperm(vR4,vGR4,vcfgAdjRedPack); // orignal
			vB5packed	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);

			vRL1best	= (ushort16)vshiftr(vRL1best, (unsigned char)1);// times// interpoaltion		
			vBL1best	= (ushort16)vshiftr(vBL1best, (unsigned char)1);// times// interpoaltion		
			vG0L1best	= (ushort16)vshiftr(vG0L1best, (unsigned char)1);// times// interpoaltion		
			vG1L1best	= (ushort16)vshiftr(vG1L1best, (unsigned char)1);// times// interpoaltion		


			vRL1Long	= vselect(vR4packed, vRL1best, R_B_SHORT_PATTERN);
			vBL1Long	= vselect(vB5packed, vBL1best, R_B_SHORT_PATTERN );
			vG0L1Long	= vselect(vG4offset/*vG4packed*/, vG0L1best, G_LONG_PATTERN);
			vG1L1Long	= vselect(vG5offset/*vG5packed*/, vG1L1best, G_SHORT_PATTERN);

			vRL1Short	= vselect(vR4packed, vRL1best, R_B_LONG_PATTERN);
			vBL1Short	= vselect(vB5packed, vBL1best, R_B_LONG_PATTERN);
			vG0L1Short	= vselect(vG4offset/*vG4packed*/, vG0L1best, G_SHORT_PATTERN);
			vG1L1Short	= vselect(vG5offset/*vG5packed*/, vG1L0best, G_LONG_PATTERN);

			vRL1Short	= (ushort16)vshiftl(vRL1Short, log2_expTimes);// times
			vBL1Short	= (ushort16)vshiftl(vBL1Short, log2_expTimes);// times
			vG0L1Short	= (ushort16)vshiftl(vG0L1Short, log2_expTimes);// times
			vG1L1Short	= (ushort16)vshiftl(vG1L1Short, log2_expTimes);// times

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

			v4	= vabssub(vG0L1Long, vG0L1Short);
			v5	= vabssub(vRL1Long,  vRL1Short);
			v6	= vabssub(vBL1Long,  vBL1Short);
			v7	= vabssub(vG1L1Long, vG1L1Short);

			// scale the diff by [2^param.bits/(param.noise*param.exptimes)] = 1024/(64*8)
			v4 	= (ushort16)vshiftl(v4 , log2_resiScale); 
			v5 	= (ushort16)vshiftl(v5 , log2_resiScale); 
			v6	= (ushort16)vshiftl(v6 , log2_resiScale); 
			v7	= (ushort16)vshiftl(v7 , log2_resiScale); 

			// and min(x,ref)
			v4 	= vmin(v4 , (ushort16) normValue);         
			v5 	= vmin(v5 , (ushort16) normValue);         
			v6	= vmin(v6 , (ushort16) normValue);         
			v7	= vmin(v7 , (ushort16) normValue);         
			// store the sad to DTCM.	
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
	if ( 0 == x_pos  && 0 == y_pos && mFrameNum > 0 )
		PROFILER_END();
#endif	
}

//PRAGMA_CSECT("zzhdr_sect")

void HDRprocess::dma_2Dtransf(unsigned short *dst, 
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



PRAGMA_CSECT("zzhdr_sect")

void HDRprocess::hdr_block_process(int 		  x,
							int 	  y,
							int		  thumbStride,
							int 	  frameNum,
							uint16_t *pRawInBuff, 
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

	if ( frameNum > 0 && 0 == x && 0 == y)	
	{
		memcpy(pPrevThumb,pCurrThumb,sizeof(uint16_t)*THUMB_SIZE_W*THUMB_SIZE_W);
		// nonlinear clip for thumb
		;

		// gaussian filter for thumb
		;

		// WdrScaleTable -> 16banks
		//uint16_t exp_times = MIN((RK_U16)(com->mIspGain + 0.5), 24) - 1;
		//printf("exp_times=%d\n",exp_times);
		//lutWdrTable(tmpWCT_DSP, pWdrScaleTable);
		
	}

	FilterdLUTBilinear ( pWeightBuff				, 	//<<! [in] 0-1024 scale tab.
						 p_u16TabLongShort			,	//<<! [in] 0-1024 scale tab, may be can use char type for 0-255
						 pL_S_ImageBuff[buffIdx]	,	//<<! [in] long and short image[block32x64]
						 pPrevThumb + (y>>5)*thumbStride + (x>>6) ,	//<<! [in] previous frame thumb image for WDR scale.
						 pCurrThumb + (y>>5)*thumbStride + (x>>6) ,
						 pWdrTab16banks				,
						 thumbStride				,
						 HDR_FILTER_W				, 	//<<! [in] weight stride which add padding.
						 HDR_BLOCK_W				, 	//<<! [in] 16 align
						 validH						, 	//<<! [in] 
						 validW						,	//<<! [in]  
 						 normValue					,
 						 frameNum					,
						 pHDRoutBuff);					//<<! [out] HDR out 16bit,have not do WDR.


		

#if HDR_DEBUG_ENABLE
    char name_hdr[512],name_w[512];
    char name_image[512], name_weight[512];;

	if  ( y_pos == 0 && x_pos == 0 && frameNum > 0)
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


