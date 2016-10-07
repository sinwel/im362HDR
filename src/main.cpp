#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hdr_zigzag.h"              // Type definition

PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA")   uint16_t		pPrevThumb[THUMB_SIZE_W*THUMB_SIZE_W] 			 = {0};// 32k store in DDR.
PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA")   uint16_t		pCurrThumb[THUMB_SIZE_W*THUMB_SIZE_W] 			 = {0};// 32k store in DDR.
PRAGMA_DSECT_LOAD("ZZ_HDR_DTCM_256K") 	ZZHdrDTCMStruct	g_rk1608_256k_dtcm;

// 32k
PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA") uint16_t		pTabLongShort[962*16]   =
{
	#include "../table/longshort_mapping_16banks.dat"
};
// 32k
PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA") uint16_t		pWdrTab[962*16]   =
{
	#include "../table/16banks.dat"
};


#if __XM4__
#include "XM4_defines.h"
PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA") unsigned short dst[2016*1504];
PRAGMA_DSECT_LOAD("HDR_APP_EXT_DATA") unsigned short src[2016*1504] ;
#endif




int main(int argc, char **argv)
{
	FILE			*fp;
	int				W = 2016;
	int				H = 1504;

#ifdef WIN32
	const char		ifname[] = "../../data/raw_2016x1504_new.raw";
	const char		ofname[] = "../../data/raw_2016x1504_Out.raw";
	const char		tfname[] = "../../data/raw_2016x1504_thumb_Out.raw";

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





	// write HDR out
	fp = fopen(ofname, "wb+");
	fwrite(dst, 2, W*H, fp);
	fclose(fp);

	// write thumb out
	fp = fopen(tfname, "wb+");
	fwrite(pCurrThumb, 2, structHDR.mThumbWid*structHDR.mThumbHgt, fp);
	fclose(fp);
	
	if(src)
		free(src);
	if(dst)
		free(dst);
#else
	const char		ifname[] = "data/raw_2016x1504_new.raw";
	const char		ofname[] = "data/raw_2016x1504_Out_ceva.raw";
	fp = fopen(ifname, "rb+");
	
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

	// write HDR out
	fp = fopen(ofname, "wb+");
	fwrite(dst, 2, W*H, fp);
	fclose(fp);

#endif
	return 0;
}


