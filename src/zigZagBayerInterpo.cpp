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



/**
 * 
 * \name residualLUT,read long and short image to get residual do LUT store out.
 * \param[in] p_u16Long 	 Pointer to the long source 
 * \param[in] p_u16Short	 Pointer to the short source 	
 * \param[out]p_u16Weight 	 Pointer to the destination
 * \param[in] s32Step 	 input stride (distance in pixel units between vertically adjacent pixels)
 * \param[in] u32Rows 	 Number of rows
 * \param[in] u32Cols 	 Number of columns
 *
 *
 *  \b Return \b value \n
 *  None
 *
 *	\b Restrictions \n
 *  1. p_u16Src must be padded by one pixel on each side \ref padding "- padding".
 *  2. Source buffer size must be s32SrcStep*(u32Rows+2), and s32SrcStep should be bigger than (u32Cols+2).
 *
 *
 *
 *
 */


// 0.23 cycle/pixel

void residualLUT(RK_U16 *p_u16Long, 	//<<! [in] long time image.
					RK_U16 *p_u16Short, //<<! [in] short time image.
					RK_U16 *p_u16Tab, 	//<<! [in] table
					RK_U16 *p_u16Weight,//<<! [out] bilinear weight
					int stride, 		//<<! [in] residual scale factor
					RK_U16  normValue, 	//<<! [in] refValue
					uint u32Cols)
{
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v14,v15;
	RK_U16* pLong 	= p_u16Long;
	RK_U16* pShort 	= p_u16Short;	
	
	v0 = *(ushort16*)(pLong); 	
	v1 = *(ushort16*)(pLong+16); 	
	v2 = *(ushort16*)(pLong+1*stride); 	
	v3 = *(ushort16*)(pLong+1*stride+16); 	
	v4 = *(ushort16*)(pLong+2*stride); 	
	v5 = *(ushort16*)(pLong+2*stride+16);
	v6 = *(ushort16*)(pLong+3*stride); 	
	v7 = *(ushort16*)(pLong+3*stride+16);

	v8  = *(ushort16*)(pShort); 	
	v9  = *(ushort16*)(pShort+16); 	
	v10 = *(ushort16*)(pShort+1*stride); 	
	v11 = *(ushort16*)(pShort+1*stride+16); 	
	v12 = *(ushort16*)(pShort+2*stride); 	
	v13 = *(ushort16*)(pShort+2*stride+16);
	v14 = *(ushort16*)(pShort+3*stride); 	
	v15 = *(ushort16*)(pShort+3*stride+16);
	
	// ------------------------------------------
	// use difference to LUT
	// ------------------------------------------
	v0	= vabssub(v0, v8);
	v1	= vabssub(v1, v9);
	v2	= vabssub(v2, v10);
	v3	= vabssub(v3, v11);

	v4	= vabssub(v4, v12);
	v5	= vabssub(v5, v13);
	v6	= vabssub(v6, v14);
	v7	= vabssub(v7, v15);

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

	// LUT
	
	v0 = vpld(p_u16Tab, (short16)v0);
	v1 = vpld(p_u16Tab, (short16)v1);
	v2 = vpld(p_u16Tab, (short16)v2);
	v3 = vpld(p_u16Tab, (short16)v3);

	v4 = vpld(p_u16Tab, (short16)v4);
	v5 = vpld(p_u16Tab, (short16)v5);
	v6 = vpld(p_u16Tab, (short16)v6);
	v7 = vpld(p_u16Tab, (short16)v7);		

#if 1
	// store the weight for 3x3 max filter
	vst(v0, p_u16Weight, 				0xffff);
	vst(v1, p_u16Weight+16,				0xffff);
	vst(v2, p_u16Weight+1*stride, 		0xffff);
	vst(v3, p_u16Weight+1*stride+16, 	0xffff);

	vst(v4, p_u16Weight+2*stride, 		0xffff);
	vst(v5, p_u16Weight+2*stride+16, 	0xffff);
	vst(v6, p_u16Weight+3*stride, 		0xffff);
	vst(v7, p_u16Weight+3*stride+16,	0xffff);
#endif

}
	
/**
 * 
 * \name  do max3x3 filter for the table value.
 * \param[in] p_u16Src 	 Pointer to the first source 
 * \param[out] p_u16Dst	 Pointer to the destination
 * \param[in] s32SrcStep 	 First input stride (distance in pixel units between vertically adjacent pixels)
 * \param[in] s32DstStep 	 Output stride (distance in pixel units between vertically adjacent pixels)
 * \param[in] u32Rows 	 Number of rows
 * \param[in] u32Cols 	 Number of columns
 *
 *
 *  \b Return \b value \n
 *  None
 *
 *	\b Restrictions \n
 *  1. p_u16Src must be padded by one pixel on each side \ref padding "- padding".
 *  2. Source buffer size must be s32SrcStep*(u32Rows+2), and s32SrcStep should be bigger than (u32Cols+2).
 *
 *
 *
 *
 */


// 0.23 cycle/pixel

void Max3x3AndBilinear (RK_U8  *p_u8Weight, 	//<<! [in] 0-255 scale tab.
							 RK_U16 *p_u16ImageL_S,	//<<! [in] long and short image
							 int s32SrcStep, 
							 int s32DstStep, 
							 int u32Rows, 
							 int u32Cols,
							 RK_U16 *p_u16Dst)
{
	
	uint row, col;
	RK_U8* 	p_tab  = p_u8Weight;
	RK_U16* p_imgL = p_u16ImageL_S;
	RK_U16* p_imgS = p_u16ImageL_S + 32*64;
	RK_U16* pHDRout  = p_u16Dst;
	short offset[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15};
	short16 voffset = *(short16*)offset;
	
	unsigned char cfg_fir[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
							     32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47};
	unsigned char cfg_snd[32] = {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
							 	 48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63};
	uchar32 vcfg_fir = *(uchar32*)cfg_fir;
	uchar32 vcfg_snd = *(uchar32*)cfg_snd;
	uchar32 v0a, v0b, v0c, vdummy;
	uchar32 v1a, v1b, v1c;
	uchar32 v2a, v2b, v2c;
	uchar32 u1,oneSubu1,splitBiEven,splitBiOdd;
	ushort16 imgL,imgS,hdrOut;
	uint vprMask;
	uint vprRightMask;

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
			p_tab += s32SrcStep;
			v0a = vmax(v0a, v0b, v0c);
			vldov((uchar32*)(p_tab), v1a, v1b, v1c, vdummy);
			p_tab += s32SrcStep;
			v1a = vmax(v1a, v1b, v1c);
			vldov((uchar32*)(p_tab), v2a, v2b, v2c, vdummy);
			p_tab += s32SrcStep;
		}
		/* load the long short image which store by even and odd */
		//vldchk(p_u16ImageL_S, imgL, imgS);  
		imgL = vpld(p_imgL,voffset);  
		imgS = vpld(p_imgS,voffset);  
		p_imgL += s32DstStep;
		p_imgS += s32DstStep;
		
		for(row = 0; row < u32Rows; row++) 
		{
			if (0 == (col&31))
			{
				v2a = vmax(v2a, v2b, v2c);
				u1  = vmax(v0a, v1a, v2a);

				//D_adj = ordfilt2(D_scale, 9, ones(3,3));	
				//out = (l_ref_image.*(normlizeValue-D_adj)+s_ref_image.*D_adj)/normlizeValue;
				oneSubu1 		= (uchar32)vsub((unsigned char)255,u1);
				splitBiEven		= (uchar32)vperm(oneSubu1,u1,vcfg_fir); // low for long, high for short.
				splitBiOdd		= (uchar32)vperm(oneSubu1,u1,vcfg_snd); // low for long, high for short.
			}
			hdrOut 			= (ushort16)vmac3(splitsrc, psl, imgL, imgS, 0 == (col&31) ? splitBiEven : splitBiOdd, (uint16) 128, (unsigned char)8); 

			v0a = v1a;
			v0b = v1b;
			v0c = v1c;
			v1a = v2a;
			v1a = v2a;
			v1a = v2a;
			vldov((uchar32*)(p_tab), v2a, v2b, v2c, vdummy);
			p_tab += s32SrcStep;

			/* load the long short image which store by even and odd */
			//vldchk(p_u16ImageL_S, imgL, imgS);  
			imgL = vpld(p_imgL,voffset);  
			imgS = vpld(p_imgS,voffset);  
			p_imgL += s32DstStep;
			p_imgS += s32DstStep;
			
		#if 1
			// store 32 result pixels
			vst(hdrOut, (ushort16*)pHDRout, vprMask);
		#else
			// WDR, read prev light to bilinear the out.
		
		#endif
			pHDRout+=s32DstStep;
		}
		if (0 == ((col+16)&31))
			p_tab  = p_u8Weight + col*2;		 
		
		p_imgL = p_u16ImageL_S 	+ col + 16;			 
		p_imgS = p_u16ImageL_S 	+ 32*64 + col + 16;;	 
		pHDRout = p_u16Dst +  col + 16;		
	}
	//GET_FUNCTION_NAME(ceva::g_p_function_name);
}

// 4x32 block process
void zigzagDebayer(	RK_U16 *p_u16Src, 
						RK_U8  *p_u16Tab, 
						RK_U16 blockW,
						RK_U16 blockH,
						RK_U16 stride,
						RK_U16 normValue,
						RK_U16 *buff,	//<<! [out] 32x64 short16  long and short
						RK_U8  *scale) 	//<<! [out] 32x64 short16  table
						
{
#ifdef __XM4__
	PROFILER_START(32, 64);
#endif
	unsigned short   i,j,SecMask;
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
	//static unsigned short buff[2*4*32];
	unsigned short* pL0G0	= buff 									;                              
	unsigned short* pL0Red 	= buff+1									;                              
	unsigned short* pL0Blu	= buff	 + HDR_BLOCK_W 	;                              
	unsigned short* pL0G1	= buff+1 + HDR_BLOCK_W 	;                              
	unsigned short* pL1G0	= buff 	 + 2*HDR_BLOCK_W ;                              
	unsigned short* pL1Red 	= buff+1 + 2*HDR_BLOCK_W ;                              
	unsigned short* pL1Blu 	= buff   + 3*HDR_BLOCK_W ;                              
	unsigned short* pL1G1	= buff+1 + 3*HDR_BLOCK_W ;                              
                                                                                 
	unsigned short* pL0G0_s	 = buff 				  + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL0Red_s = buff+1				  + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL0Blu_s = buff	  + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL0G1_s	 = buff+1 + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL1G0_s	 = buff   + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL1Red_s = buff+1 + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL1Blu_s = buff   + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
	unsigned short* pL1G1_s	 = buff+1 + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   

	unsigned char* pScaleL0G0	= scale 									;                              
	unsigned char* pScaleL0Red  = scale+1									;                              
	unsigned char* pScaleL0Blu	= scale	  + HDR_FILTER_W 	;                              
	unsigned char* pScaleL0G1	= scale+1 + HDR_FILTER_W 	;                              
	unsigned char* pScaleL1G0	= scale   + 2*HDR_FILTER_W ;                              
	unsigned char* pScaleL1Red  = scale+1 + 2*HDR_FILTER_W ;                              
	unsigned char* pScaleL1Blu  = scale   + 3*HDR_FILTER_W ;                              
	unsigned char* pScaleL1G1	= scale+1 + 3*HDR_FILTER_W ;                              
      

	unsigned short  OffsetGap2[16] 		= {	0,	2, 4, 6, 8,10,12,14,
											16,18,20,22,24,26,28,30};
	short16 vOffsetGap2		 = *(short16*)(&OffsetGap2);	

	unsigned short  OffsetGap4[16] 		= {	0,	4, 8, 12, 16,20,24,28,
											32,36,40, 44, 48,52,56,60};
	short16 vOffsetGap4		 = *(short16*)(&OffsetGap4);	

	
	unsigned short* pInLine0 = p_u16Src ;				// + 3,Load R 
	unsigned short* pInLine1 = p_u16Src + stride 	;	// + 1 load G & B
	unsigned short* pInLine2 = p_u16Src + 2*stride 	;	// + 1 LOAD R & G
	unsigned short* pInLine3 = p_u16Src + 3*stride;		//     load B & G
	unsigned short* pInLine4 = p_u16Src + 4*stride	;	// + 2 LOAD G & R
	unsigned short* pInLine5 = p_u16Src + 5*stride	;	// + 2 LOAD B 
	unsigned short* pInLine6 = p_u16Src + 6*stride	;	// + 2 LOAD B 
	unsigned short* pInLine7 = p_u16Src + 7*stride	;	// + 2 LOAD B 
	// g r
	// b g
	// perm cfg:
	// g0 is same with b
	// g1 is same with r
	unsigned char cfg_adj_red1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_red2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,16+3,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_red3[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uchar32 vcfgAdjRed1= *(uchar32*)(&cfg_adj_red1);
	uchar32 vcfgAdjRed2= *(uchar32*)(&cfg_adj_red2);
	uchar32 vcfgAdjRedPack= *(uchar32*)(&cfg_adj_red3);

	
	unsigned char cfg_adj_blu1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_blu2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjBlu1= *(uchar32*)(&cfg_adj_blu1);
	uchar32 vcfgAdjBlu2= *(uchar32*)(&cfg_adj_blu2);
	uchar32	vcfgAdjBluePack= *(uchar32*)(&cfg_adj_blu1);
		
	unsigned char cfg_adj_gre1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_gre2[32]  	= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjGre1= *(uchar32*)(&cfg_adj_gre1);
	uchar32 vcfgAdjGre2= *(uchar32*)(&cfg_adj_gre2);

	unsigned char cfg_pack0[32]  	= {	0, 16, 1, 17, 2, 18, 3, 19,
										4, 20, 5, 21, 6, 22, 7, 23,
									    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_pack1[32]  	= { 8,  24, 9,  25, 10, 26, 11, 27, 
										12, 28, 13, 29, 14, 30, 15, 31,
										0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	uchar32	vcfgPack0	= *(uchar32*)(&cfg_pack0);
	uchar32	vcfgPack1	= *(uchar32*)(&cfg_pack1);
//#define G_RB_ARRANGE 0x55555555
//#define RB_G_ARRANGE 0xaaaaaaaa

#if 0
	volatile register unsigned int r0 __asm__("r0");// r0 bind for asm
	volatile register unsigned int r1 __asm__("r1");// r1 bind for asm
	r0 = 0x310;
	_dsp_asm("SC0.mov #784, r0.i");
	_dsp_asm("SC0.in{cpm} (r0.ui).i, r1.i");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	g_start = r1;
#endif


	//  6 x 3 = 15 reg for one pixel interpolation.		
	for ( i = 0 ; i < blockW/32 ; i++ )
	{
		vldchk(pInLine0, vG0, vR0);   vGR0 = *(ushort16*)(pInLine0+32);   //pInLine0 += 8*stride;
		vldchk(pInLine1, vB1, vG1);   vBG1 = *(ushort16*)(pInLine1+32);   //pInLine1 += 8*stride;
		vldchk(pInLine2, vG2, vR2);   vGR2 = *(ushort16*)(pInLine2+32);   //pInLine2 += 8*stride;
		vldchk(pInLine3, vB3, vG3);   vBG3 = *(ushort16*)(pInLine3+32);   //pInLine3 += 8*stride;
		// subtract blacklevel
		vG0	= vsubsat(vG0, (unsigned short)64);
		vR0	= vsubsat(vR0, (unsigned short)64);
		vB1	= vsubsat(vB1, (unsigned short)64);
		vG1	= vsubsat(vG1, (unsigned short)64);

		vG2	= vsubsat(vG2, (unsigned short)64);
		vR2	= vsubsat(vR2, (unsigned short)64);
		vB3	= vsubsat(vB3, (unsigned short)64);
		vG3	= vsubsat(vG3, (unsigned short)64);

		vGR0	= vsubsat(vGR0, (unsigned short)64);
		vBG1	= vsubsat(vBG1, (unsigned short)64);
		vGR2	= vsubsat(vGR2, (unsigned short)64);
		vBG3	= vsubsat(vBG3, (unsigned short)64);


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

		vR2packed	= (ushort16)vperm(vR2,vGR2,vcfgAdjRedPack); // orignal

		for ( j = 0 ; j < blockH/4; j++ )
		{
			vldchk(pInLine4, vG4, vR4);   vGR4 = *(ushort16*)(pInLine4+32);   pInLine4 += 4*stride;
			vldchk(pInLine5, vB5, vG5);   vBG5 = *(ushort16*)(pInLine5+32);   pInLine5 += 4*stride;
			vldchk(pInLine6, vG6, vR6);   vGR6 = *(ushort16*)(pInLine6+32);   pInLine6 += 4*stride;
			vldchk(pInLine7, vB7, vG7);   vBG7 = *(ushort16*)(pInLine7+32);   pInLine7 += 4*stride;

			vG4	= vsubsat(vG4, (unsigned short)64);
			vR4	= vsubsat(vR4, (unsigned short)64);
			vB5	= vsubsat(vB5, (unsigned short)64);
			vG5	= vsubsat(vG5, (unsigned short)64);
			
			vG6	= vsubsat(vG6, (unsigned short)64);
			vR6	= vsubsat(vR6, (unsigned short)64);
			vB7	= vsubsat(vB7, (unsigned short)64);
			vG7	= vsubsat(vG7, (unsigned short)64);

			vGR4	= vsubsat(vGR4, (unsigned short)64);
			vBG5	= vsubsat(vBG5, (unsigned short)64);
			vGR6	= vsubsat(vGR6, (unsigned short)64);
			vBG7	= vsubsat(vBG7, (unsigned short)64);
			
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
			vRL0best	= (ushort16)vshiftr(vRL0best, (uchar)1);// times// interpoaltion		
			vRL0Long		= vselect(vR2packed, vRL0best, R_B_LONG_PATTERN);
			//PRINT_CEVA_VRF("vRL0Long", vRL0Long, stderr);
			vRL0Short		= vselect(vR2packed, vRL0best, R_B_SHORT_PATTERN);
			vRL0Short		= (ushort16)vshiftl(vRL0Short, 3);// times
			//PRINT_CEVA_VRF("vRL0Short", vRL0Short, stderr);


			vB3packed	= vB3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vBL0best	= (ushort16)vshiftr(vBL0best, (uchar)1);// times// interpoaltion		
			vBL0Long		= vselect(vB3packed, vBL0best, R_B_LONG_PATTERN);
			//PRINT_CEVA_VRF("vBL0Long", vBL0Long, stderr);
			vBL0Short		= vselect(vB3packed, vBL0best, R_B_SHORT_PATTERN);
			vBL0Short		= (ushort16)vshiftl(vBL0Short, 3);// times
			//PRINT_CEVA_VRF("vBL0Short", vBL0Short, stderr);


			vG2packed	= vG2offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L0best	= (ushort16)vshiftr(vG0L0best, (uchar)1);// times// interpoaltion		
			vG0L0Long		= vselect(vG2packed, vG0L0best, G_LONG_PATTERN);
			//PRINT_CEVA_VRF("vG0L0Long", vG0L0Long, stderr);
			vG0L0Short	= vselect(vG2packed, vG0L0best, G_SHORT_PATTERN);
			vG0L0Short	= (ushort16)vshiftl(vG0L0Short, 3);// times
			//PRINT_CEVA_VRF("vG0L0Short", vG0L0Short, stderr);

			vG3packed	= vG3offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L0best	= (ushort16)vshiftr(vG1L0best, (uchar)1);// times// interpoaltion		
			vG1L0Long		= vselect(vG3packed, vG1L0best, G_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vG1L0Long", vG1L0Long, stderr);
			vG1L0Short	= vselect(vG3packed, vG1L0best, G_LONG_PATTERN);
			vG1L0Short	= (ushort16)vshiftl(vG1L0Short, 3);// times
			//PRINT_CEVA_VRF("vG1L0Short", vG1L0Short, stderr);

			// line 1 GRBG
			vR4packed	= (ushort16)vperm(vR4,vGR4,vcfgAdjRedPack); // orignal
			vRL1best	= (ushort16)vshiftr(vRL1best, (uchar)1);// times// interpoaltion		
			vRL1Long		= vselect(vR4packed, vRL1best, R_B_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vRL1Long", vRL1Long, stderr);
			vRL1Short		= vselect(vR4packed, vRL1best, R_B_LONG_PATTERN);
			vRL1Short		= (ushort16)vshiftl(vRL1Short, 3);// times
			//PRINT_CEVA_VRF("vRL1Short", vRL1Short, stderr);


			vB5packed	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
			vBL1best	= (ushort16)vshiftr(vBL1best, (uchar)1);// times// interpoaltion		
			vBL1Long		= vselect(vB5packed, vBL1best, R_B_SHORT_PATTERN );
			//PRINT_CEVA_VRF("vBL1Long", vBL1Long, stderr);
			vBL1Short		= vselect(vB5packed, vBL1best, R_B_LONG_PATTERN);
			vBL1Short		= (ushort16)vshiftl(vBL1Short, 3);// times
			//PRINT_CEVA_VRF("vBL1Short", vBL1Short, stderr);


			vG4packed	= vG4offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG0L1best	= (ushort16)vshiftr(vG0L1best, (uchar)1);// times// interpoaltion		
			vG0L1Long		= vselect(vG4packed, vG0L1best, G_LONG_PATTERN);
			//PRINT_CEVA_VRF("vG0L1Long", vG0L1Long, stderr);
			vG0L1Short	= vselect(vG4packed, vG0L1best, G_SHORT_PATTERN);
			vG0L1Short	= (ushort16)vshiftl(vG0L1Short, 3);// times
			//PRINT_CEVA_VRF("vG0L1Short", vG0L1Short, stderr);

			vG5packed	= vG5offset;//(ushort16)vperm(vB3,vBG3,vcfgAdjBlu1); // orignal
			vG1L1best	= (ushort16)vshiftr(vG1L1best, (uchar)1);// times// interpoaltion		
			vG1L1Long		= vselect(vG5packed, vG1L1best, G_SHORT_PATTERN);
			//PRINT_CEVA_VRF("vG1L1Long", vG1L1Long, stderr);
			vG1L1Short	= vselect(vG5packed, vG1L0best, G_LONG_PATTERN);
			vG1L1Short	= (ushort16)vshiftl(vG1L1Short, 3);// times
			//PRINT_CEVA_VRF("vG1L1Short", vG1L1Short, stderr);
#if 1
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
		#else
			// even / odd store long and short image.
			vpst(vG0L0Long, pL0G0,  vOffsetGap4);
			vpst(vG0L0Short,pL0Red, vOffsetGap4);
			vpst(vBL0Long,  pL0Blu, vOffsetGap2);
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
		#endif

#if CONNECT_LUT
					
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

			// LUT
			
			vt0 = vpld(p_u16Tab, (short16)v0);
			vt1 = vpld(p_u16Tab, (short16)v1);
			vt2 = vpld(p_u16Tab, (short16)v2);
			vt3 = vpld(p_u16Tab, (short16)v3);

			vt4 = vpld(p_u16Tab, (short16)v4);
			vt5 = vpld(p_u16Tab, (short16)v5);
			vt6 = vpld(p_u16Tab, (short16)v6);
			vt7 = vpld(p_u16Tab, (short16)v7);		

			vpst(vt0, 	pScaleL0G0,  vOffsetGap2);
			vpst(vt1,	pScaleL0Red, vOffsetGap2);
			vpst(vt2, 	pScaleL0Blu, vOffsetGap2);
			vpst(vt3, 	pScaleL0G1,  vOffsetGap2);

			vpst(vt4, 	pScaleL1G0,  vOffsetGap2);
			vpst(vt5,	pScaleL1Red, vOffsetGap2);
			vpst(vt6, 	pScaleL1Blu, vOffsetGap2);
			vpst(vt7, 	pScaleL1G1,  vOffsetGap2);

			/*
			D_scale = max(D_scale, double(s_ref_image>0.9*normlizeValue)*normlizeValue );
			D_scale = min(D_scale, (1 - double(s_ref_image < (0.85*normlizeValue)/times))*normlizeValue);
			*/

			//D_adj = ordfilt2(D_scale, 9, ones(3,3));
			

			//out = (l_ref_image.*(normlizeValue-D_adj)+s_ref_image.*D_adj)/normlizeValue;
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

			pScaleL0G0   += 4*HDR_FILTER_W;	
			pScaleL0Red  += 4*HDR_FILTER_W;	
			pScaleL0Blu  += 4*HDR_FILTER_W;	
			pScaleL0G1   += 4*HDR_FILTER_W;	
			                                
			pScaleL1G0   += 4*HDR_FILTER_W;	
			pScaleL1Red  += 4*HDR_FILTER_W;	
			pScaleL1Blu  += 4*HDR_FILTER_W;	
			pScaleL1G1   += 4*HDR_FILTER_W;	

			
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
		pInLine0 = p_u16Src + 32 ;						// + 3,Load R 
		pInLine1 = p_u16Src + 32  + stride 		;	// + 1 load G & B
		pInLine2 = p_u16Src + 32  + 2*stride 	;	// + 1 LOAD R & G
		pInLine3 = p_u16Src + 32  + 3*stride	;	//     load B & G
		
		pInLine4 = p_u16Src + 32  + 4*stride	;	// + 2 LOAD G & R
		pInLine5 = p_u16Src + 32  + 5*stride	;	// + 2 LOAD B 
		pInLine6 = p_u16Src + 32  + 6*stride	;	// + 2 LOAD B 
		pInLine7 = p_u16Src + 32  + 7*stride	;	// + 2 LOAD B 



		pL0G0	= buff + 32									;                              
		pL0Red 	= buff + 32	+1									;                              
		pL0Blu	= buff + 32	   + HDR_BLOCK_W 	;                              
		pL0G1	= buff + 32	+1 + HDR_BLOCK_W 	;                              
		pL1G0	= buff + 32	   + 2*HDR_BLOCK_W ;                              
		pL1Red 	= buff+ 32	+1 + 2*HDR_BLOCK_W ;                              
		pL1Blu 	= buff+ 32	   + 3*HDR_BLOCK_W ;                              
		pL1G1	= buff+ 32	+1 + 3*HDR_BLOCK_W ;                              
		                                                               
		pL0G0_s	 = buff+ 32	 			 	   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0Red_s = buff+ 32	+1			 	   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0Blu_s = buff+ 32	   + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL0G1_s	 = buff+ 32	+1 + HDR_BLOCK_W   + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1G0_s	 = buff+ 32	   + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1Red_s = buff+ 32	+1 + 2*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1Blu_s = buff+ 32	   + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   
		pL1G1_s	 = buff+ 32	+1 + 3*HDR_BLOCK_W + HDR_BLOCK_H*HDR_BLOCK_W;   


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


void CCV_interpolation5x5(short *p_s16Src, 
	short *p_s16Dst, 
	int s32SrcStep,
	int s32DstStep,
	uint u32N, 
	uint u32M, 
	short *p_s16Kernel, 
	uchar u8RightShift ) 
{
	//PROFILER_START(u32N, u32M);
	//Calculate 32 pixels in 6 cycles (unroll 8).
	ushort   i,j;
	int   rnd;
	short16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13,v_coeff = *(short16*)p_s16Kernel;
	short16 vLeft_Hori,vRight_Veri,vBestTmp;

	short* p_in_s16;
	short* p_out_s16;
	int16 vacc0;
	int16 vacc1;
	short32 vacc2;
	uint16 vacc_left ,vacc_right, vacc_veri, vacc_hori ;
	unsigned short  SecRight ;
	unsigned char config_list[32] = {1,  2,  3,  4,  5,  6, 7,  8, 
								 9,  10, 11, 12, 13, 14,15, 16, 
								 17, 18, 19,  0,  0,  0, 0,  0,  
								 0,  0,  0,  0,  0,  0, 0,  0 };
	uchar32 vconfig0 = *(uchar32*)(&config_list[0]);
	uchar32 vconfig1 = *(uchar32*)(&config_list[1]);
	uchar32 vconfig2 = *(uchar32*)(&config_list[2]);
	uchar32 vconfig3 = *(uchar32*)(&config_list[3]);

#define G_RB_ARRANGE 0x55555555
#define RB_G_ARRANGE 0xaaaaaaaa

#if 0
	volatile register unsigned int r0 __asm__("r0");// r0 bind for asm
	volatile register unsigned int r1 __asm__("r1");// r1 bind for asm
	r0 = 0x310;
	_dsp_asm("SC0.mov #784, r0.i");
	_dsp_asm("SC0.in{cpm} (r0.ui).i, r1.i");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	_dsp_asm("nop");
	g_start = r1;
#endif
	uint u32OutLoop = (u32M + 15) >> 4; // 16 align 
	unsigned short vprRightMask, vprMask;
	vprRightMask 	= 0xffff;
	vprMask 		= 0xffff;

	if (u32OutLoop != u32M >> 4)
		vprRightMask = ((1 << (u32M & 15)) - 1) & 0xffff;

	rnd = 0;//(1<<(u8RightShift-1));
	int padding_v = 2;	
	for (i = 0; i <u32OutLoop; i++)
	{		
		p_in_s16  = (short *)&p_s16Src[-padding_v-padding_v*s32SrcStep + i*16];
		p_out_s16 = (short *)&p_s16Dst[i*16];
		
		if (i == u32OutLoop-1)
			vprMask = vprRightMask;
		// load data to 16short + 4short for width=16
		
		v0 = *(short16*)(p_in_s16); v5 = *(short16*)(p_in_s16+16);
		p_in_s16 += s32SrcStep;
		v1 = *(short16*)(p_in_s16);	v6 = *(short16*)(p_in_s16+16);
		p_in_s16 += s32SrcStep;
		v2 = *(short16*)(p_in_s16);	v7 = *(short16*)(p_in_s16+16);
		p_in_s16 += s32SrcStep;
		v3 = *(short16*)(p_in_s16);	v8 = *(short16*)(p_in_s16+16);
		p_in_s16 += s32SrcStep;
		v4 = *(short16*)(p_in_s16);	v9 = *(short16*)(p_in_s16+16);
		p_in_s16 += s32SrcStep;
		

		for (j = 0; j < u32N; ++j)	
		{
			vacc_left  = 0;
			vacc_right = 0;
			vacc_veri  = 0;
			vacc_hori  = 0;
			// ---------------------------------
			/* get 9 point from 25, compare left and right to get best Green interpolation */
			// ----
			//v0 do not need perm operation.
			v10 =  vperm(v1, v6,  vconfig0);// offset_hori+1
			v11 =  vperm(v2, v7,  vconfig1);// offset_hori+2
			v12 =  vperm(v3, v8,  vconfig2);// offset_hori+3
			v13 =  vperm(v4, v9,  vconfig3);// offset_hori+4

			vacc_left = vabssubacc(v0,  v11, vacc_left);
			vacc_left = vabssubacc(v13, v11, vacc_left);
			vacc_left = vabssubacc(v12, v10, vacc_left);
			vLeft_Hori = vadd(v12, v10);
			
			v10 =  vperm(v0, v5,  vconfig3);// offset_hori+4
			v11 =  vperm(v1, v6,  vconfig2);// offset_hori+3
			v12 =  vperm(v2, v7,  vconfig1);// offset_hori+2
			v13 =  vperm(v3, v8,  vconfig0);// offset_hori+1
			//v4 do not need perm operation.

			vacc_right = vabssubacc(v10,  v12, vacc_right);
			vacc_right = vabssubacc(v4,   v12, vacc_right);
			vacc_right = vabssubacc(v13,  v11, vacc_right);
			vRight_Veri = vadd(v13, v11);
			// compare left and right
			v10 = (short16) vacc_left;
			v11 = (short16) vacc_right;
			SecRight = vcmp(gt, v10, v11);
			vBestTmp =  vselect( vRight_Veri, vLeft_Hori, SecRight); // get the Green fushion.


			// ------------------------
			/* get 4 point from 25, compare veri and hori to get best R/B interpolation */
			/* VERI */
			v10 =  vperm(v0, v5,  vconfig1);// offset_hori+2
			v11 =  vperm(v1, v6,  vconfig1);// offset_hori+2

			v12 =  vperm(v3, v8,  vconfig1);// offset_hori+2
			v13 =  vperm(v4, v9,  vconfig1);// offset_hori+2

			vacc_veri = vabssubacc(v13, v10, vacc_veri);
			vacc_veri = vabssubacc(v12, v11, vacc_veri);
			vRight_Veri = vadd(v13, v10);

			/* HORI */
			// v2 do not need perm operation.
			v11 =  vperm(v2, v7,  vconfig0);// offset_hori+2

			v12 =  vperm(v2, v7,  vconfig2);// offset_hori+2
			v13 =  vperm(v2, v7,  vconfig3);// offset_hori+2

			vacc_hori = vabssubacc(v13, v2,  vacc_hori);
			vacc_hori = vabssubacc(v12, v11, vacc_hori);
			vLeft_Hori = vadd(v13, v2);

			// compare VERI and HORI
			v11 = (short16)vacc_hori ;
			v10 = (short16)vacc_veri ;
			SecRight = vcmp(gt, v11, v10); // MUST BE SAME WITH c MODEL.
			
			vLeft_Hori =  vselect(  vRight_Veri, vLeft_Hori, SecRight); // get the R.B fushion.

			
			// ----
			v0 = v1;  									v5 = v6;
			v1 = v2;									v6 = v7;
			v2 = v3; 									v7 = v8;
			v3 = v4; 									v8 = v9;
			v4 = *(short16*)(p_in_s16) ;				v9 = *(short16*)(p_in_s16+16);
			p_in_s16  += s32SrcStep;

			//TF = int16(repmat([1 0; 0 1], [size(in,1)/2, size(in,2)/2]));
			//f_image = fg.*TF + frb.*(1-TF);
			vLeft_Hori =  vselect(  vBestTmp, vLeft_Hori, (j&1) == 0 ? G_RB_ARRANGE : RB_G_ARRANGE); // get the R.B fushion.
			//vst(vRight_Veri , (short16*)p_out_s16, vprMask);
			vst(vLeft_Hori , (short16*)p_out_s16, vprMask);
			p_out_s16 += s32DstStep;

		}//m
	}//n	
	//PROFILER_END();
}
