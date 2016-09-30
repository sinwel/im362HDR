/***************************************************************************
**  
**  hdr_tonning.cpp
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

#include "rk_bayerhdr.h"

//<<! do zigzag hdr stag2 for long and short image.
//<<! 1. vld imgL,imgS and diff = SAD(imgL,imgS)
//<<! 2. r_filter = MAX(3,3,r)
//<<! 3. LUT by r_filter
//<<! 4. mac(L,S,r)
//<<! 5. WDR
#define 	HDR_LS_BITS			8
#define 	HDR_LS_ROUND		(1<<(HDR_LS_BITS-1))
#define 	HDR_LS_QUANT		((1<<HDR_LS_BITS)-1)
void FilterdLUTBilinear ( uint16_t*	p_u16Weight, 		//<<! [in] 0-1024 scale tab.
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
							 uint16_t*	p_u16Dst)		//<<! [out] HDR out 16bit,have not do WDR.
{
#ifdef __XM4__
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

		#if ENABLE_WDR

			// TODO:  read Thumb to bilinear the map, LUT table for mac.

			// y aixs interpolation
			v8  = (ushort16)vmac3(v6, (unsigned short)(32 - row  ), v7, (unsigned short)row,     (uint16) 0, (unsigned char)(5+log2ExpTimes));
			v9  = (ushort16)vmac3(v6, (unsigned short)(32 - row-1), v7, (unsigned short)(row+1), (uint16) 0, (unsigned char)(5+log2ExpTimes));
			v10 = (ushort16)vmac3(v6, (unsigned short)(32 - row-2), v7, (unsigned short)(row+2), (uint16) 0, (unsigned char)(5+log2ExpTimes));
			v11 = (ushort16)vmac3(v6, (unsigned short)(32 - row-3), v7, (unsigned short)(row+3), (uint16) 0, (unsigned char)(5+log2ExpTimes));


			// LUT wdr table for scale.
			v8  = vpld(rel,pScaleTab16banks , (short16)v8 );
			v9  = vpld(rel,pScaleTab16banks , (short16)v9 );
			v10 = vpld(rel,pScaleTab16banks , (short16)v10);
			v11 = vpld(rel,pScaleTab16banks , (short16)v11);

			// mpy and clip
			vOut0 = (ushort16)vmpy(psl, vOut0, v8,  (unsigned char)(5+log2ExpTimes));
			vOut1 = (ushort16)vmpy(psl, vOut0, v9,  (unsigned char)(5+log2ExpTimes));
			vOut2 = (ushort16)vmpy(psl, vOut0, v10, (unsigned char)(5+log2ExpTimes));
			vOut3 = (ushort16)vmpy(psl, vOut0, v11, (unsigned char)(5+log2ExpTimes));
		#endif
			// TODO: do wdr by prev thumb image to bilinear hdrout.

			
			vst(vOut0,(ushort16*)(p_dst)   				,			0xffff);  
			vst(vOut1,(ushort16*)(p_dst+imageStep)		,			0xffff);  
			vst(vOut2,(ushort16*)(p_dst+2*imageStep)	,			0xffff);  
			vst(vOut3,(ushort16*)(p_dst+3*imageStep)	,			0xffff);  
			p_dst += 4*imageStep;
			
			
		}
		if ( ((col+16)&31) == 0 )
		{
			vAvgSeg		= (ushort16)vaccshiftr(vaccThumb, (ushort16) 6);//2x32
			thumb 		= (vintrasum(vintrasum(vAvgSeg))>>4);			// scale 32x32 block to one pixel.
			*(p_u16CurrThumb+(col/32)) = thumb;
		}
	}

	
#ifdef __XM4__
PROFILER_END();
#endif	

}



