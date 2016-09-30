#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hdr_zigzag.h"              // Type definition
#define 	WORD_INSTRIDE  256
#define 	WORD_OUTSTRIDE  256

#if __XM4__
#include "XM4_defines.h".
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA") unsigned short dst[2*64*32] = {0};
PRAGMA_DSECT_LOAD("IMAGE_HDR_APP_EXT_DATA") unsigned short src[2016*1504] = {0};
#endif




int main(int argc, char **argv)
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
	fread(src, 2, W*H, fp);
	fclose(fp);
	
	HDRInterface hdrIf;
	HDRInfStruct structHDR;
	
	structHDR.mRawWid			= W;							  
	structHDR.mRawHgt			= H;			
	structHDR.mRawStride		= W;		
	structHDR.pRawSrc 		    = src;  
	                 
	structHDR.mThumbWid 		= (W+63)/64; 		
	structHDR.mThumbHgt 		= (H+31)/32;  		
	structHDR.mThumbStride		= (W+63)/64;	
	structHDR.pThumbSrcs		= NULL; 
	     
	structHDR.pRawDst       	= dst;
	                            
	structHDR.mRedGain          = 1.0f; 
	structHDR.mBlueGain         = 1.0f;
	structHDR.mWdrGain          = 8.0f;
	                            
	structHDR.mBits          	= 10;	
	structHDR.mNoiseIntensity   = 64;
	structHDR.mExpTimes         = 8;
	structHDR.mBlackLevel     	= 64;
	
	hdrIf.init(&structHDR);
	hdrIf.hdrprocess_sony_raw();
	hdrIf.deinit();
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
	return 0;
}


