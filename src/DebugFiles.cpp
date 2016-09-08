#include <stdio.h>
#include <stdlib.h>
#include "rk_typedef.h"                 // Type definition

void writeFile(uint8_t *data, int cols, int rows, int stride, char* FileName)
{
	uint8_t *p = data;
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

void writeFile(uint16_t *data, int cols, int rows, int stride, char* FileName)
{
	uint16_t *p = data;
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

void writeBinFile(uint16_t *data, int Num, char* FileName)
{
	FILE* fp = fopen(FileName,"wb");
	fwrite(data,sizeof(uint16_t),Num,fp);
	fclose(fp);
}

