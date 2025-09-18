/*
 * Implements FVDI driver API functions c_expand_area() and c_blit_area().
 */

#include "fvdi.h"
#include <stdint.h>
#include "gfxvga.h"
#include "driver.h"

#include <osbind.h>

/* #undef FVDI_DEBUG */


long CDECL
c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
              long operation, long colour)
{
    return 1;
    //return 0;
}
long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
            long operation)
{
    return 1;
}
