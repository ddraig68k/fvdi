//#define FVDI_DEBUG 1

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

#define WORD_BITS 16

static void replace_aligned(short *src_addr, int src_line_add, PIXEL *dst_addr, int dst_line_add, int w, int h, PIXEL foreground, PIXEL background)
{
    int i;
    int full_words = w >> 4;
    int rem_bits = w & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;
        int j;

        for (j = full_words - 1; j >= 0; j--) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            if (!expand_word) {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    *dst++ = background;
                }
            } else if (expand_word == 0xffff) {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    *dst++ = foreground;
                }
            } else {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    if (expand_word & 0x8000) {
                        *dst++ = foreground;
                    } else {
                        *dst++ = background;
                    }
                    expand_word <<= 1;
                }
            }
        }

        if (rem_bits) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            for (k = rem_bits - 1; k >= 0; k--) {
                if (expand_word & 0x8000) {
                    *dst++ = foreground;
                } else {
                    *dst++ = background;
                }
                expand_word <<= 1;
            }
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void transparent_aligned(short *src_addr, int src_line_add, PIXEL *dst_addr, int dst_line_add, int w, int h, PIXEL foreground)
{
    int i;
    int full_words = w >> 4;
    int rem_bits = w & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;
        int j;

        for (j = full_words - 1; j >= 0; j--) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            if (!expand_word) {
                dst += WORD_BITS;
            } else if (expand_word == 0xffff) {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    *dst++ = foreground;
                }
            } else {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    if (expand_word & 0x8000) {
                        *dst = foreground;
                    }
                    dst++;
                    expand_word <<= 1;
                }
            }
        }

        if (rem_bits) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            for (k = rem_bits - 1; k >= 0; k--) {
                if (expand_word & 0x8000) {
                    *dst = foreground;
                }
                dst++;
                expand_word <<= 1;
            }
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void xor_aligned(short *src_addr, int src_line_add, PIXEL *dst_addr, int dst_line_add, int w, int h)
{
    int i;
    int full_words = w >> 4;
    int rem_bits = w & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;
        int j;

        for (j = full_words - 1; j >= 0; j--) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            if (!expand_word) {
                dst += WORD_BITS;
            } else if (expand_word == 0xffff) {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    *dst = ~*dst;
                    dst++;
                }
            } else {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    if (expand_word & 0x8000) {
                        *dst = ~*dst;
                    }
                    dst++;
                    expand_word <<= 1;
                }
            }
        }

        if (rem_bits) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            for (k = rem_bits - 1; k >= 0; k--) {
                if (expand_word & 0x8000) {
                    *dst = ~*dst;
                }
                dst++;
                expand_word <<= 1;
            }
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void revtransp_aligned(short *src_addr, int src_line_add, PIXEL *dst_addr, int dst_line_add, int w, int h, PIXEL foreground)
{
    int i;
    int full_words = w >> 4;
    int rem_bits = w & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;
        int j;

        for (j = full_words - 1; j >= 0; j--) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            if (expand_word == 0xffff) {
                dst += WORD_BITS;
            } else if (!expand_word) {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    *dst++ = foreground;
                }
            } else {
                for (k = WORD_BITS - 1; k >= 0; k--) {
                    if (!(expand_word & 0x8000)) {
                        *dst = foreground;
                    }
                    dst++;
                    expand_word <<= 1;
                }
            }
        }

        if (rem_bits) {
            unsigned int expand_word = (unsigned short)*src++;
            int k;

            for (k = rem_bits - 1; k >= 0; k--) {
                if (!(expand_word & 0x8000)) {
                    *dst = foreground;
                }
                dst++;
                expand_word <<= 1;
            }
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
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

static void replace(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    int shift;
    int bit_count;
    unsigned int expand_word;

    (void) dst_addr_fast;
    shift = x & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;

        expand_word = (unsigned short)*src++;
        expand_word <<= shift;
        bit_count = WORD_BITS - shift;

        for(j = w - 1; j >= 0; j--) {
            if (!bit_count) {
                expand_word = (unsigned short)*src++;
                bit_count = WORD_BITS;
            }

            if (expand_word & 0x8000) {
                *dst++ = foreground;
            } else {
                *dst++ = background;
            }

            expand_word <<= 1;
            bit_count--;
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void transparent(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    int shift;
    int bit_count;
    unsigned int expand_word;

    (void) dst_addr_fast;
    (void) background;

    shift = x & 0x000f;
    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;

        expand_word = (unsigned short)*src++;
        expand_word <<= shift;
        bit_count = WORD_BITS - shift;

        for(j = w - 1; j >= 0; j--) {
            if (!bit_count) {
                expand_word = (unsigned short)*src++;
                bit_count = WORD_BITS;
            }

            if (expand_word & 0x8000) {
                *dst++ = foreground;
            } else {
                dst++;
            }

            expand_word <<= 1;
            bit_count--;
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void xor(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    int shift;
    int bit_count;
    unsigned int expand_word;

    (void) dst_addr_fast;
    (void) foreground;
    (void) background;
    shift = x & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;

        expand_word = (unsigned short)*src++;
        expand_word <<= shift;
        bit_count = WORD_BITS - shift;

        for(j = w - 1; j >= 0; j--) {
            if (!bit_count) {
                expand_word = (unsigned short)*src++;
                bit_count = WORD_BITS;
            }

            if (expand_word & 0x8000) {
                *dst = ~*dst;
                dst++;
            } else {
                dst++;
            }

            expand_word <<= 1;
            bit_count--;
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

static void revtransp(short *src_addr, int src_line_add, PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add, int x, int w, int h, PIXEL foreground, PIXEL background)
{
    int i, j;
    int shift;
    int bit_count;
    unsigned int expand_word;

    (void) dst_addr_fast;
    (void) background;
    shift = x & 0x000f;

    for(i = h - 1; i >= 0; i--) {
        short *src = src_addr;
        PIXEL *dst = dst_addr;

        expand_word = (unsigned short)*src++;
        expand_word <<= shift;
        bit_count = WORD_BITS - shift;

        for(j = w - 1; j >= 0; j--) {
            if (!bit_count) {
                expand_word = (unsigned short)*src++;
                bit_count = WORD_BITS;
            }

            if (!(expand_word & 0x8000)) {
                *dst++ = foreground;
            } else {
                dst++;
            }

            expand_word <<= 1;
            bit_count--;
        }

        src_addr = src + src_line_add;
        dst_addr = dst + dst_line_add;
    }
}

long CDECL c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour)
{
    Workstation *wk;
    PIXEL *src_addr, *dst_addr, *dst_addr_fast;
    unsigned long foreground, background;
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    int src_words_used;
    unsigned long src_pos, dst_pos;
    int to_screen;

    wk = vwk->real_address;

    c_get_colours(vwk, colour, &foreground, &background);

    src_wrap = (long)src->wdwidth * 2;      /* Always monochrome */
    src_addr = src->address;
    src_pos = (short)src_y * (long)src_wrap + (src_x >> 4) * 2;
    src_words_used = ((src_x & 0x000f) + w + 15) >> 4;
    src_line_add = src_wrap - src_words_used * 2;

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

    if (!(src_x & 0x000f)) {
        switch (operation) {
        case 1:             /* Replace */
            replace_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground, background);
            return 1;
        case 2:             /* Transparent */
            transparent_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground);
            return 1;
        case 3:             /* XOR */
            xor_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h);
            return 1;
        case 4:             /* Reverse transparent */
            revtransp_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground);
            return 1;
        }
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
