/*
 * 16 bit pixel set/get routines, by Johan Klockars.
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

#define BOTH	/* Write in both FastRAM and on screen */

#include "fvdi.h"
#include "relocate.h"
#include "v9990.h"
#include <stdint.h>

extern Access *access;

/* destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */
long CDECL
c_write_pixel (Virtual *vwk, MFDB *dst, long x, long y, long colour)
{
        Workstation *wk;
        long offset;
        uint16_t col;

        if ((long)vwk & 1)
        {
                return 0;
        }

        wk = vwk->real_address;

        if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address))
        {
                v9990_SetCmdWriteMask (0xffff);
                col = (colour & 0x0f) + ((colour & 0x0f) << 4);
                col += (col << 8);
                v9990_SetCmdColour (col);
                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                v9990_SetPoint ((uint16_t)x, (uint16_t)y);
                //#else
                //		offset = wk->screen.wrap * y + x * sizeof(short);
                //#ifdef BOTH
                //		if (wk->screen.shadow.address) {
                //			*(short *)((long)wk->screen.shadow.address + offset) = colour;
                //		}
                //#endif
                //		*(short *)((long)wk->screen.mfdb.address + offset) = colour;
        }
        else
        {
                offset = (dst->wdwidth * 8 * dst->bitplanes) * y + x * sizeof (short);
                * (short *) ((long)dst->address + offset) = colour;
        }

        return 1;
}


long CDECL
c_read_pixel (Virtual *vwk, MFDB *src, long x, long y)
{
        Workstation *wk;
        long offset;
        unsigned long colour;
        wk = vwk->real_address;

        if (!src || !src->address || (src->address == wk->screen.mfdb.address))
        {
                colour = v9990_GetColour (x, y) >> 4; /* 4bpp: colourcode in the high nibble */
                //access->funcs.puts("c_read_pixel()\r\n");
                //		offset = wk->screen.wrap * y + x * sizeof(short);
                //#ifdef BOTH
                //		if (wk->screen.shadow.address) {
                //			colour = *(unsigned short *)((long)wk->screen.shadow.address + offset);
                //		} else {
                //#endif
                //			colour = *(unsigned short *)((long)wk->screen.mfdb.address + offset);
                //#ifdef BOTH
                //		}
        }
        else
        {
                offset = (src->wdwidth * 8 * src->bitplanes) * y + x * sizeof (short);
                colour = * (unsigned short *) ((long)src->address + offset);
                //        colour = 0;
        }

        return colour;
}
