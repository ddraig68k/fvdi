/*
 * Fill routines
 *
 * Implements driver API function c_fill_area().
 */

#include "gfxvga.h"
#include "driver.h"

long CDECL c_fill_area(Virtual *vwk, long x0, long y0, long w, long h, short *pattern, long colour, long mode,
                       long interior_style)
{
    return 1;
}
