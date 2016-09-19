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
**   	------------
**		--	G R
**			B G ---
**		------------
** Copyright 2016, rockchip.
**
***************************************************************************/
#include "XM4_defines.h"
#include <vec-c.h>
#include "rk_typedef.h"
#include "rk_bayerwdr.h"
#include "profiler.h"
#include "DebugFiles.h"


/*
// void max3x3 (unsigned short *p_u16Src, 
// 				unsigned short *p_u16Dst, 
// 				int s32SrcStep, 
// 				int s32DstStep, 
// 				unsigned int u32Rows, 
// 				unsigned int u32Cols)
// {
// 	
// 	unsigned int 	row, col;
// 	unsigned short* p_src = p_u16Src;
// 	unsigned short* p_dst = p_u16Dst;
// 	ushort16 v0a, v0b, v0c;
// 	ushort16 v1a, v1b, v1c;
// 	ushort16 v2a, v2b, v2c;
// 	ushort16 v0, v1, v2;
// 	ushort16 vdummy;
// 	ushort16 u1;
// 	unsigned int vprMask;
// 	unsigned int vprRightMask;
// 	
// 	vprMask = 0xFFFFFFFF;
// 	vprRightMask = 0xFFFFFFFF;
// 	if ((u32Cols & 31) != 0)
// 		vprRightMask = ((1 << (u32Cols & 31)) - 1);
// 
// 	for(col = 0; col < u32Cols; col += 16) 
// 	{
// 		if (u32Cols - col < 32)
// 			vprMask = vprRightMask;
// 
// 		p_src = p_u16Src + col;
// 		p_dst = p_u16Dst + col;
// 
// 		vldov((ushort16*)(p_src), v0a, v0b, v0c, vdummy);
// 		p_src += s32SrcStep;
// 		vldov((ushort16*)(p_src), v1a, v1b, v1c, vdummy);
// 		p_src += s32SrcStep;
// 		vldov((ushort16*)(p_src), v2a, v2b, v2c, vdummy);
// 		p_src += s32SrcStep;
// 
// 		v0 = vmax(v0a, v0b, v0c);
// 		v1 = vmax(v1a, v1b, v1c);
// 
// 		for(row = 0; row < u32Rows; row++) 
// 		{
// 			v2 = vmax(v2a, v2b, v2c);
// 			u1 = vmax(v0, v1, v2);
// 
// 			v0 = v1;
// 			v1 = v2;
// 			vldov((uchar32*)(p_src), v2a, v2b, v2c, vdummy);
// 			p_src += s32SrcStep;
// 
// 			// store 32 result pixels
// 			vst(u1, (uchar32*)p_dst, vprMask);		
// 			p_dst+=s32DstStep;
// 		}
// 	}
// 		GET_FUNCTION_NAME(ceva::g_p_function_name);
// }
*/

//<<! do zigzag hdr stag2 for long and short image.
//<<! 1. vld imgL,imgS and diff = SAD(imgL,imgS)
//<<! 2. r_filter = MAX(3,3,r)
//<<! 3. LUT by r_filter
//<<! 4. mac(L,S,r)
//<<! 5. WDR
#define 	HDR_LS_BITS			8
#define 	HDR_LS_ROUND		(1<<(HDR_LS_BITS-1))
#define 	HDR_LS_QUANT		((1<<HDR_LS_BITS)-1)
void FilterdLUTBilinear ( uint16_t *p_u16Weight, 		//<<! [in] 0-1024 scale tab.
							 uint16_t *p_u16TabLS, 		//<<! [in] 0-1024 scale tab, may be can use char type for 0-255
							 uint16_t *p_u16ImageL_S,	//<<! [in] long and short image[block32x64]
							 uint16_t *p_u16PrevThumb,	//<<! [in] previous frame thumb image for WDR scale.
							 int32_t weightStep, 		//<<! [in] weight stride which add padding.
							 int32_t imageStep, 		//<<! [in] 16 align
							 uint32_t u32Rows, 			//<<! [in] 
							 uint32_t u32Cols,			//<<! [in]  
	 						 uint16_t normValue,
						#if HDR_DEBUG_ENABLE
							 uint16_t *p_u16Dst,
							 uint16_t  *p_u8FilterW,
							 int xPos,
							 int yPos)
						#else
							 uint16_t *p_u16Dst)		//<<! [out] HDR out 16bit,have not do WDR.
						#endif
{
#ifdef __XM4__
	PROFILER_START(HDR_BLOCK_H, HDR_BLOCK_W);
#endif	
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

	
	ushort16 v0, v1, v2, v3, v4, v5;
	ushort16 vr0,vr1,vr2,vr3;
	ushort16 vdummy;
	ushort16 vtL0,vtL1,vtL2,vtL3;
	ushort16 vtS0,vtS1,vtS2,vtS3;
	ushort16 vOut0,vOut1,vOut2,vOut3;
	uchar32  vbi0,vbi1,vbi2,vbi3;
	
	unsigned int vprMask;
	unsigned int vprRightMask;
	
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

			vst(vOut0,(ushort16*)(p_dst)   			,			0xffff);  
			vst(vOut1,(ushort16*)(p_dst+imageStep)	,			0xffff);  
			vst(vOut2,(ushort16*)(p_dst+2*imageStep)	,			0xffff);  
			vst(vOut3,(ushort16*)(p_dst+3*imageStep)	,			0xffff);  
			p_dst += 4*imageStep;
			
			// TODO: do wdr by prev thumb image to bilinear hdrout.

			
			
		}
	}

#ifdef __XM4__
PROFILER_END();
#endif	

}

	
/**
 * 
 * \name  do max3x3 filter and bilinear for HDR out.
 * \param[in] 	p_u16Src 	 Pointer to the first source 
 * \param[out] 	p_u16Dst	 Pointer to the destination
 * \param[in] 	weightStep 	 First input stride (distance in pixel units between vertically adjacent pixels)
 * \param[in] 	imageStep 	 Output stride (distance in pixel units between vertically adjacent pixels)
 * \param[in] 	u32Rows 	 Number of rows
 * \param[in] 	u32Cols 	 Number of columns
 *
 *
 *  \b Return \b null \n
 *  None
 *
 *	\b Restrictions \n
 *  1. p_u16Src must be padded by one pixel on each side \ref padding "- padding".
 *  2. Source buffer size must be weightStep*(u32Rows+2), and weightStep should be bigger than (u32Cols+2).
 *
 *
 *
 *
 */


// 0.23 cycle/pixel

void Max3x3AndBilinear ( uint8_t *p_u8Weight, 		//<<! [in] 0-255 scale tab.
							 uint16_t *p_u16ImageL_S,	//<<! [in] long and short image[block32x64]
							 uint16_t *p_u16PrevThumb,	//<<! [in] previous frame thumb image for WDR scale.
							 int32_t weightStep, 		//<<! [in] weight stride which add padding.
							 int32_t imageStep, 		//<<! [in] 16 align
							 int32_t u32Rows, 			//<<! [in] 
							 int32_t u32Cols,			//<<! [in]  
						#if HDR_DEBUG_ENABLE
							 uint16_t *p_u16Dst,
							 uint8_t  *p_u8FilterW,
							 int xPos,
							 int yPos)
						#else
							 uint16_t *p_u16Dst)		//<<! [out] HDR out 16bit,have not do WDR.
						#endif
{
#ifdef __XM4__
	PROFILER_START(HDR_BLOCK_H, HDR_BLOCK_W);
#endif	
	int32_t   row, col;
	uint8_t*  p_tab  	= p_u8Weight;
	uint16_t* p_imgL 	= p_u16ImageL_S;
	uint16_t* p_imgS 	= p_u16ImageL_S + HDR_BLOCK_H*HDR_BLOCK_W;
	uint16_t* pHDRout  	= p_u16Dst;
#if HDR_DEBUG_ENABLE	
	uint8_t* pWeight  	= p_u8FilterW;
#endif
	short offset[16] 	= {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};
	short16 voffset 	= *(short16*)offset;
	
	uint8_t cfg_fir[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
							     32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
	uint8_t cfg_snd[32] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
							 	 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};
	uchar32 vcfg_fir 	= *(uchar32*)cfg_fir;
	uchar32 vcfg_snd 	= *(uchar32*)cfg_snd;
	uchar32 v0a, v0b, v0c, vdummy;
	uchar32 v1a, v1b, v1c;
	uchar32 v2a, v2b, v2c;
	uchar32 u1,oneSubu1,splitBiEven,splitBiOdd[HDR_BLOCK_H];// cache 32 line bi factor.
	ushort16 imgL,imgS,hdrOut;
	uint32_t vprMask;
	uint32_t vprRightMask;

	vprMask = 0xFFFFFFFF;
	vprRightMask = 0xFFFFFFFF;
	if ((u32Cols & 31) != 0)
		vprRightMask = ((1 << (u32Cols & 31)) - 1);

	for(col = 0; col < u32Cols; col += 16) 
	{
		if (u32Cols - col < 32)
			vprMask = vprRightMask;
		
		if (0 == (col&31))
		{
			/* load the weight */
			vldov((uchar32*)(p_tab), v0a, v0b, v0c, vdummy);
			p_tab 	+= weightStep;
			v0a 	 = vmax(v0a, v0b, v0c);
			vldov((uchar32*)(p_tab), v1a, v1b, v1c, vdummy);
			p_tab 	+= weightStep;
			v1a 	 = vmax(v1a, v1b, v1c);
			vldov((uchar32*)(p_tab), v2a, v2b, v2c, vdummy);
			p_tab 	+= weightStep;
		}

		imgL 		 = vpld(p_imgL,voffset);  
		imgS 		 = vpld(p_imgS,voffset);  
		p_imgL 		+= imageStep;
		p_imgS 		+= imageStep;
		
		for(row = 0; row < u32Rows; row++) 
		{
			if (0 == (col&31))
			{
				v2a = vmax(v2a, v2b, v2c);
				u1  = vmax(v0a, v1a, v2a);
				//D_adj = ordfilt2(D_scale, 9, ones(3,3));	
				oneSubu1 		= (uchar32)vsub((unsigned char)255,u1);
				splitBiEven		= (uchar32)vperm(oneSubu1,u1,vcfg_fir); // low for long, high for short.
				splitBiOdd[row]	= (uchar32)vperm(oneSubu1,u1,vcfg_snd); // low for long, high for short.
			}
			//out = (l_ref_image.*(normlizeValue-D_adj)+s_ref_image.*D_adj)/normlizeValue;
			hdrOut = (ushort16)vmac3(splitsrc, psl, imgL, imgS, 0 == (col&31) ? splitBiEven : splitBiOdd[row], (uint16) 128, (unsigned char)8); 


			v0a = v1a;
			v0b = v1b;
			v0c = v1c;
			v1a = v2a;
			v1a = v2a;
			v1a = v2a;
			vldov((uchar32*)(p_tab), v2a, v2b, v2c, vdummy);
			p_tab += weightStep;


			imgL = vpld(p_imgL,voffset);  
			imgS = vpld(p_imgS,voffset);  
			p_imgL += imageStep;
			p_imgS += imageStep;
			
		#if CONNECT_WDR
			// do wdr by prev thumb image to bilinear hdrout.
			;
		#endif
			vst(hdrOut, (ushort16*)pHDRout, vprMask);

		
		#if HDR_DEBUG_ENABLE
			if (0 == (col&31))
				vst(u1, (uchar32*)pWeight, vprMask);
			pWeight += imageStep;
		#endif
		
			pHDRout+=imageStep;
		}
		if (0 == ((col+16)&31))
			p_tab  = p_u8Weight + col*2;		 
	#if HDR_DEBUG_ENABLE
		if (0 == ((col+16)&31))
			pWeight = p_u8FilterW + col*2;;
	#endif	
		p_imgL 	= p_u16ImageL_S 	+ col + 16;			 
		p_imgS 	= p_u16ImageL_S 	+ col + 16 + HDR_BLOCK_H*HDR_BLOCK_W;	 
		pHDRout = p_u16Dst 			+ col + 16;		
	}
#ifdef __XM4__
	PROFILER_END();
#endif	
	//GET_FUNCTION_NAME(ceva::g_p_function_name);
}


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
      

	uint16_t OffsetGap2[16]  = {0, 2, 4, 6, 8,10,12,14,
								16,18,20,22,24,26,28,30};
	short16 vOffsetGap2		 = *(short16*)(&OffsetGap2);	

	uint16_t OffsetGap4[16]  = {0, 4, 8, 12, 16,20,24,28,
								32,36,40, 44, 48,52,56,60};
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
	uint8_t cfg_adj_red1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_red2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,16+3,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_red3[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uchar32 vcfgAdjRed1= *(uchar32*)(&cfg_adj_red1);
	uchar32 vcfgAdjRed2= *(uchar32*)(&cfg_adj_red2);
	uchar32 vcfgAdjRedPack= *(uchar32*)(&cfg_adj_red3);

	
	uint8_t cfg_adj_blu1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_blu2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjBlu1= *(uchar32*)(&cfg_adj_blu1);
	uchar32 vcfgAdjBlu2= *(uchar32*)(&cfg_adj_blu2);
	uchar32	vcfgAdjBluePack= *(uchar32*)(&cfg_adj_blu1);
		
	uint8_t cfg_adj_gre1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_adj_gre2[32]  	= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjGre1= *(uchar32*)(&cfg_adj_gre1);
	uchar32 vcfgAdjGre2= *(uchar32*)(&cfg_adj_gre2);

	uint8_t cfg_pack0[32]  	= {	0, 16, 1, 17, 2, 18, 3, 19,
										4, 20, 5, 21, 6, 22, 7, 23,
									    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uint8_t cfg_pack1[32]  	= { 8,  24, 9,  25, 10, 26, 11, 27, 
										12, 28, 13, 29, 14, 30, 15, 31,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uchar32	vcfgPack0	= *(uchar32*)(&cfg_pack0);
	uchar32	vcfgPack1	= *(uchar32*)(&cfg_pack1);



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
			vRL0Long		= vselect(vR2packed, vRL0best, R_B_LONG_PATTERN);
			//PRINT_CEVA_VRF("vRL0Long", vRL0Long, stderr);
			vRL0Short		= vselect(vR2packed, vRL0best, R_B_SHORT_PATTERN);
			vRL0Short		= (ushort16)vshiftl(vRL0Short, 3);// times
			//PRINT_CEVA_VRF("vRL0Short", vRL0Short, stderr);


			vB3packed	= vB3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vBL0best	= (ushort16)vshiftr(vBL0best, (unsigned char)1);// times// interpoaltion		
			vBL0Long		= vselect(vB3packed, vBL0best, R_B_LONG_PATTERN);
			//PRINT_CEVA_VRF("vBL0Long", vBL0Long, stderr);
			vBL0Short		= vselect(vB3packed, vBL0best, R_B_SHORT_PATTERN);
			vBL0Short		= (ushort16)vshiftl(vBL0Short, 3);// times
			//PRINT_CEVA_VRF("vBL0Short", vBL0Short, stderr);


			vG2packed	= vG2offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L0best	= (ushort16)vshiftr(vG0L0best, (unsigned char)1);// times// interpoaltion		
			vG0L0Long		= vselect(vG2packed, vG0L0best, G_LONG_PATTERN);
			//PRINT_CEVA_VRF("vG0L0Long", vG0L0Long, stderr);
			vG0L0Short	= vselect(vG2packed, vG0L0best, G_SHORT_PATTERN);
			vG0L0Short	= (ushort16)vshiftl(vG0L0Short, 3);// times
			//PRINT_CEVA_VRF("vG0L0Short", vG0L0Short, stderr);

			vG3packed	= vG3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L0best	= (ushort16)vshiftr(vG1L0best, (unsigned char)1);// times// interpoaltion		
			vG1L0Long		= vselect(vG3packed, vG1L0best, G_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vG1L0Long", vG1L0Long, stderr);
			vG1L0Short	= vselect(vG3packed, vG1L0best, G_LONG_PATTERN);
			vG1L0Short	= (ushort16)vshiftl(vG1L0Short, 3);// times
			//PRINT_CEVA_VRF("vG1L0Short", vG1L0Short, stderr);

			// line 1 GRBG
			vR4packed	= (ushort16)vperm(vR4,vGR4,vcfgAdjRedPack); // orignal
			vRL1best	= (ushort16)vshiftr(vRL1best, (unsigned char)1);// times// interpoaltion		
			vRL1Long		= vselect(vR4packed, vRL1best, R_B_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vRL1Long", vRL1Long, stderr);
			vRL1Short		= vselect(vR4packed, vRL1best, R_B_LONG_PATTERN);
			vRL1Short		= (ushort16)vshiftl(vRL1Short, 3);// times
			//PRINT_CEVA_VRF("vRL1Short", vRL1Short, stderr);


			vB5packed	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
			vBL1best	= (ushort16)vshiftr(vBL1best, (unsigned char)1);// times// interpoaltion		
			vBL1Long		= vselect(vB5packed, vBL1best, R_B_SHORT_PATTERN );
			//PRINT_CEVA_VRF("vBL1Long", vBL1Long, stderr);
			vBL1Short		= vselect(vB5packed, vBL1best, R_B_LONG_PATTERN);
			vBL1Short		= (ushort16)vshiftl(vBL1Short, 3);// times
			//PRINT_CEVA_VRF("vBL1Short", vBL1Short, stderr);


			vG4packed	= vG4offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L1best	= (ushort16)vshiftr(vG0L1best, (unsigned char)1);// times// interpoaltion		
			vG0L1Long		= vselect(vG4packed, vG0L1best, G_LONG_PATTERN);
			//PRINT_CEVA_VRF("vG0L1Long", vG0L1Long, stderr);
			vG0L1Short	= vselect(vG4packed, vG0L1best, G_SHORT_PATTERN);
			vG0L1Short	= (ushort16)vshiftl(vG0L1Short, 3);// times
			//PRINT_CEVA_VRF("vG0L1Short", vG0L1Short, stderr);

			vG5packed	= vG5offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L1best	= (ushort16)vshiftr(vG1L1best, (unsigned char)1);// times// interpoaltion		
			vG1L1Long		= vselect(vG5packed, vG1L1best, G_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vG1L1Long", vG1L1Long, stderr);
			vG1L1Short	= vselect(vG5packed, vG1L0best, G_LONG_PATTERN);
			vG1L1Short	= (ushort16)vshiftl(vG1L1Short, 3);// times
			//PRINT_CEVA_VRF("vG1L1Short", vG1L1Short, stderr);
			// data arrange as scatter, store out to packed bayer.
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

		#if 1//CONNECT_LUT
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

			
		#endif

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

