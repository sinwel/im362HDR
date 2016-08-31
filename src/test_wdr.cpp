
#include "rk_typedef.h"              // Type definition
#include "rk_bayerwdr.h"


#define 	WORD_INSTRIDE  256
#define 	WORD_OUTSTRIDE  256
RK_U16		p_u16Src[(32+2)*(256+2)];
RK_U16		p_u16Dst1[(32+2)*(256+2)];
RK_U16		p_u16Dst2[(32+2)*(256+2)];
#ifdef __XM4__
RK_U16      g_BaseThumbBuf[409600];                         // Thumb data pointers
int main(void)//(int argc, char* argv[])
#else
void simuVSplatform()
#endif
{
	int ret; 

	wdr_simu_cevaxm4();
	
#ifdef __XM4__
	return ret;	
#endif
}
void histogram(uchar* p_u8Src, ushort* p_u16DstB0 MEM_BLOCK(0), ushort* p_u16DstB1 MEM_BLOCK(1), uint s32SrcStep, int s32N, int s32M)
{

	int i,j;
	short16 v0, v1;

	ushort* p_u16dstB0 MEM_BLOCK(0) = p_u16DstB0;
	ushort* p_u16dstB1 MEM_BLOCK(1) = p_u16DstB1;
	ushort vprRightMask;
	vprRightMask = 0xffff;

	if (s32M != (s32M >> 4) << 4)
		vprRightMask = (1 << (s32M & 15)) - 1;

	for (i=0;i<s32M;i+=16)
	{
		uchar* p_u8data = p_u8Src + i;
		ushort vprMask = 0xffff;

		if (i + 16 > s32M)
			vprMask = vprRightMask;

		for (j=0;j<s32N;j+=2)
		{

			v0 = (short16)*(uchar16*)p_u8data;
			p_u8data+=s32SrcStep;
			v1 = (short16)*(uchar16*)p_u8data;
			p_u8data+=s32SrcStep;

			vhist((short*)p_u16dstB0, v0, (ushort)vprMask);
			if (j + 2  > s32N)
			{
				vprMask = 0;
			}
			vhist((short*)p_u16dstB1, v1, (ushort)vprMask);
		}
	}
	//GET_FUNCTION_NAME();
}
