#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "rk_typedef.h"              // Type definition
#if __XM4__
#include "XM4_defines.h".
unsigned short src[2016*1504] PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA"); 
/*= 
{
	#include "../data/128x32.dat"
};*/
unsigned short dst[2*64*32] PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_INT_BANK_0");

#endif
#define 	WORD_INSTRIDE  256
#define 	WORD_OUTSTRIDE  256



void hdrprocess_sony_raw( unsigned short *src, 
								unsigned short *dst, 
								unsigned short *simage, 
								int W, 
								int H, 
								int times, 
								int noise_thred);
void main(int argc, char **argv)
{
	FILE			*fp;
	int				W = 2016;
	int				H = 1504;

#ifdef WIN32
	const char		ifname[] = "../../data/raw_2016x1504_new.raw";
	const char		ofname[] = "../../data/raw_2016x1504_Out.raw";
	fp = fopen(ifname, "rb+");
	unsigned short 	*src,*dst;
	src = (unsigned short*)malloc(W*H*sizeof(unsigned short));
	dst = (unsigned short*)malloc(W*H*sizeof(unsigned short));
	fread(src, 2, W*H+2016, fp);
	fclose(fp);

	hdrprocess_sony_raw(src, dst, 0, W, H, 4, 0);

	fp = fopen(ofname, "wb+");
	fwrite(dst, 2, W*H, fp);
	fclose(fp);
	if(src)
		free(src);
	if(dst)
		free(dst);
#else
	const char		ifname[] = "data/raw_2016x1504_new.raw";
	const char		ofname[] = "data/raw_2016x1504_Out.raw";
	fp = fopen(ifname, "rb+");
	
	fread(src, 2, W*H, fp);
	fclose(fp);

	hdrprocess_sony_raw(src, dst, 0,  W, H, 4, 0);

#endif

}


