/* 
 * Pixel read/write routines for Xosera.
 *
 * This file implements for FVDI API functions c_read_pixel() and c_write_pixel().
 */

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
    return 1;
}

long CDECL
c_read_pixel(Virtual *vwk, MFDB *src, long x, long y)
{
    return 0;
}
