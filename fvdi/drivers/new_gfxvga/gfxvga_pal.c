/* 
 * Palette handling routines for gfxvga.
 */

#include "gfxvga.h"
#include "driver.h"
#include <stdint.h>

#define red_bits   5
#define green_bits 6
#define blue_bits  5


long CDECL
c_get_colour(Virtual *UNUSED(vwk), long colour)
{
    return 0;
}

void CDECL
c_set_colours(Virtual *UNUSED(vwk), long start, long entries, unsigned short *requested, Colour palette[])
{
}
