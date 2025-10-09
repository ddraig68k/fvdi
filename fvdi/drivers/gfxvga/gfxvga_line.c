/*
 * Line drawing routines
 *
 * Implements FVDI driver API function c_line_draw().
 *
 */

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)
#define PIXEL_32    long

long CDECL c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2,
                       long pattern, long colour, long mode)
{
    Workstation *wk;
    unsigned long foreground, background;

    if ((long)vwk & 1) {
        return -1;          /* Don't know about anything yet */
    }

    if (!clip_line(vwk, &x1, &y1, &x2, &y2))
        return 1;

    c_get_colours(vwk, colour, &foreground, &background);

    wk = vwk->real_address;

    if ((pattern & 0xffff) == 0xffff) {
        switch (mode) {
        case 1:             /* Replace */
            DPRINTF(("c_line_draw: mode=replace x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            gfx_draw_line(0, x1, y1, x2, y2, foreground, background, 0xFFFF);
        case 2:             /* Transparent */
            DPRINTF(("c_line_draw: mode=transparent x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            /* Transparent line? Don't draw */
            break;
        case 3:             /* XOR */
            DPRINTF(("c_line_draw: mode=xor x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            gfx_draw_line(DRAW_MODE_XOR, x1, y1, x2, y2, foreground, background, 0xFFFF);
            break;
        case 4:             /* Reverse transparent */
            DPRINTF(("c_line_draw: mode=revtrans x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            /* Recerse Transparent line? Don't draw */
            break;
        }
    } else {
        switch (mode) {
        case 1:             /* Replace */
            DPRINTF(("c_line_draw: mode=pat_replace x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            gfx_draw_line(DRAW_PATTERN(1), x1, y1, x2, y2, foreground, background, pattern);
            break;
        case 2:             /* Transparent */
            DPRINTF(("c_line_draw: mode=pat_trans x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            gfx_draw_line(DRAW_PATTERN(1) | DRAW_MODE_TRANS, x1, y1, x2, y2, foreground, background, pattern);
            break;
        case 3:             /* XOR */
            DPRINTF(("c_line_draw: mode=pat_xor x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            gfx_draw_line(DRAW_PATTERN(1) | DRAW_MODE_XOR, x1, y1, x2, y2, foreground, background, pattern);
            break;
        case 4:             /* Reverse transparent */
            DPRINTF(("c_line_draw: mode=pat_revtrans x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            gfx_draw_line(DRAW_PATTERN(1) | DRAW_MODE_REVTRANS, x1, y1, x2, y2, foreground, background, pattern);
            break;
        }
    }
    return 1;       /* Return as completed */
}
