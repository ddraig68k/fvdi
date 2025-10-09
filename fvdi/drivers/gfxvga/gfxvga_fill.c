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
#define PIXEL_32    long

static int is_solid_pattern(const UWORD *pattern)
{
    for (int i = 0; i < 16; i++) {
        DPRINTF(("pattern %d = %04X\n\r", i, pattern[i]));
        if (pattern[i] != 0xFFFF)
            return 0;
    }
    return 1;
}

/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

static void fill_replace(PIXEL *addr, PIXEL *addr_fast, int line_add, short *pattern, int x, int y, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned short pattern_word, mask;

    (void) addr_fast;
    i = y;
    h = y + h;
    x = 1 << (15 - (x & 0x000f));

    /* Tell gcc that this cannot happen (already checked in c_fill_area() below) */
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

    /* Tell gcc that this cannot happen (already checked in c_fill_area() below) */
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

    /* Tell gcc that this cannot happen (already checked in c_fill_area() below) */
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

    /* Tell gcc that this cannot happen (already checked in c_fill_area() below) */
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
    unsigned long pos;
    short *table;

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

    wk = vwk->real_address;

    int16_t fill_type = (int16_t)((interior_style >> 16) & 0xFFFF);
    int16_t pattern_index = (int16_t)(interior_style & 0xFFFF);
    int solid_pattern = (fill_type == 0) || (fill_type == 1) || ((fill_type == 2) && (pattern_index == 8)) 
        || ((fill_type == 2) && (pattern_index == 4)) || is_solid_pattern(pattern);

    UWORD draw_mode = 0;
    if (fill_type > 1)
        draw_mode = DRAW_PATTERN(pattern_index + 2);

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
