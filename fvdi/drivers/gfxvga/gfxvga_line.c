/*
 * Line drawing routines
 *
 * Implements FVDI driver API function c_line_draw().
 *
 */

#define FVDI_DEBUG 1
#include "gfxvga.h"
#include "gfxvdp.h"

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
    volatile unsigned long canary = 0x4A6D7C91UL;
    unsigned long sp_entry, sp_exit;
    unsigned long ret_entry, ret_exit;
    long max_x, max_y;

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_entry));
    ret_entry = (unsigned long)__builtin_return_address(0);

    GFX_CP_ENTER(CP_LINE_DRAW);

    if (!vwk || ((long)vwk & 1)) {
        GFX_CP_EXIT(CP_LINE_DRAW);
        return -1;          /* Don't know about anything yet */
    }

    wk = vwk->real_address;
    if (!wk) {
        GFX_CP_EXIT(CP_LINE_DRAW);
        return -1;
    }

    max_x = wk->screen.mfdb.width - 1;
    max_y = wk->screen.mfdb.height - 1;

    if (!clip_line(vwk, &x1, &y1, &x2, &y2)) {
        GFX_CP_EXIT(CP_LINE_DRAW);
        return 1;
    }

    if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || x1 > max_x || x2 > max_x || y1 > max_y || y2 > max_y) {
        my_kprintf("CP+06o %ld %ld %ld %ld\n", x1, y1, x2, y2);
        GFX_CP_EXIT(CP_LINE_DRAW);
        return -1;
    }

    c_get_colours(vwk, colour, &foreground, &background);
    my_kprintf("CP+06a 99\n");  /* After c_get_colours */

    if (canary != 0x4A6D7C91UL) {
        my_kprintf("CP+06k 1\n");
        GFX_CP_EXIT(CP_LINE_DRAW);
        return -1;
    }

    if ((pattern & 0xffff) == 0xffff) {
        switch (mode) {
        case 1:             /* Replace */
            my_kprintf("CP+06b 99\n");  /* Before VDP calls */
            DPRINTF(("c_line_draw: mode=replace x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            vdp_set_drawmode(DRAW_MODE_SOLID);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(0xFFFF);
            my_kprintf("CP+06c 1\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 1\n");
            break;
        case 2:             /* Transparent */
            DPRINTF(("c_line_draw: mode=transparent x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            /* Transparent line? Don't draw */
            break;
        case 3:             /* XOR */
            DPRINTF(("c_line_draw: mode=xor x1=%ld,y1=%ld,x2=%ld,y2=%ld color=%04X\n\r", x1, y1, x2, y2, (int)foreground));
            vdp_set_drawmode(DRAW_MODE_XOR);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(0xFFFF);
            my_kprintf("CP+06c 3\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 3\n");
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
            vdp_set_drawmode(DRAW_MODE_SOLID);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(pattern);
            my_kprintf("CP+06c 5\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 5\n");
            break;
        case 2:             /* Transparent */
            DPRINTF(("c_line_draw: mode=pat_trans x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            vdp_set_drawmode(DRAW_MODE_TRANS);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(pattern);
            my_kprintf("CP+06c 6\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 6\n");
            break;
        case 3:             /* XOR */
            DPRINTF(("c_line_draw: mode=pat_xor x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            vdp_set_drawmode(DRAW_MODE_XOR);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(pattern);
            my_kprintf("CP+06c 7\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 7\n");
            break;
        case 4:             /* Reverse transparent */
            DPRINTF(("c_line_draw: mode=pat_revtrans x1=%ld,y1=%ld,x2=%ld,y2=%ld pattern=%d color=%04X\n\r", x1, y1, x2, y2, (int)pattern, (int)foreground));
            vdp_set_drawmode(DRAW_MODE_REVTRANS);
            vdp_set_draw_color(foreground);
            vdp_set_back_color(background);
            vdp_set_pattern(pattern);
            my_kprintf("CP+06c 8\n");
            vdp_draw_line(x1, y1, x2, y2);
            my_kprintf("CP+06d 8\n");
            break;
        }
    }

    if (canary != 0x4A6D7C91UL) {
        my_kprintf("CP+06k 2\n");
        GFX_CP_EXIT(CP_LINE_DRAW);
        return -1;
    }

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_exit));
    ret_exit = (unsigned long)__builtin_return_address(0);
    if (sp_entry != sp_exit || ret_entry != ret_exit) {
        my_kprintf("CP+06s sp=%lX->%lX ra=%lX->%lX\n", sp_entry, sp_exit, ret_entry, ret_exit);
    }

    GFX_CP_EXIT(CP_LINE_DRAW);
    return 1;       /* Return as completed */
}
