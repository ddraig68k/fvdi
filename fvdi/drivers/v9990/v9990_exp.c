/*
 * A 16 bit graphics mono-expand routine, by Johan Klockars.
 *
 * This file is an example of how to write an
 * fVDI device driver routine in C.
 *
 * You are encouraged to use this file as a starting point
 * for other accelerated features, or even for supporting
 * other graphics modes. This file is therefore put in the
 * public domain. It's not copyrighted or under any sort
 * of license.
 */

#include "fvdi.h"
#include "v9990.h"
#include <stdint.h>

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

//extern void CDECL c_get_colour(Virtual *vwk, long colour, short *foreground, short *background);

static void replace (short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
        int i, j;
        unsigned int expand_word, mask;
        x = 1 << (15 - (x & 0x000f));

        for (i = h - 1; i >= 0; i--)
        {
                expand_word = *src_addr++;
                mask = x;

                for (j = w - 1; j >= 0; j--)
                {
                        if (expand_word & mask)
                        {
                                *dst_addr++ = foreground;
                        }
                        else
                        {
                                *dst_addr++ = background;
                        }

                        if (! (mask >>= 1))
                        {
                                mask = 0x8000;
                                expand_word = *src_addr++;
                        }
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
        }
}

static void transparent (short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
        int i, j;
        unsigned int expand_word, mask;
        x = 1 << (15 - (x & 0x000f));

        for (i = h - 1; i >= 0; i--)
        {
                expand_word = *src_addr++;
                mask = x;

                for (j = w - 1; j >= 0; j--)
                {
                        if (expand_word & mask)
                        {
                                *dst_addr++ = foreground;
                        }
                        else
                        {
                                dst_addr++;
                        }

                        if (! (mask >>= 1))
                        {
                                mask = 0x8000;
                                expand_word = *src_addr++;
                        }
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
        }
}

static void xor (short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
        int i, j, v;
        unsigned int expand_word, mask;
        x = 1 << (15 - (x & 0x000f));

        for (i = h - 1; i >= 0; i--)
        {
                expand_word = *src_addr++;
                mask = x;

                for (j = w - 1; j >= 0; j--)
                {
                        if (expand_word & mask)
                        {
                                v = ~*dst_addr;
                                *dst_addr++ = v;
                        }
                        else
                        {
                                dst_addr++;
                        }

                        if (! (mask >>= 1))
                        {
                                mask = 0x8000;
                                expand_word = *src_addr++;
                        }
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
        }
}

static void revtransp (short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
        int i, j;
        unsigned int expand_word, mask;
        x = 1 << (15 - (x & 0x000f));

        for (i = h - 1; i >= 0; i--)
        {
                expand_word = *src_addr++;
                mask = x;

                for (j = w - 1; j >= 0; j--)
                {
                        if (! (expand_word & mask))
                        {
                                *dst_addr++ = foreground;
                        }
                        else
                        {
                                dst_addr++;
                        }

                        if (! (mask >>= 1))
                        {
                                mask = 0x8000;
                                expand_word = *src_addr++;
                        }
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
        }
}

long CDECL c_expand_area (Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour)
{
        Workstation *wk;
        PIXEL *src_addr, *dst_addr, *dst_addr_fast;
        short foreground, background;
        int src_wrap, dst_wrap;
        int src_line_add, dst_line_add;
        unsigned long src_pos, dst_pos;
        int to_screen;
        uint16_t pixelcount;
        VDP_BOX ramblitbox;
        uint8_t *srcaddr, *tmp_addr;
        uint16_t cntX, cntY, i, j;
        uint16_t mask;
        uint8_t pixl;
        wk = vwk->real_address;
        //c_get_colours(vwk, colour, &foreground, &background);
        src_wrap = (long)src->wdwidth * 2;		/* Always monochrome */
        src_addr = src->address;
        src_pos = (short)src_y * (long)src_wrap + (src_x >> 4) * 2;
        src_line_add = src_wrap - (((src_x + w) >> 4) - (src_x >> 4) + 1) * 2;
        to_screen = 0;

        if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address))  		/* To screen? */
        {
                dst_wrap = wk->screen.wrap;
                dst_addr = wk->screen.mfdb.address;
                to_screen = 1;
        }
        else
        {
                dst_wrap = (long)dst->wdwidth * 2 * dst->bitplanes;
                dst_addr = dst->address;
        }

        dst_pos = (short)dst_y * (long)dst_wrap + dst_x * PIXEL_SIZE;
        dst_line_add = dst_wrap - w * PIXEL_SIZE;
        src_addr += src_pos / 2;
        dst_addr += dst_pos / PIXEL_SIZE;
        src_line_add /= 2;
        dst_line_add /= PIXEL_SIZE;			/* Change into pixel count */
        srcaddr = (uint8_t *)src->address + src_pos; ///2;

        if (to_screen)
        {
                /* foreground colour */
                foreground = (colour & 0x0f) + ((colour & 0x0f) << 4);
                foreground += (foreground << 8);
                /* background colour */
                background = ((colour >> 16) & 0x0f) + (((colour >> 16) & 0x0f) << 4);
                background += (background << 8);

                switch (operation)
                {
                case 1:				/* Replace */
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                        break;

                case 2:				/* Transparent */
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC + 16);
                        //VDPWriteReg(VDP_LOP,2+16);
                        break;

                case 3:				/* XOR */
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCEORSC);
                        break;

                case 4:				/* Reverse transparent */
                        //return 0;
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCNOTSC + 16);
                        //VDPWriteReg(VDP_LOP,VDP_LOP_WCSC);
                        break;
                }

#if 1

                /* Transparency fix, if colour to paint == 0x0000 (white)
                 * as it is the same value as transparent color... */
                if (operation == 2 && foreground == 0) // && background != 0x1111)
                {
                        VDPWriteReg (VDP_LOP, 2 + 16);
                        foreground = 0xffff;
                        background = 0x0000;
                }

#endif
                v9990_SetCmdColour (foreground);
                WRITE_VDP (VDP_REGDAT, (uint8_t) (background));
                WRITE_VDP (VDP_REGDAT, (uint8_t) (background >> 8));
                v9990_SetCmdWriteMask (0xffff);
                VDPWriteReg (VDP_ARG, 0);
#if 0

                for (cntY = 0; cntY < h; cntY += 8)
                {
                        for (cntX = 0; cntX <
                }

#else
#if 1
                ramblitbox.top = dst_y;// + (cntY<<3);// + j<<3;

                for (cntY = 0; cntY < (h / 8); cntY++)
                {
                        ramblitbox.left = dst_x; // + (cntX<<3);// + i<<3;

                        for (cntX = 0; cntX < src_wrap; cntX++)
                        {
                                if ((cntX << 3) < w)
                                {
                                        tmp_addr = (uint8_t *)srcaddr + cntX;
                                        j = w - (cntX << 3);
                                        mask = 0xff;

                                        if (j < 8)
                                        {
                                                mask <<= (8 - j);
                                                //ramblitbox.width = j;
                                        }

                                        ramblitbox.width = 8;
                                        ramblitbox.height = 8;
                                        v9990_SetupCopyRamCharToXY (&ramblitbox);

                                        for (pixelcount = 0; pixelcount < ramblitbox.height; pixelcount++)
                                        {
                                                //pixl = *tmp_addr & mask;
                                                //pixl &= mask;
                                                //WRITE_VDP(VDP_CMDDAT, pixl);
                                                WRITE_VDP (VDP_CMDDAT, *tmp_addr);
                                                tmp_addr += src_wrap;
                                        }
                                }

                                ramblitbox.left += 8;
                        }

                        ramblitbox.top += 8;
                        srcaddr += src_wrap << 3;
                }

#endif

                if (h % 8 != 0)
                {
                        ramblitbox.left = dst_x;
                        ramblitbox.top = dst_y + h - (h % 8);

                        for (cntX = 0; cntX < src_wrap; cntX++)
                        {
                                if ((cntX << 3) < w)
                                {
                                        tmp_addr = (uint8_t *)srcaddr + cntX;
                                        //ramblitbox.left = dst_x + (cntX<<3);// + i<<3;
                                        j = w - (cntX << 3);
                                        mask = 0xff;

                                        if (j < 8)
                                        {
                                                mask <<= (8 - j);
                                                //ramblitbox.width = j;
                                        }

                                        ramblitbox.width = 8;
                                        ramblitbox.height = h % 8;
                                        v9990_SetupCopyRamCharToXY (&ramblitbox);

                                        for (pixelcount = 0; pixelcount < ramblitbox.height; pixelcount++)
                                        {
                                                pixl = *tmp_addr & mask;
                                                //pixl &= mask;
                                                WRITE_VDP (VDP_CMDDAT, pixl);
                                                tmp_addr += src_wrap;
                                        }
                                }

                                ramblitbox.left += 8;
                        }

//                        VDPWriteReg(VDP_OPCODE,VDP_OPCODE_STOP);
                }

#endif
        return 1;
}
else
{
        return 0;
}

dst_addr_fast = wk->screen.shadow.address;	/* May not really be to screen at all, but... */
#ifdef BOTH

if (!to_screen || !dst_addr_fast)
        {
#endif

                switch (operation)
                {
                case 1:				/* Replace */
                        replace (src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 2:				/* Transparent */
                        transparent (src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 3:				/* XOR */
                        xor (src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 4:				/* Reverse transparent */
                        revtransp (src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
                        break;
                }

#ifdef BOTH
        }
        else
        {
                dst_addr_fast += dst_pos / PIXEL_SIZE;

                switch (operation)
                {
                case 1:				/* Replace */
                        s_replace (src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 2:				/* Transparent */
                        s_transparent (src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 3:				/* XOR */
                        s_xor (src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
                        break;

                case 4:				/* Reverse transparent */
                        s_revtransp (src_addr, src_line_add, dst_addr, dst_addr_fast, dst_line_add, src_x, w, h, foreground, background);
                        break;
                }
        }

#endif
        return 1;		/* Return as completed */
}
