#include <stdio.h>
#include <stdlib.h>
#include "rk_typedef.h"                 // Type definition

void writeFile(RK_U8 *data, int cols, int rows, int stride, char* FileName)
{
	RK_U8 *p = data;
	FILE* fp = fopen(FileName,"w");
	for ( int j = 0 ; j < rows; j++ )
	{
		for ( int i = 0 ; i < cols; i++ )
		{
			fprintf(fp,"%6d,", *(p+i));
		}
		p += stride;
		fprintf(fp,"\n");
	}

	fclose(fp);
}

void writeFile(RK_U16 *data, int cols, int rows, int stride, char* FileName)
{
	RK_U16 *p = data;
	FILE* fp = fopen(FileName,"w");
	for ( int j = 0 ; j < rows; j++ )
	{
		for ( int i = 0 ; i < cols; i++ )
		{
			fprintf(fp,"%6d,", *(p+i));
		}
		p += stride;
		fprintf(fp,"\n");
	}


	fclose(fp);
}

void writeBinFile(RK_U16 *data, int Num, char* FileName)
{
	FILE* fp = fopen(FileName,"wb");
	fwrite(data,sizeof(RK_U16),Num,fp);
	fclose(fp);
}

