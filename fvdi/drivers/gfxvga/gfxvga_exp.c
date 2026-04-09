//#define FVDI_DEBUG 1

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define FVDI_DEBUG 1
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
    volatile unsigned long canary = 0x7B91C42DUL;
    unsigned long sp_entry, sp_exit;
    unsigned long ret_entry, ret_exit;
    ULONG src_base;
    ULONG dst_base;
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    int src_words_used;
    unsigned long src_pos, dst_pos;
    ULONG dst_first, dst_last;
    ULONG fb_start, fb_end;
    ULONG vram_start, vram_end;
    long src_max_w, src_max_h;
    long dst_max_w, dst_max_h;
    int to_screen;
    long src_bits_per_row;

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_entry));
    ret_entry = (unsigned long)__builtin_return_address(0);
    my_kprintf("CP+07e ra=%lX sp=%lX\n", ret_entry, sp_entry);

    GFX_CP_ENTER(CP_EXPAND_AREA);

    if (!vwk || ((long)vwk & 1) || !src || !src->address || w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    wk = vwk->real_address;
    if (!wk || !wk->screen.mfdb.address) {
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    src_max_w = src->width;
    src_max_h = src->height;

    if (src_x < 0) {
        dst_x -= src_x;
        w += src_x;
        src_x = 0;
    }
    if (src_y < 0) {
        dst_y -= src_y;
        h += src_y;
        src_y = 0;
    }
    if (dst_x < 0) {
        src_x -= dst_x;
        w += dst_x;
        dst_x = 0;
    }
    if (dst_y < 0) {
        src_y -= dst_y;
        h += dst_y;
        dst_y = 0;
    }

    c_get_colours(vwk, colour, &foreground, &background);

    to_screen = 0;
    if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address)) {       /* To screen? */
        dst_wrap = wk->screen.wrap;
        dst_addr = wk->screen.mfdb.address;
        dst_max_w = wk->screen.mfdb.width;
        dst_max_h = wk->screen.mfdb.height;
        to_screen = 1;
    } else {
        dst_wrap = (long)dst->wdwidth * 2 * dst->bitplanes;
        dst_addr = dst->address;
        dst_max_w = dst->width;
        dst_max_h = dst->height;
    }

    src_bits_per_row = (long)src->wdwidth * 16;
    if (src_x + w > src_max_w) {
        w = src_max_w - src_x;
    }
    if (src_x + w > src_bits_per_row) {
        w = src_bits_per_row - src_x;
    }
    if (src_y + h > src_max_h) {
        h = src_max_h - src_y;
    }
    if (dst_x + w > dst_max_w) {
        w = dst_max_w - dst_x;
    }
    if (dst_y + h > dst_max_h) {
        h = dst_max_h - dst_y;
    }

    if (w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return 1;
    }

    src_base = (ULONG)src->address;
    dst_base = (ULONG)dst_addr;
    if ((src_base & 1UL) || (dst_base & 1UL)) {
        DPRINTF(("c_expand_area: odd base alignment src=%lX dst=%lX\n\r", src_base, dst_base));
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    src_wrap = (long)src->wdwidth * 2;      /* Always monochrome */
    src_addr = src->address;
    src_pos = (short)src_y * (long)src_wrap + (src_x >> 4) * 2;
    src_words_used = ((src_x & 0x000f) + w + 15) >> 4;
    if (src_words_used <= 0 || src_words_used > src->wdwidth) {
        DPRINTF(("c_expand_area: invalid src_words_used=%d wdwid=%d src_x=%ld w=%ld\n\r", src_words_used, src->wdwidth, src_x, w));
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }
    src_line_add = src_wrap - src_words_used * 2;

    dst_pos = (short)dst_y * (long)dst_wrap + dst_x * PIXEL_SIZE;
    dst_line_add = dst_wrap - w * PIXEL_SIZE;

    src_addr += src_pos / 2;
    dst_addr += dst_pos / PIXEL_SIZE;
    src_line_add /= 2;
    dst_line_add /= PIXEL_SIZE;         /* Change into pixel count */

    if (to_screen) {
        fb_start = (ULONG)wk->screen.mfdb.address;
        fb_end = fb_start + (ULONG)wk->screen.wrap * (ULONG)wk->screen.mfdb.height;
        vram_start = (ULONG)g_vdp_memory_base;
        vram_end = vram_start + 0x00100000UL;
        dst_first = (ULONG)dst_addr;
        dst_last = dst_first + (ULONG)(h - 1) * (ULONG)dst_wrap + (ULONG)(w - 1) * PIXEL_SIZE;
        if (dst_first < fb_start || dst_last >= fb_end || dst_last < dst_first) {
            my_kprintf("CP+07m d=%lX..%lX fb=%lX..%lX\n", dst_first, dst_last, fb_start, fb_end);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
        if (dst_first < vram_start || dst_last >= vram_end) {
            my_kprintf("CP+07w d=%lX..%lX vram=%lX..%lX\n", dst_first, dst_last, vram_start, vram_end);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
    }

    dst_addr_fast = wk->screen.shadow.address;  /* May not really be to screen at all, but... */

    my_kprintf("CP+07a\n");

    if (0 && !(src_x & 0x000f)) {
        switch (operation) {
        case 1:             /* Replace */
            replace_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground, background);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return 1;
        case 2:             /* Transparent */
            transparent_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return 1;
        case 3:             /* XOR */
            xor_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return 1;
        case 4:             /* Reverse transparent */
            revtransp_aligned(src_addr, src_line_add, dst_addr, dst_line_add, w, h, foreground);
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return 1;
        }
    }

    switch (operation) {
    case 1:             /* Replace */
        my_kprintf("CP+07r\n");
        replace(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 2:             /* Transparent */
        my_kprintf("CP+07t\n");
        transparent(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 3:             /* XOR */
        my_kprintf("CP+07x\n");
        xor(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    case 4:             /* Reverse transparent */
        my_kprintf("CP+07v\n");
        revtransp(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        break;
    }

    if (canary != 0x7B91C42DUL) {
        my_kprintf("CP+07k\n");
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_exit));
    ret_exit = (unsigned long)__builtin_return_address(0);
    my_kprintf("CP+07f ra=%lX sp=%lX\n", ret_exit, sp_exit);
    if (sp_entry != sp_exit || ret_entry != ret_exit) {
        my_kprintf("CP+07s sp=%lX->%lX ra=%lX->%lX\n", sp_entry, sp_exit, ret_entry, ret_exit);
    }

    (void) to_screen;
    (void) dst_addr_fast;
    GFX_CP_EXIT(CP_EXPAND_AREA);
    return 1;       /* Return as completed */
}
