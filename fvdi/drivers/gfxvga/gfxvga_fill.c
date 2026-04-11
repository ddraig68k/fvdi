/*
 * Fill routines
 *
 * Implements driver API function c_fill_area().
 */

#include "gfxvga.h"
#include "driver.h"

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)
#define PIXEL_32    long

static void fill_replace(PIXEL *addr, PIXEL *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned short pattern_word, mask;

    (void) addr_fast;
    i = y;
    h = y + h;
    x = 1 << (15 - (x & 0x000f));

    if (w <= 0 || h <= 0)
        unreachable();
    for(; i < h; i++) {
        pattern_word = pattern[i & 0x000f];
        switch (pattern_word) {
        case 0xffff:
            for(j = w - 1; j >= 0; j--) {
                *addr = foreground;
                addr++;
            }
            break;
        default:
            mask = x;
            for(j = w - 1; j >= 0; j--) {
                if (pattern_word & mask) {
                    *addr = foreground;
                    addr++;
                } else {
                    *addr = background;
                    addr++;
                }
                if (!(mask >>= 1))
                    mask = 0x8000;
            }
            break;
        }
        addr += line_add;
    }
}

static void fill_transparent(PIXEL *addr, PIXEL *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned short pattern_word, mask;

    (void) addr_fast;
    (void) background;
    i = y;
    h = y + h;
    x = 1 << (15 - (x & 0x000f));

    if (w <= 0 || h <= 0)
        unreachable();
    for(; i < h; i++) {
        pattern_word = pattern[i & 0x000f];
        switch (pattern_word) {
        case 0xffff:
            for(j = w - 1; j >= 0; j--) {
                *addr = foreground;
                addr++;
            }
            break;
        default:
            mask = x;
            for(j = w - 1; j >= 0; j--) {
                if (pattern_word & mask) {
                    *addr = foreground;
                    addr++;
                } else {
                    addr++;
                }
                if (!(mask >>= 1))
                    mask = 0x8000;
            }
            break;
        }
        addr += line_add;
    }
}

static void fill_xor(PIXEL *addr, PIXEL *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned short pattern_word, mask;
    PIXEL v;

    (void) addr_fast;
    (void) foreground;
    (void) background;
    i = y;
    h = y + h;
    x = 1 << (15 - (x & 0x000f));

    if (w <= 0 || h <= 0)
        unreachable();
    for(; i < h; i++) {
        pattern_word = pattern[i & 0x000f];
        switch (pattern_word) {
        case 0xffff:
            for(j = w - 1; j >= 0; j--) {
                v = ~*addr;
                *addr = v;
                addr++;
            }
            break;
        default:
            mask = x;
            for(j = w - 1; j >= 0; j--) {
                if (pattern_word & mask) {
                    v = ~*addr;
                    *addr = v;
                    addr++;
                } else {
                    addr++;
                }
                if (!(mask >>= 1))
                    mask = 0x8000;
            }
            break;
        }
        addr += line_add;
    }
}

static void fill_revtransp(PIXEL *addr, PIXEL *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned short pattern_word, mask;

    (void) addr_fast;
    (void) background;
    i = y;
    h = y + h;
    x = 1 << (15 - (x & 0x000f));

    if (w <= 0 || h <= 0)
        unreachable();
    for(; i < h; i++) {
        pattern_word = pattern[i & 0x000f];
        switch (pattern_word) {
        case 0x0000:
            for(j = w - 1; j >= 0; j--) {
                *addr = foreground;
                addr++;
            }
            break;
        default:
            mask = x;
            for(j = w - 1; j >= 0; j--) {
                if (!(pattern_word & mask)) {
                    *addr = foreground;
                    addr++;
                } else {
                    addr++;
                }
                if (!(mask >>= 1))
                    mask = 0x8000;
            }
            break;
        }
        addr += line_add;
    }
}

long CDECL c_fill_area(Virtual *vwk, long x, long y, long w, long h,
                       short *pattern, long colour, long mode, long interior_style)
{
    Workstation *wk;
    PIXEL *addr, *addr_fast;
    unsigned long foreground, background;
    long line_add;
    ULONG pos;
    ULONG fb_start, fb_end;
    ULONG dst_first, dst_last;
    long x1, y1;
    static short solid_pattern[16] = {
        0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff,
        0xffff, 0xffff, 0xffff, 0xffff
    };
    short *table;

    if (w <= 0 || h <= 0)
        return 1;

    (void) interior_style;
    table = 0;
    if ((long) vwk & 1) {
        if ((y & 0xffff) != 0)
            return -1;
        table = (short *)x;
        (void) table;
        h = (y >> 16) & 0xffff;
        vwk = (Virtual *)((long)vwk - 1);
        return -1;
    }

    if (!vwk) {
        return -1;
    }

    c_get_colours(vwk, colour, &foreground, &background);

    wk = vwk->real_address;
    if (!wk || !wk->screen.mfdb.address) {
        return -1;
    }

    if (!pattern) {
        pattern = solid_pattern;
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
        return 1;
    }

    x1 = x + w - 1;
    y1 = y + h - 1;

    fb_start = (ULONG)wk->screen.mfdb.address;
    fb_end = fb_start + (ULONG)wk->screen.wrap * (ULONG)wk->screen.mfdb.height;
    dst_first = fb_start + (ULONG)y * (ULONG)wk->screen.wrap + (ULONG)x * PIXEL_SIZE;
    dst_last = fb_start + (ULONG)y1 * (ULONG)wk->screen.wrap + (ULONG)x1 * PIXEL_SIZE;
    if ((dst_first & 1UL) || (dst_last & 1UL) || dst_first < fb_start || dst_last >= fb_end || dst_last < dst_first) {
        my_kprintf("CP+08m d=%lX..%lX fb=%lX..%lX\n", dst_first, dst_last, fb_start, fb_end);
        return -1;
    }

    pos = (ULONG)y * (ULONG)wk->screen.wrap + (ULONG)x * 2UL;
    addr = wk->screen.mfdb.address;
    line_add = (wk->screen.wrap - w * 2) >> 1;

    addr += pos / PIXEL_SIZE;
    switch (mode) {
    case 1:
        fill_replace(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
        break;
    case 2:
        fill_transparent(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
        break;
    case 3:
        fill_xor(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
        break;
    case 4:
        fill_revtransp(addr, addr_fast, line_add, pattern, x, y, w, h, foreground, background);
        break;
    }

    return 1;
}
