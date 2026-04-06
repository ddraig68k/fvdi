//#define FVDI_DEBUG 1

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)
#define CUSTOM_PATTERN_SLOT 39

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

static UWORD extract_mono_row_bits(const short *row_base, int bit_offset)
{
    UWORD first = (UWORD)row_base[0];

    if (bit_offset == 0)
        return first;

    return (UWORD)((first << bit_offset) | ((UWORD)row_base[1] >> (16 - bit_offset)));
}

static int expand_via_pattern(Virtual *vwk, MFDB *src, long src_x, long src_y,
                              long dst_x, long dst_y, long w, long h,
                              long operation, UWORD foreground, UWORD background)
{
    const short *src_base;
    int src_row_words;
    int bit_offset;
    UWORD mode;

    if (!src || !src->address || src->bitplanes != 1)
        return 0;
    if ((w != 8 || h != 8) && (w != 16 || h != 16))
        return 0;
    if (operation != 1 && operation != 2)
        return 0;

    src_row_words = src->wdwidth;
    src_base = (const short *)src->address + src_y * src_row_words + (src_x >> 4);
    bit_offset = (int)(src_x & 0x000f);

    mode = (operation == 1) ? DRAW_MODE_SOLID : DRAW_MODE_TRANS;

    vdp_set_drawmode(mode);
    vdp_set_draw_color(foreground);
    vdp_set_back_color(background);

    if (w == 8 && h == 8) {
        UWORD rows[8];
        int row;

        for (row = 0; row < 8; row++) {
            UWORD bits = extract_mono_row_bits(src_base + row * src_row_words, bit_offset);
            rows[row] = bits & 0xFF00;
        }

        vdp_upload_pattern8(CUSTOM_PATTERN_SLOT, rows);
    } else {
        UWORD rows[16];
        int row;

        for (row = 0; row < 16; row++)
            rows[row] = extract_mono_row_bits(src_base + row * src_row_words, bit_offset);

        vdp_upload_pattern16(CUSTOM_PATTERN_SLOT, rows);
    }

    vdp_set_pattern(VDP_PATTERN_RELATIVE | CUSTOM_PATTERN_SLOT);
    vdp_draw_fill_rect((UWORD)dst_x, (UWORD)dst_y,
                       (UWORD)(dst_x + w - 1), (UWORD)(dst_y + h - 1));

    (void)vwk;
    return 1;
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

    DPRINTF(("c_expand_area: src MFDB addr=%lX w=%d, h=%d, wdwid=%d, std=%d, bitpl=%d\n\r", (ULONG)src->address, src->width, src->height, src->wdwidth, src->standard, src->bitplanes));
    DPRINTF(("c_expand_area: dst MFDB addr=%lX w=%d, h=%d, wdwid=%d, std=%d, bitpl=%d\n\r", (ULONG)dst->address, dst->width, dst->height, dst->wdwidth, dst->standard, dst->bitplanes));

    if (to_screen && expand_via_pattern(vwk, src, src_x, src_y, dst_x, dst_y, w, h,
                                        operation, (UWORD)foreground, (UWORD)background)) {
        DPRINTF(("c_expand_area: mode=pattern_fast srx=%ld,sry=%ld,dx=%ld,dy=%ld,w=%ld,h=%ld,op=%ld\n\r",
                 src_x, src_y, dst_x, dst_y, w, h, operation));
        return 1;
    }

    switch (operation) {
    case 1:             /* Replace */
        DPRINTF(("c_expand_area: mode=replace saddr=%lX,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
        replace(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 2:             /* Transparent */
        DPRINTF(("c_expand_area: mode=trans saddr=%lx,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
        transparent(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 3:             /* XOR */
        DPRINTF(("c_expand_area: mode=xor saddr=%lx,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
        xor(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 4:             /* Reverse transparent */
        DPRINTF(("c_expand_area: mode=revtrans saddr=%lx,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
        revtransp(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    }
    (void) to_screen;
    (void) dst_addr_fast;
    return 1;       /* Return as completed */
}
