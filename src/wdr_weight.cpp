#include "rk_bayerwdr.h"


void wdrPreFilterBlock( ushort *pBaseRawData, ushort *p_dst, int BlockHeight, int BlockWidth )
{
	RK_U16		*p1, *p2, *p3;
	int			x,y;  
	int 		ScaleDownlight = 0;

	p1 = pBaseRawData;
	p2 = pBaseRawData + BlockWidth;
	p3 = pBaseRawData + 2 * BlockWidth;

	for (y = 1; y < BlockHeight - 1 ; y++)
	{
		for (x = 1; x < BlockWidth - 1; x++)
		{

			ScaleDownlight	= p1[x - 1] + 2 * p1[x] + p1[x + 1] + 2 * p2[x - 1] + 4 * p2[x] + 2 * p2[x + 1] + p3[x - 1] + 2 * p3[x] + p3[x + 1];

			ScaleDownlight >>= 6;

			p_dst[ y * BlockWidth + x ] = ScaleDownlight;
		}
		p1 += BlockWidth;
		p2 += BlockWidth;
		p3 += BlockWidth;
	}
}

void CalcuHist( ushort *p_src, ushort *pcount_mat, uint *pweight_mat, int BlockHeight, int BlockWidth, int statisticWidth, int row )
{
	RK_U16		lindex;
	int			x,y,blacklevel=256;  
	int 		idx, idy; 
	int 		ScaleDownlight = 0;

	for (y = 1; y < BlockHeight - 1; y++)
	{
		for (x = 1; x < BlockWidth - 1; x++)
		{

			ScaleDownlight	= p_src[ y * BlockWidth + x ];
			lindex = (ScaleDownlight + 1024) >> 11;

			idx = (x  - 1 + 16) >> 5;
			idy = row * 32 + ((y - 1 + 16) >> 5);

			pcount_mat [lindex*256 + idy*statisticWidth + idx] = pcount_mat [lindex*256 + idy*statisticWidth + idx] + 1;

			pweight_mat[lindex*256 + idy*statisticWidth + idx] = pweight_mat[lindex*256 + idy*statisticWidth + idx] + ScaleDownlight;

		}
	}
}

void HistFilter( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int i,x,y;
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i*256 + y*statisticWidth];
			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
					tr = pcount_mat[i*256 + y*statisticWidth + x + 1];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr);
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pcount_mat[i*256 + x];
			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
					tr = pcount_mat[i*256 + (y + 1)*statisticWidth + x];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr );
			}
		}
	}


	
	// do the 3-rd array filter .
	
	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;
			tr = pcount_mat[0*256 + y*statisticWidth + x];
			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
					tr = pcount_mat[(i + 1)*256 + y*statisticWidth + x];
				else
					tr = 0;
				pcount_mat[i*256 + y*statisticWidth + x] = (tl) + (tm *2) + (tr);// enlarge 4 times.
			}
		}
	}
	//filter
	// do the 3-rd array filter .
	for (i = 0; i < 9; i++)
	{
		RK_U32	tl, tm, tr;
		for (y = 0; y < statisticHeight; y++)
		{
			tl = 0;
			tm = 0;

			tr = pweight_mat[i*256 + y*statisticWidth];

			for (x = 0; x < statisticWidth; x++)
			{
				tl = tm;
				tm = tr;
				if (x < statisticWidth - 1)
				{
					tr = pweight_mat[i*256 + y*statisticWidth + x + 1];
				}
				else
					tr = 0;

				pweight_mat[i*256 +y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
				
			}
		}
		for (x = 0; x < statisticWidth; x++)
		{
			tl = 0;
			tm = 0;
			tr = pweight_mat[i*256 + x];

			for (y = 0; y < statisticHeight; y++)
			{
				tl = tm;
				tm = tr;
				if (y < statisticHeight - 1)
				{
					tr = pweight_mat[i*256 + (y + 1)*statisticWidth + x];
				}
				else
					tr = 0;

				pweight_mat[i*256 + y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}
	
	for (y = 0; y < statisticHeight; y++)
	{
		for (x = 0; x < statisticWidth; x++)
		{
			RK_U32	tl, tm, tr;

			tl = 0;
			tm = 0;

			tr = pweight_mat[y*statisticWidth + x];

			for (i = 0; i < 9; i++)
			{
				tl = tm;
				tm = tr;
				if (i < 8)
				{

					tr = pweight_mat[(i + 1)*256 + y*statisticWidth + x];

				}
				else
					tr = 0;

				pweight_mat[i*256 + y*statisticWidth + x] = (tl >> 2) + (tm >> 1) + (tr >> 2);
			}
		}
	}

}

void normalizeWeight( ushort *pcount_mat, uint *pweight_mat, int statisticHeight, int statisticWidth )
{
	int			i,x,y;  
	for (i = 0; i < 9; i++)
	{
		for (y = 0; y < statisticHeight; y++)
		{
			for (x = 0; x < statisticWidth; x++)
			{
				if (pcount_mat[i*256 + y*statisticWidth + x])
				{
					pcount_mat[i*256 +y*statisticWidth + x] = (RK_U16)(1*pweight_mat[i*256 + y*statisticWidth + x] / pcount_mat[i*256 + y*statisticWidth + x]);
				}
				else
				{
					pcount_mat[i*256 +y*statisticWidth + x] = 0;
				}
				if (pcount_mat[i*256 +y*statisticWidth + x] > 16*1023){
					pcount_mat[i*256 +y*statisticWidth + x] = 16*1023;// mux for RK_U16, OUTPUT.
				}
			}
		}
	}
}


