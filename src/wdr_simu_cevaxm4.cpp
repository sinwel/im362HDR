/***************************************************************************
**  
**  wdr_simu_cevaxm4.cpp
**  
** 	To calcu the cycle of WDR.
**
**  NOTE: dependency on:
**      	profiler_lib. 
**   
**  Author: zxy
**  Contact: zxy@rock-chips.com
**  2016/08/11 10:53:06 version 1.0
**  
**	version 1.0 	have not add profiler.
**   
**	version 1.1 	add profiler and 1.7 cycle/pixel.
**
**	version 2.0 	1.55 cycle/pixel, verify passed.
**
** Copyright 2016, rockchip.
**
***************************************************************************/
//<<! Test for cylce for ceva xm4 platform.
//<<! 
#include "rk_bayerwdr.h"
#include <assert.h>
#include <vec-c.h>
#ifdef __XM4__
#include "profiler.h"
#include "XM4_defines.h"
#endif
extern unsigned short cure_table[24][961];

static unsigned long rand_next = 1;

void CCV_srand(unsigned long seed)
{
    rand_next = seed;
}

int CCV_rand()
{
    return ((rand_next = rand_next * (long)1103515245 + 12345l) & 0x7fffffff);
} 



void wdr_simu_cevaxm4()
{
	RK_U16		pweight_vecc[8*256] = {0};// actully is 17x13 = 221
	RK_U16		scale_table[1025];
	RK_U16		LeftRight[32*16*2];
	RK_U16 		left[32*16],right[32*16];

	int 		w	= 256;
	int 		h	= 128;
	int 		sw  = 17;// 4164/256
	int			stride = w+2;
	RK_U16 		*pixel_in 		= (RK_U16*)malloc((w+2)*(h+2)*sizeof(RK_U16));
	RK_U16 		*pGainMat 		= (RK_U16*)malloc(w*h*sizeof(RK_U16));
	RK_U16 		*pixel_out 		= (RK_U16*)malloc(w*h*sizeof(RK_U16));

	memcpy(scale_table,&cure_table[8][0],961*sizeof(RK_U16));

#ifdef WIN32	
	// copy input with stride.
	for ( int i = 0 ; i < h ; i++ )
	    for ( int j = 0 ; j < stride ; j++ )
		{	
			pixel_in[i*w+j] =  (RK_U16)(CCV_rand()&0x03ff); //i*1024+j;		
		}

	//init pweight_vecc
	for (int i = 0 ; i < 8*256 ; i++ )
		pweight_vecc[i] =  (RK_U16)(CCV_rand()&0x3fff); //i*1024+j;	

#endif

//#ifdef __XM4__
//	PROFILER_START(h, w);
//#endif
	for (int y = 0 ; y < h ; y+=32 )
	{
	    for (int x = 0 ; x < w ; x+=64 )
	    {
        	wdr_process_block(x,
							y,
							64,
							32,
							sw,
							stride,
							w,
							pixel_in,
							pweight_vecc,
							scale_table,
							pGainMat,
							pixel_out,
							LeftRight);
	    }
	}

//#ifdef __XM4__
//	PROFILER_END();
//#endif

	if(pixel_in)
		free(pixel_in);	
	if(pGainMat)
		free(pGainMat);
	if(pixel_out)
		free(pixel_out);
}
