/*
 * Fill routines
 *
 * Implements driver API function c_fill_area().
 */

#include "driver.h"

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

/* #define FVDI_DEBUG 1 */
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

long CDECL c_fill_area(Virtual *vwk, long x, long y, long w, long h,
                       short *pattern, long colour, long mode, long interior_style)
{
    //Workstation *wk;
    unsigned long foreground, background;
    short *table;
    (void) pattern;

    if (w <= 0 || h <= 0)
        return 1;

    table = 0;
    if ((long) vwk & 1) {
        if ((y & 0xffff) != 0)
            return -1;      /* Don't know about this kind of table operation */
        table = (short *)x;
        (void) table;
        h = (y >> 16) & 0xffff;
        vwk = (Virtual *)((long)vwk - 1);
        return -1;          /* Don't know about anything yet */
    }

    c_get_colours(vwk, colour, &foreground, &background);

    //wk = vwk->real_address;

    int16_t fill_type = (int16_t)((interior_style >> 16) & 0xFFFF);
    int16_t pattern_index = (int16_t)(interior_style & 0xFFFF);
    // int solid_pattern = (fill_type == 0) || (fill_type == 1) || ((fill_type == 2) && (pattern_index == 8)) 
    //     || ((fill_type == 2) && (pattern_index == 4));

    UWORD draw_mode = 0;
    if (fill_type > 1) {
        draw_mode = DRAW_PATTERN((pattern_index + 2));
    }

    DPRINTF(("c_fill_area Interior style=%08lX fill_type=%d pattern_index=%d\n", interior_style, fill_type, pattern_index));
    
    switch (mode) {
    case 1:             /* Replace */
        DPRINTF(("c_fill_area: mode=replace x=%ld,y=%ld,w=%ld,h=%ld color=%04lX fill_type=%d, pattern_index=%d\n\r", x, y, w, h, foreground, fill_type, pattern_index));
        break;
    case 2:             /* Transparent */
        DPRINTF(("c_fill_area: mode=transparent x=%ld,y=%ld,w=%ld,h=%ld color=%04lX fill_type=%d, pattern_index=%d\n\r", x, y, w, h, foreground, fill_type, pattern_index));
        draw_mode |= DRAW_MODE_TRANS;
        break;
    case 3:             /* XOR */
        DPRINTF(("c_fill_area: mode=xor x=%ld,y=%ld,w=%ld,h=%ld color=%04lX fill_type=%d, pattern_index=%d\n\r", x, y, w, h, foreground, fill_type, pattern_index));
        draw_mode |= DRAW_MODE_XOR;
        break;
    case 4:             /* Reverse transparent */
        DPRINTF(("c_fill_area: mode=revtrans x=%ld,y=%ld,w=%ld,h=%ld color=%04lX fill_type=%d, pattern_index=%d\n\r", x, y, w, h, foreground, fill_type, pattern_index));
        draw_mode |= DRAW_MODE_REVTRANS;
        break;
    }
    
    gfx_draw_box(draw_mode, x, y, x + w -1, y + h - 1, foreground, background);

    return 1;       /* Return as completed */
}
