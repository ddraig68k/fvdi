/*
 * Line drawing routines
 *
 * Implements FVDI driver API function c_line_draw().
 *
 */

 #include "gfxvga.h"
#include "driver.h" /* For clip_line() */

long CDECL c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2, long line_style_in, long colour, long mode)
{
    return 1;
}
