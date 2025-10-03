//#define FVDI_DEBUG 1

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

static void replace(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned int expand_word, mask;

    (void) dst_addr_fast;
    x = 1 << (15 - (x & 0x000f));

    for(i = h - 1; i >= 0; i--) {
        expand_word = *src_addr++;
        mask = x;
        for(j = w - 1; j >= 0; j--) {
            if (expand_word & mask) {
                *dst_addr++ = foreground;
            } else {
                *dst_addr++ = background;
            }
            if (!(mask >>= 1)) {
                mask = 0x8000;
                expand_word = *src_addr++;
            }
        }
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}

static void transparent(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned int expand_word, mask;

    (void) dst_addr_fast;
    (void) background;
    x = 1 << (15 - (x & 0x000f));

    for(i = h - 1; i >= 0; i--) {
        expand_word = *src_addr++;
        mask = x;
        for(j = w - 1; j >= 0; j--) {
            if (expand_word & mask) {
                *dst_addr++ = foreground;
            } else {
                dst_addr++;
            }
            if (!(mask >>= 1)) {
                mask = 0x8000;
                expand_word = *src_addr++;
            }
        }
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}

static void xor(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j, v;
    unsigned int expand_word, mask;

    (void) dst_addr_fast;
    (void) foreground;
    (void) background;
    x = 1 << (15 - (x & 0x000f));

    for(i = h - 1; i >= 0; i--) {
        expand_word = *src_addr++;
        mask = x;
        for(j = w - 1; j >= 0; j--) {
            if (expand_word & mask) {
                v = ~*dst_addr;
                *dst_addr++ = v;
            } else {
                dst_addr++;
            }
            if (!(mask >>= 1)) {
                mask = 0x8000;
                expand_word = *src_addr++;
            }
        }
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}

static void revtransp(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    unsigned int expand_word, mask;

    (void) dst_addr_fast;
    (void) background;
    x = 1 << (15 - (x & 0x000f));

    for(i = h - 1; i >= 0; i--) {
        expand_word = *src_addr++;
        mask = x;
        for(j = w - 1; j >= 0; j--) {
            if (!(expand_word & mask)) {
                *dst_addr++ = foreground;
            } else {
                dst_addr++;
            }
            if (!(mask >>= 1)) {
                mask = 0x8000;
                expand_word = *src_addr++;
            }
        }
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}

long CDECL c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour)
{
    Workstation *wk;
    PIXEL *src_addr, *dst_addr, *dst_addr_fast;
    unsigned long foreground, background;
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    unsigned long src_pos, dst_pos;
    int to_screen;

    wk = vwk->real_address;

    DPRINTF(("GfxVGA: c_expand_area: sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));

    c_get_colours(vwk, colour, &foreground, &background);

    src_wrap = (long)src->wdwidth * 2;      /* Always monochrome */
    src_addr = src->address;
    src_pos = (short)src_y * (long)src_wrap + (src_x >> 4) * 2;
    src_line_add = src_wrap - (((src_x + w) >> 4) - (src_x >> 4) + 1) * 2;

    to_screen = 0;
    if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {       /* To screen? */
        dst_wrap = wk->screen.wrap;
        dst_addr = wk->screen.mfdb.address;
        to_screen = 1;
    } else {
        dst_wrap = (long)dst->wdwidth * 2 * dst->bitplanes;
        dst_addr = dst->address;
    }
    dst_pos = (short)dst_y * (long)dst_wrap + dst_x * PIXEL_SIZE;
    dst_line_add = dst_wrap - w * PIXEL_SIZE;

    src_addr += src_pos / 2;
    dst_addr += dst_pos / PIXEL_SIZE;
    src_line_add /= 2;
    dst_line_add /= PIXEL_SIZE;         /* Change into pixel count */

    dst_addr_fast = wk->screen.shadow.address;  /* May not really be to screen at all, but... */

    switch (operation) {
    case 1:             /* Replace */
        replace(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 2:             /* Transparent */
        transparent(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 3:             /* XOR */
        xor(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 4:             /* Reverse transparent */
        revtransp(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    }
    (void) to_screen;
    (void) dst_addr_fast;
    return 1;       /* Return as completed */
}
