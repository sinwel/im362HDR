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
**   
**
**	
**
** Copyright 2016, rockchip.
**
***************************************************************************/
#include "XM4_defines.h"
#include <vec-c.h>
#include "rk_typedef.h"
#include "rk_bayerwdr.h"
#if WIN32
#include <stdlib.h>
void writeFile(RK_U16 *data, int Num, char* FileName)
{
	FILE* fp = fopen(FileName,"w");
	for ( int i = 0 ; i < Num; i++ )
	{
		fprintf(fp,"%6d,", *data++);
		if ( (i+1)%256 == 0 )
		{
			fprintf(fp,"\n");
		}
	}

	fclose(fp);
}
#endif

#define CODE_SCATTER 1

// 4x32 block process
void zigzagDebayer(	RK_U16 *p_u16Src, 
						RK_U16 width,
						RK_U16 stride,
						RK_U16 *p_u16Dst	//<<! [out]
						) 
{
#ifdef __XM4__
	PROFILER_START(4, 32);
#endif
	unsigned short   i,j,SecMask;
	ushort16 vG0,vR0,vB1,vG1,vG2,vR2,vB3,vG3,vG4,vR4,vB5,vG5,vG6,vR6,vB7,vG7;
	ushort16 vGR0,vBG1,vGR2,vBG3,vGR4,vBG5,vGR6,vBG7;
	ushort16 v0,v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12,v13;
	
	ushort16 vR0offset,vR2offset,vR4offset,vR6offset;
	ushort16 vB1offset,vB3offset,vB5offset,vB7offset;
	ushort16 vG2offset,vG2offset_,vG4offset,vG4offset_;
	ushort16 vG1offset,vG3offset,vG3offset_;
	ushort16 vG6offset,vG6offset_,vG5offset,vG5offset_;	
	
	ushort16 vRL0best,vBL0best,vG0L0best,vG1L0best;
	ushort16 vRL1best,vBL1best,vG0L1best,vG1L1best;
	


	unsigned short* pInLine0 = p_u16Src ;				// + 3,Load R 
	unsigned short* pInLine1 = p_u16Src + stride 	;	// + 1 load G & B
	unsigned short* pInLine2 = p_u16Src + 2*stride 	;	// + 1 LOAD R & G
	unsigned short* pInLine3 = p_u16Src + 3*stride;		//     load B & G
	unsigned short* pInLine4 = p_u16Src + 4*stride	;	// + 2 LOAD G & R
	unsigned short* pInLine5 = p_u16Src + 5*stride	;	// + 2 LOAD B 
	unsigned short* pInLine6 = p_u16Src + 6*stride	;	// + 2 LOAD B 
	unsigned short* pInLine7 = p_u16Src + 7*stride	;	// + 2 LOAD B 

	unsigned char cfg_adj_red1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_red2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16+1,16+3,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjRed1= *(uchar32*)(&cfg_adj_red1);
	uchar32 vcfgAdjRed2= *(uchar32*)(&cfg_adj_red2);
	unsigned char cfg_adj_blu1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_blu2[32] 		= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjBlu1= *(uchar32*)(&cfg_adj_blu1);
	uchar32 vcfgAdjBlu2= *(uchar32*)(&cfg_adj_blu2);

	unsigned char cfg_adj_gre1[32] 		= {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	unsigned char cfg_adj_gre2[32]  	= {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,16+2,
											0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
	uchar32 vcfgAdjGre1= *(uchar32*)(&cfg_adj_gre1);
	uchar32 vcfgAdjGre2= *(uchar32*)(&cfg_adj_gre2);

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

	vldchk(pInLine0, vG0, vR0);   vGR0 = *(ushort16*)(pInLine0+32);   pInLine0 += 8*stride;
	vldchk(pInLine1, vB1, vG1);   vBG1 = *(ushort16*)(pInLine1+32);   pInLine1 += 8*stride;
	vldchk(pInLine2, vG2, vR2);   vGR2 = *(ushort16*)(pInLine2+32);   pInLine2 += 8*stride;
	vldchk(pInLine3, vB3, vG3);   vBG3 = *(ushort16*)(pInLine3+32);   pInLine3 += 8*stride;
	vldchk(pInLine4, vG4, vR4);   vGR4 = *(ushort16*)(pInLine4+32);   pInLine4 += 8*stride;
	vldchk(pInLine5, vB5, vG5);   vBG5 = *(ushort16*)(pInLine5+32);   pInLine5 += 8*stride;
	vldchk(pInLine6, vG6, vR6);   vGR6 = *(ushort16*)(pInLine6+32);   pInLine6 += 8*stride;
	vldchk(pInLine7, vB7, vG7);   vBG7 = *(ushort16*)(pInLine7+32);   pInLine7 += 8*stride;
	
	{
		// ========= line 0 ==============
		// -------------r-----------------
		// abs(R0-R4),abs(R2 - R2+4)
		// ----
		vR0offset	= (ushort16)vperm(vR0,vGR0,vcfgAdjRed1);
		vR4offset	= (ushort16)vperm(vR4,vGR4,vcfgAdjRed1);
		vR2offset	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed2);
	#if CODE_SCATTER
		v0			= vabssub(vR0offset, vR4offset);
		v1			= vabssub(vR2, vR2offset);
		v2			= (ushort16)vadd(vR0offset, vR4offset);
		v3			= (ushort16)vadd(vR2, vR2offset); 
		SecMask 	= vcmp(lt,v0,v1);
		vRL0best	= vselect(v2,v3,SecMask);
	#else
		SecMask 	= vcmp(lt,vabssub(vR0offset, vR4offset),vabssub(vR2, vR2offset));
		vRL0best 	= vselect( (ushort16)vadd(vR0offset, vR4offset), (ushort16)vadd(vR2, vR2offset), SecMask); 
	#endif

		PRINT_CEVA_VRF("vRL0best", vRL0best, stderr);

		// -------------b-----------------
		// abs(B1-B5),abs(B3 - B3+4)
		// ----
		vB1offset	= (ushort16)vperm(vB1,vBG1,vcfgAdjBlu1);
		vB5offset	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu1);
		vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu2);
	#if CODE_SCATTER
		v0			= vabssub(vB1offset, vB5offset);
		v1			= vabssub(vB3, vB3offset);
		v2			= (ushort16)vadd(vB1offset, vB5offset);
		v3			= (ushort16)vadd(vB3, vB3offset); 
		SecMask 	= vcmp(lt,v0,v1);
		vBL0best	= vselect(v2,v3,SecMask);
	#else
		
		SecMask 	= vcmp(lt,vabssub(vB1offset, vB5offset),vabssub(vB3, vB3offset));
		vBL0best 	= vselect( (ushort16)vadd(vB1offset, vB5offset), (ushort16)vadd(vB3, vB3offset), SecMask); 
	#endif
		PRINT_CEVA_VRF("vBL0best", vBL0best, stderr);
		// -------------G-----------------
		// abs(G1-G3),abs(G2 - G4)
		//  G1  G3 
		vG1offset	= (ushort16)vperm(vG1,vBG1,vcfgAdjRed1);
		vG3offset	= (ushort16)vperm(vG3,vBG3,vcfgAdjRed1);
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
		PRINT_CEVA_VRF("vG0L0best", vG0L0best, stderr);

		//  G2  G4  
		vG2offset	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre1);
		vG4offset	= (ushort16)vperm(vG4,vGR4,vcfgAdjGre1);
		vG2offset_	= (ushort16)vperm(vG2,vGR2,vcfgAdjGre2);
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
		PRINT_CEVA_VRF("vG1L0best", vG1L0best, stderr);
		// ========= line 1 ==============
		// -------------r-----------------
		// abs(R2-R6),abs(R4 - R4+4)
		// ----
		vR2offset	= (ushort16)vperm(vR2,vGR2,vcfgAdjRed1);
		vR6offset	= (ushort16)vperm(vR6,vGR6,vcfgAdjRed1);
		vR4offset	= (ushort16)vperm(vR4,vGR4,vcfgAdjRed2);
	#if CODE_SCATTER
		v0			= vabssub(vR2offset, vR6offset);
		v1			= vabssub(vR4, vR4offset);
		v2			= (ushort16)vadd(vR2offset, vR6offset);
		v3			= (ushort16)vadd(vR4, vR4offset); 
		SecMask 	= vcmp(lt,v0,v1);
		vRL1best	= vselect(v2,v3,SecMask);
	#else
		SecMask 	= vcmp(lt,vabssub(vR2offset, vR6offset),vabssub(vR4, vR4offset));
		vRL1best 	= vselect( (ushort16)vadd(vR2offset, vR6offset), (ushort16)vadd(vR4, vR4offset), SecMask); 
	#endif
		PRINT_CEVA_VRF("vRL1best", vRL1best, stderr);

		// -------------b-----------------
		// abs(B3-B7),abs(B5 - B5+4)
		// ----
		vB3offset	= (ushort16)vperm(vB3,vBG3,vcfgAdjBlu1);
		vB7offset	= (ushort16)vperm(vB7,vBG7,vcfgAdjBlu1);
		vB5offset	= (ushort16)vperm(vB5,vBG5,vcfgAdjBlu2);
	#if CODE_SCATTER
		v0			= vabssub(vB3offset, vB7offset);
		v1			= vabssub(vB5, vB5offset);
		v2			= (ushort16)vadd(vB3offset, vB7offset);
		v3			= (ushort16)vadd(vB5, vB5offset); 
		SecMask 	= vcmp(lt,v0,v1);
		vBL1best	= vselect(v2,v3,SecMask);
	#else
		SecMask 	= vcmp(lt,vabssub(vB3offset, vB7offset),vabssub(vB5, vB5offset));
		vBL1best 	= vselect( (ushort16)vadd(vB3offset, vB7offset), (ushort16)vadd(vB5, vB5offset), SecMask); 
	#endif
		PRINT_CEVA_VRF("vBL1best", vBL1best, stderr);


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
		PRINT_CEVA_VRF("vG0L1best", vG0L1best, stderr);
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
		PRINT_CEVA_VRF("vG1L1best", vG1L1best, stderr);


	}//n	
	//PROFILER_END();
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
