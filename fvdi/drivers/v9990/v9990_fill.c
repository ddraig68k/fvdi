/*
 * A 16 bit graphics fill routine, by Johan Klockars.
 *
 * $Id: 16b_fill.c,v 1.2 2002-07-10 22:13:39 johan Exp $
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

#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

#include "fvdi.h"
#include "v9990.h"

extern void CDECL c_get_colour (Virtual *vwk, long colour, short *foreground, short *background);
extern long CDECL fallback_fill (Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style);

/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

#ifdef BOTH
static void s_replace (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        *addr_fast = background;
                                        addr_fast++;
#endif
                                        *addr = background;
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void s_transparent (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void s_xor (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask, v;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                v = ~*addr_fast;
#else
                                v = ~*addr;
#endif
#ifdef BOTH
                                *addr_fast = v;
                                addr_fast++;
#endif
                                *addr = v;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        v = ~*addr_fast;
#else
                                        v = ~*addr;
#endif
#ifdef BOTH
                                        *addr_fast = v;
                                        addr_fast++;
#endif
                                        *addr = v;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void s_revtransp (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0x0000:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (! (pattern_word & mask))
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}


#define BOTH_WAS_ON
#endif
#undef BOTH

/*
 * The functions below are exact copies of those above.
 * The '#undef BOTH' makes sure that this works as it should
 * when no shadow buffer is available
 */

static void replace (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        *addr_fast = background;
                                        addr_fast++;
#endif
                                        *addr = background;
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void transparent (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void xor (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask, v;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0xffff:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                v = ~*addr_fast;
#else
                                v = ~*addr;
#endif
#ifdef BOTH
                                *addr_fast = v;
                                addr_fast++;
#endif
                                *addr = v;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (pattern_word & mask)
                                {
#ifdef BOTH
                                        v = ~*addr_fast;
#else
                                        v = ~*addr;
#endif
#ifdef BOTH
                                        *addr_fast = v;
                                        addr_fast++;
#endif
                                        *addr = v;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}

static void revtransp (short *addr, short *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, short foreground, short background)
{
        int i, j;
        unsigned int pattern_word, mask;
        i = y;
        h = y + h;
        x = 1 << (15 - (x & 0x000f));

        for (; i < h; i++)
        {
                pattern_word = pattern[i & 0x000f];

                switch (pattern_word)
                {
                case 0x0000:
                        for (j = w - 1; j >= 0; j--)
                        {
#ifdef BOTH
                                *addr_fast = foreground;
                                addr_fast++;
#endif
                                *addr = foreground;
                                addr++;
                        }

                        break;

                default:
                        mask = x;

                        for (j = w - 1; j >= 0; j--)
                        {
                                if (! (pattern_word & mask))
                                {
#ifdef BOTH
                                        *addr_fast = foreground;
                                        addr_fast++;
#endif
                                        *addr = foreground;
                                        addr++;
                                }
                                else
                                {
#ifdef BOTH
                                        addr_fast++;
#endif
                                        addr++;
                                }

                                if (! (mask >>= 1))
                                {
                                        mask = 0x8000;
                                }
                        }

                        break;
                }

#ifdef BOTH
                addr_fast += line_add;
#endif
                addr += line_add;
        }
}


#ifdef BOTH_WAS_ON
#define BOTH
#endif

long CDECL c_vfill_area (Virtual *vwk, long x, long y, long w, long h,
                         short *pattern, long colour, long mode, long interior_style)
{
        Workstation *wk;
        short *addr, *addr_fast;
        short foreground, background, col, bg;
        int line_add;
        long pos;
        short *table;
        short pat;
        uint8_t patline, pat_block;
        uint8_t pattern_fill, pattern_num_bytes;
        uint8_t pattern_byte[32];
        uint16_t backbuffer_x, backbuffer_y;
        VDP_BOX box;
        box.left = x;
        box.top = y;
        box.width = w;
        box.height = h;
        table = 0;

        if ((long)vwk & 1)
        {
                if ((y & 0xffff) != 0)
                {
                        return -1;        /* Don't know about this kind of table operation */
                }

                table = (short *)x;
                h = (y >> 16) & 0xffff;
                vwk = (Virtual *) ((long)vwk - 1);
                return -1;			/* Don't know about anything yet */
        }

        /* foreground colour */
        col = (colour & 0x0f) + ((colour & 0x0f) << 4);
        col += (col << 8);
        /* background colour */
        bg = ((colour >> 16) & 0x0f) + (((colour >> 16) & 0x0f) << 4);
        bg += (bg << 8);

        switch (mode)
        {
        case 1:				/* Replace */
                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                break;

        case 2:				/* Transparent */
                VDPWriteReg (VDP_LOP, VDP_LOP_WCORSC);
                break;

        case 3:				/* XOR */
                VDPWriteReg (VDP_LOP, VDP_LOP_WCEORSC);
                //col = 0x1110; /* Toggle bit 0 => black/white */
                break;

        case 4:				/* Reverse transparent */
                VDPWriteReg (VDP_LOP, 4 + 16);
                break;
        default:
                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
        }

#if 1

        /* Transparency fix, if colour to paint == 0x0000
         * as it is the same value as transparent color... */
        if (mode == 2 && col == 0) // && background != 0)
        {
                VDPWriteReg (VDP_LOP, 2 + 16);
                col = 0xffff;
        }

        /* XOR fix, white == 0x0000, black == 0x1111
         * skip everything else. At least for now. */
        if (mode == 3) // && (col == 0x7777)) // && bg == 0x1111)) // && background != 0)
        {
                if (col == 0)
                {
                        col = 0x1111;
                }
                else
                {
                        //return 1;
                        //col = 0x1111;
                }
        }

#endif
        v9990_SetCmdColour (col);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (bg));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (bg >> 8));
        VDPWriteReg (VDP_ARG, 0);
        v9990_SetCmdWriteMask (0xffff);
#if 1
        pattern_fill = 0;

        for (patline = 0; patline < 16; patline++)
        {
                if ((pattern[patline] & 0xffff) != 0xffff)
                {
                        pattern_fill = 1;
                        break;
                }
        }

        /* Pattern fill => fill 16x16 to a backbuffer, copy backbuffer n-times to screen */
        if (pattern_fill)
        {
                VDP_BOX backbuffer;
                VDP_COPY_XY_XY blitbox;
                uint8_t direction;
                uint16_t cntX, cntY;
                uint16_t mask;
                pattern_num_bytes = backbuffer.height * (backbuffer.width >> 4);
                //v9990_SetupCopyRamCharToXY(&backbuffer);
                /* Iterate over every 8x8 block : top left, top right, bottom left, bottom right */
#if 0
                pos = 0;

                for (pat_block = 0; pat_block < 32; pat_block += 16)
                {
                        for (patline = 0; patline < 8; patline++)
                        {
                                pat = pattern[ (pat_block + patline) & 0x0f];
                                pattern_byte[ (patline)] = 0b11111111; //(pat >> 8) & 0xff;
                                pattern_byte[ (patline + 8)] = 0b00000000; //pat & 0xff;
                                pos++;
                                //WRITE_VDP(0, pattern_byte);
                                //WRITE_VDP(VDP_CMDDAT, pattern_byte);
                                //asm volatile("move.b   %0, 0x3df600"::"m"(*(pattern_byte++)));
                                //pattern_byte++;
                        }
                }

                v9990_SetVRAMWrite (320); // 640/0 => 320
                v9990_CopyRamToVram (pattern_byte, 32);
                //for(pos=0;pos<32;pos++)
                //{
                //        asm volatile("move.b   %0, 0x3df600"::"m"(pattern_byte[pos]));
                //}
#else
                v9990_SetVRAMWrite (320); // 640/0 => 320
                v9990_CopyRamToVram ((uint8_t *)pattern, 32);
#endif
                /* 1. Blit the backbuffer n-times to destination MFDB on screen.
                 * Fill row from backbuffer, blit row to complete fill height. */
                backbuffer.width = 16;
                backbuffer.height = 16;

                /* second buffer for <16px width */
                backbuffer.top = 480; // under screen
                backbuffer.left = 0; // under screen
                v9990_CopyVRamCharToXY (&backbuffer, 320);

                for (cntY = 0; cntY < h; cntY += 16)
                {
                        if (h - cntY < 16)
                        {
                                backbuffer.height = h - cntY;
                        }

                        backbuffer.top = y + cntY;

                        for (cntX = 0; cntX < w; cntX += 16)
                        {
                                wait_vdp();

                                if (w - cntX < 16)
                                {
#if 1
                                    direction = 0;
                                    blitbox.sourceX = 0;
                                    blitbox.sourceY = 480;
                                    blitbox.destX = x + cntX;
                                    blitbox.destY = y + cntY;
                                    blitbox.width = w - cntX;
                                    blitbox.height = h - cntY;
                                    VDPWriteReg (VDP_ARG, (uint8_t)direction);
                                    VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                    v9990_SetCmdWriteMask (0xffff);
                                    v9990_CopyXYToXY (&blitbox);
#else
                                    mask = 0xffff;
                                    //mask <<= (w-cntX);
                                    v9990_SetCmdWriteMask(mask);
                                    backbuffer.width = w - cntX;
#endif
                                }
                                else
                                {
                                    v9990_SetCmdWriteMask (0xffff);
                                    backbuffer.width = 16;
                                    backbuffer.left = x + cntX;
                                    //v9990_CopyXYToXY(&ramblitbox);
                                    v9990_CopyVRamCharToXY (&backbuffer, 320);
                                }

                                //backbuffer.left = x + cntX;
                                //v9990_CopyXYToXY(&ramblitbox);
                                //v9990_CopyVRamCharToXY (&backbuffer, 320);
                        }
                }

                return 1;
        }

#endif
        //v9990_SetBackdropColour(2);
        c_get_colours (vwk, colour, &foreground, &background);
        wk = vwk->real_address;
        pos = (short)y * (long)wk->screen.wrap + x * 2;
        addr = wk->screen.mfdb.address;
        line_add = (wk->screen.wrap - w * 2) >> 1;
        addr += pos >> 1;
        //VDPWriteReg(VDP_ARG,0);
        //v9990_SetCmdWriteMask(0xffff);
        //v9990_SetCmdColour(col);
        //WRITE_VDP(VDP_REGDAT,(uint8_t)(bg));
        //WRITE_VDP(VDP_REGDAT,(uint8_t)(bg>>8));
        v9990_DrawFilledBox (&box, col);
        wait_vdp();
        return 1;		/* Return as completed */
}
