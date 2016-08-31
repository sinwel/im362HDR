//
/////////////////////////////////////////////////////////////////////////
// File: rk_memory.cpp
// Desc: Implementation of DMA operation
// 
// Date: Revised by yousf 20160820
// 
//////////////////////////////////////////////////////////////////////////
////-------- Header files
//
#include "../include/rk_dma.h"          // Memory operation


//////////////////////////////////////////////////////////////////////////
////-------- Functions Definition
//
/************************************************************************/
// Func: classMemory::classMemory()
// Desc: classMemory constructor
//         typedef struct {
//             unsigned int    src_addr;        /* source pic addr */
//             unsigned int    dst_addr;	    /* destin pic addr */
//             unsigned short  width;           /* pixel num */
//             unsigned short  height;			/* row num */
//             unsigned short  src_stride; 	    /* unit is byte */
//             unsigned short  dst_stride;      /* unit is byte */
//             unsigned char   transfer_mode;   /* enum rdma_transfer_mode */
//             unsigned char   shift_num;	    /* 10bit -> 16bit every pixel left shift num / 16bit -> 10bit every pixel right shift num */
//             unsigned char   bit_offset;      /* 10bit -> 16bit src line first pixel bits offset / 16bit -> 10bit dst line first pixel bits offset */
//             unsigned char   dsp_sel;
//             unsigned char   dir;
//             unsigned char   res[3];
//         } rdma_info_t;
//  Out: 
// 
// Date: Revised by yousf 20160801
// 
/*************************************************************************/
U32 rdma_transf(rdma_info_t *info)
{
#ifndef CEVA_CHIP_CODE
    U32     i, j;
    U8      *p_src, *p_dst;
    U16     *p_src_16, *p_dst_16;
    U16     temp;
    U8      offset;
//    U8      offset_int;
    if (info->transfer_mode == RDMA_DIRECTION)
    {
        p_src = (U8 *)info->src_addr;
        p_dst = (U8 *)info->dst_addr;
        for(j=0; j<info->height; j++)
        {
            memcpy(p_dst, p_src, info->width);
            p_src += info->src_stride;
            p_dst += info->dst_stride;
        }
    }
    else if (info->transfer_mode == RDMA_10BIT_2_16BIT)
    {
        p_src    = (U8 *)info->src_addr;
        p_dst_16 = (U16 *)info->dst_addr;
        for(j=0; j<info->height; j++)
        {
            
            for(i=0; i<info->width; i++)
            {
                offset = (info->bit_offset + (i * 2)) & 7;
//                temp   = (p_src[i*5/4] >> offset) | (p_src[i*5/4+1] << (8 - offset));

//                offset_int = (info->bit_offset + (i * 2)) >> 3;
//                temp   = ((p_src[i*5/4 + offset_int]) >> offset) | (p_src[i*5/4 + 1 + offset_int] << (8 - offset));

                temp = (p_src[(i*10 + info->bit_offset)/8] >> offset) | (p_src[(i*10 + info->bit_offset)/8 + 1] << (8 - offset));

                temp  &= 0x3ff;
                p_dst_16[i] =  temp << info->shift_num;
            }
            p_src    += info->src_stride;
            p_dst_16 += info->dst_stride/2;
        }
    }
    else /* RDMA_16BIT_2_10BIT */
    {
        p_src_16 = (U16 *)info->src_addr;
        p_dst    = (U8 *)info->dst_addr;
        for(j=0; j<info->height; j++)
        {
            for(i=0; i<info->width; i++)
            {
                offset = info->bit_offset + ((i * 2) & 7);
                temp   = p_src_16[i] >> info->shift_num;
                temp  &= 0x3ff;
                switch(offset)
                {
                case 0 :
                    p_dst[i*5/4]     = temp & 0xff;
                    p_dst[i*5/4 + 1] = (p_dst[i*5/4 + 1] & 0xfc) | ((temp >> 8) & 3);
                    break;
                case 2 :
                    p_dst[i*5/4]     = (p_dst[i*5/4] & 0x3) | ((temp & 0x3f) << 2);
                    p_dst[i*5/4 + 1] = (p_dst[i*5/4 + 1] & 0xf0) | ((temp >> 6) & 0xf);
                    break;
                case 4 :
                    p_dst[i*5/4]     = (p_dst[i*5/4] & 0xf) | ((temp & 0xf) << 4);
                    p_dst[i*5/4 + 1] = (p_dst[i*5/4 + 1] & 0xc0) | ((temp >> 4) & 0x3f);
                    break;
                case 6 :
                    p_dst[i*5/4]     = (p_dst[i*5/4] & 0x3f) | ((temp & 0x3) << 6);
                    p_dst[i*5/4 + 1] = (temp >> 2) & 0xff;
                    break;
                }
            }
            p_src_16 += info->src_stride/2;
            p_dst    += info->dst_stride;
        }
    }

    return 0;
#elif defined(ASM_CEVA_CODE)
#else
    /* check rdma buf can be used */
    U32 pos;
    rdma_lli_t *cur_lli;
    U32 *stat_addr = (U32 *)RDMA_STATUS_OFFSET;
    U32 *lli_addr = (U32 *)RDMA_LLI_ADDR_OFFSET;
    U32 dsp_add_offset = info->dsp_sel ? 0x18000000 : 0x19000000;

    while(((*stat_addr) & 0xf) == 0)
    {
        _dsp_asm("nop #0x6");
    }

    lli_addr = (U32 *)RDMA_LLI_ADDR_OFFSET;
    pos = g_rdma_buf.pos;

    cur_lli = &g_rdma_buf.lli_list[pos];
    cur_lli->src_addr = info->src_addr;
    cur_lli->dst_addr = info->dst_addr;
    if (info->dir == DIR_EXT_INT)
        cur_lli->dst_addr += dsp_add_offset;
    else if (info->dir == DIR_INT_EXT)
        cur_lli->src_addr += dsp_add_offset;

    cur_lli->src_stride = info->src_stride;
    cur_lli->dst_stride = info->dst_stride;
    cur_lli->acnt = info->width;
    cur_lli->bcnt = info->height;
    cur_lli->opt.shift_num = info->shift_num;
    cur_lli->opt.mode = info->transfer_mode;
    cur_lli->opt.last = 1;
    cur_lli->opt.bit_addr = info->bit_offset;
    *lli_addr = (U32)cur_lli + dsp_add_offset;

    g_rdma_buf.pos = (pos + 1) & 7;

    return pos;
#endif
} // rdma_transf()




//////////////////////////////////////////////////////////////////////////



