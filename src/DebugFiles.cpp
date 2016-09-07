#include <stdio.h>
#include <stdlib.h>
#include "rk_typedef.h"                 // Type definition

void writeFile(RK_U8 *data, int Num, int Gap, char* FileName)
{
	FILE* fp = fopen(FileName,"w");
	for ( int i = 0 ; i < Num; i++ )
	{
		fprintf(fp,"%6d,", *data++);
		if ( (i+1)%Gap == 0 )
		{
			fprintf(fp,"\n");
		}
	}

	fclose(fp);
}

void writeFile(RK_U16 *data, int Num, int Gap, char* FileName)
{
	FILE* fp = fopen(FileName,"w");
	for ( int i = 0 ; i < Num; i++ )
	{
		fprintf(fp,"%6d,", *data++);
		if ( (i+1)%Gap == 0 )
		{
			fprintf(fp,"\n");
		}
	}

	fclose(fp);
}

void writeBinFile(RK_U16 *data, int Num, char* FileName)
{
	FILE* fp = fopen(FileName,"wb");
	fwrite(data,sizeof(RK_U16),Num,fp);
	fclose(fp);
}

