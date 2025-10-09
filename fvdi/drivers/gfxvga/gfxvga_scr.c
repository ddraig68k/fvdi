/* 
 * Pixel read/write routines for Xosera.
 *
 * This file implements for FVDI API functions c_read_pixel() and c_write_pixel().
 */

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#include "driver.h"
#include "../bitplane/bitplane.h"
#include "relocate.h"

#define PIXEL       short
#define PIXEL_SIZE  sizeof(PIXEL)

/*
 * Destination MFDB (odd address marks table operation)
 * x or table address
 * y or table length (high) and type (0 - coordinates)
 */

long CDECL
c_write_pixel(Virtual *vwk, MFDB *dst, long x, long y, long colour)
{
    Workstation *wk;
    long offset;

    if ((long)vwk & 1)
        return 0;

    DPRINTF(("c_write_pixel: x=%ld,y=%ld color=%ld\n\r", x, y, (int)colour));

    wk = vwk->real_address;
    if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {
        offset = wk->screen.wrap * y + x * PIXEL_SIZE;
        *(PIXEL *)((long)wk->screen.mfdb.address + offset) = colour;
    } else {
        offset = (dst->wdwidth * 2 * dst->bitplanes) * y + x * PIXEL_SIZE;
        *(PIXEL *)((long)dst->address + offset) = colour;
    }


    return 1;
}

long CDECL
c_read_pixel(Virtual *vwk, MFDB *src, long x, long y)
{
    Workstation *wk;
    long offset;
    unsigned PIXEL colour;

    DPRINTF(("c_read_pixel: x=%ld,y=%ld\n\r", x, y));

    wk = vwk->real_address;
    if (!src || !src->address || (src->address == wk->screen.mfdb.address)) {
        offset = wk->screen.wrap * y + x * PIXEL_SIZE;
        colour = *(unsigned PIXEL *)((long)wk->screen.mfdb.address + offset);
    } else {
        offset = (src->wdwidth * 2 * src->bitplanes) * y + x * PIXEL_SIZE;
        colour = *(unsigned PIXEL *)((long)src->address + offset);
    }

    return colour;
}
