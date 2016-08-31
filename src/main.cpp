
#include "rk_typedef.h"              // Type definition


#define 	WORD_INSTRIDE  256
#define 	WORD_OUTSTRIDE  256
RK_U16		p_u16Src[8*52] =
{
	#include "../data/data8x52.dat"
};
RK_U16		p_u16Dst1[4*32];


void zigzagDebayer(	RK_U16 *p_u16Src, 
						RK_U16 width,
						RK_U16 stride,
						RK_U16 *p_u16Dst	//<<! [out]
						) ;
int main(void)//(int argc, char* argv[])
{
	int ret; 

	zigzagDebayer(	p_u16Src, 
					32,
					52,
					p_u16Dst1	//<<! [out]
						); 	
#ifdef __XM4__
	return ret;	
#endif
}


