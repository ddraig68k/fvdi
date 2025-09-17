/* 
 * Pixel read/write routines for GfxVGA.
 *
 * This file implements for FVDI API functions c_read_pixel() and c_write_pixel().
 */

#include "gfxvga.h"
#include "gfxvga.h"
#include "driver.h"

/*
 * Destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */

long CDECL
c_write_pixel(Virtual *vwk, MFDB *dst, long x, long y, long colour)
{
    Workstation *wk;
    int offset;

    if ((long)vwk & 1)
        return 0;

    wk = vwk->real_address;
    if (!dst || !dst->address) {
        // Write the pixel to the screen memory
        offset = (y * wk->screen.mfdb.width) + x;
        *(UWORD *)((long)wk->screen.mfdb.address + offset) = colour;
    }
    
    // TODO: Implment double buffering support.

    // if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {
    //     offset = wk->screen.wrap * y + x;
    //     *(UWORD *)((long)wk->screen.mfdb.address + offset) = colour;
    // } else {
    //     offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x;
    //     *(UWORD *)((long)dst->address + offset) = colour;
    // }

    return 1;
}

long CDECL
c_read_pixel(Virtual *vwk, MFDB *src, long x, long y)
{
	Workstation *wk;
    int offset;
    UWORD colour;

    // static uint16_t offscreen_y_offset = 0, screen_width = 0;
    // if (offscreen_y_offset == 0) {
    //     offscreen_y_offset = vwk->real_address->screen.mfdb.height;
    //     screen_width = vwk->real_address->screen.mfdb.width;
    // }

    offset = (y * wk->screen.mfdb.width) + x;
    colour = *(UWORD *)((long)src->address + offset);

    return colour;
}
