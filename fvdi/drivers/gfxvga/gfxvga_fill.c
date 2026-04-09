/*
 * Fill routines
 *
 * Implements driver API function c_fill_area().
 */

#include "driver.h"

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define FVDI_DEBUG 1
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

long CDECL c_fill_area(Virtual *vwk, long x, long y, long w, long h,
                       short *pattern, long colour, long mode, long interior_style)
{
    Workstation *wk;
    unsigned long foreground, background;
    volatile unsigned long canary = 0x31D4A2B7UL;
    unsigned long sp_entry, sp_exit;
    unsigned long ret_entry, ret_exit;
    ULONG fb_start, fb_end;
    ULONG vram_start, vram_end;
    ULONG dst_first, dst_last;
    long x1, y1;
    short *table;
    (void) pattern;

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_entry));
    ret_entry = (unsigned long)__builtin_return_address(0);

    GFX_CP_ENTER(CP_FILL_AREA);

    if (!vwk) {
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    if (w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_FILL_AREA);
        return 1;
    }

    table = 0;
    if ((long) vwk & 1) {
        if ((y & 0xffff) != 0) {
            GFX_CP_EXIT(CP_FILL_AREA);
            return -1;      /* Don't know about this kind of table operation */
        }
        table = (short *)x;
        (void) table;
        h = (y >> 16) & 0xffff;
        vwk = (Virtual *)((long)vwk - 1);
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;          /* Don't know about anything yet */
    }

    wk = vwk->real_address;
    if (!wk || !wk->screen.mfdb.address) {
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    if (x < 0) {
        w += x;
        x = 0;
    }
    if (y < 0) {
        h += y;
        y = 0;
    }
    if (x + w > wk->screen.mfdb.width) {
        w = wk->screen.mfdb.width - x;
    }
    if (y + h > wk->screen.mfdb.height) {
        h = wk->screen.mfdb.height - y;
    }
    if (w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_FILL_AREA);
        return 1;
    }

    x1 = x + w - 1;
    y1 = y + h - 1;

    fb_start = (ULONG)wk->screen.mfdb.address;
    fb_end = fb_start + (ULONG)wk->screen.wrap * (ULONG)wk->screen.mfdb.height;
    dst_first = fb_start + (ULONG)y * (ULONG)wk->screen.wrap + (ULONG)x * PIXEL_SIZE;
    dst_last = fb_start + (ULONG)y1 * (ULONG)wk->screen.wrap + (ULONG)x1 * PIXEL_SIZE;
    if (dst_first < fb_start || dst_last >= fb_end || dst_last < dst_first) {
        my_kprintf("CP+08m d=%lX..%lX fb=%lX..%lX\n", dst_first, dst_last, fb_start, fb_end);
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    vram_start = (ULONG)g_vdp_memory_base;
    vram_end = vram_start + 0x00100000UL;
    if (dst_first < vram_start || dst_last >= vram_end) {
        my_kprintf("CP+08w d=%lX..%lX vram=%lX..%lX\n", dst_first, dst_last, vram_start, vram_end);
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    c_get_colours(vwk, colour, &foreground, &background);
    my_kprintf("CP+08a 99\n");  /* After c_get_colours */

    if (canary != 0x31D4A2B7UL) {
        my_kprintf("CP+08k\n");
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    //wk = vwk->real_address;

    int16_t fill_type = (int16_t)((interior_style >> 16) & 0xFFFF);
    int16_t pattern_index = (int16_t)(interior_style & 0xFFFF);
    // int solid_pattern = (fill_type == 0) || (fill_type == 1) || ((fill_type == 2) && (pattern_index == 8)) 
    //     || ((fill_type == 2) && (pattern_index == 4));

    UWORD vdp_mode = DRAW_MODE_SOLID;
    UWORD vdp_pattern = 0;
    if (fill_type > 1) {
        vdp_pattern = (pattern_index + 2);
    }

    my_kprintf("CP+08i style=%08lX type=%d pat=%d\n", interior_style, fill_type, pattern_index);
    
    switch (mode) {
    case 1:             /* Replace */
        my_kprintf("CP+08r\n");
        break;
    case 2:             /* Transparent */
        my_kprintf("CP+08t\n");
        vdp_mode = DRAW_MODE_TRANS;
        break;
    case 3:             /* XOR */
        my_kprintf("CP+08x\n");
        vdp_mode = DRAW_MODE_XOR;
        break;
    case 4:             /* Reverse transparent */
        my_kprintf("CP+08v\n");
        vdp_mode = DRAW_MODE_REVTRANS;
        break;
    }
    
    my_kprintf("CP+08b 99\n");  /* Before VDP calls */
    vdp_set_drawmode(vdp_mode);
    vdp_set_draw_color(foreground);
    vdp_set_back_color(background);
    vdp_set_pattern(vdp_pattern);
    vdp_draw_fill_rect((UWORD)x, (UWORD)y, (UWORD)x1, (UWORD)y1);

    if (canary != 0x31D4A2B7UL) {
        my_kprintf("CP+08k\n");
        GFX_CP_EXIT(CP_FILL_AREA);
        return -1;
    }

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_exit));
    ret_exit = (unsigned long)__builtin_return_address(0);
    if (sp_entry != sp_exit || ret_entry != ret_exit) {
        my_kprintf("CP+08s\n");
    }

    GFX_CP_EXIT(CP_FILL_AREA);
    return 1;       /* Return as completed */
}
