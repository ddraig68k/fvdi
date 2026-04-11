//#define FVDI_DEBUG 1

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

#define WORD_BITS 16

static ULONG expand_src_guard_lo;
static ULONG expand_src_guard_hi;
static ULONG expand_dst_guard_lo;
static ULONG expand_dst_guard_hi;
static int expand_guard_src_words;
static int expand_guard_width;
static int expand_guard_fault;
static int expand_busy;

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
        ULONG src_row_first;
        ULONG src_row_last;
        ULONG dst_row_first;
        ULONG dst_row_last;

        src_row_first = (ULONG)src;
        src_row_last = src_row_first + (ULONG)(expand_guard_src_words - 1) * 2UL;
        dst_row_first = (ULONG)dst;
        dst_row_last = dst_row_first + (ULONG)(expand_guard_width - 1) * PIXEL_SIZE;
        if ((src_row_first & 1UL) || (src_row_last & 1UL) ||
            (dst_row_first & 1UL) || (dst_row_last & 1UL) ||
            src_row_first < expand_src_guard_lo || src_row_last > expand_src_guard_hi ||
            dst_row_first < expand_dst_guard_lo || dst_row_last > expand_dst_guard_hi) {
            my_kprintf("CP+07p r=%d s=%lX..%lX d=%lX..%lX\n", h - 1 - i, src_row_first, src_row_last, dst_row_first, dst_row_last);
            expand_guard_fault = 1;
            return;
        }

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
        ULONG src_row_first;
        ULONG src_row_last;
        ULONG dst_row_first;
        ULONG dst_row_last;

        src_row_first = (ULONG)src;
        src_row_last = src_row_first + (ULONG)(expand_guard_src_words - 1) * 2UL;
        dst_row_first = (ULONG)dst;
        dst_row_last = dst_row_first + (ULONG)(expand_guard_width - 1) * PIXEL_SIZE;
        if ((src_row_first & 1UL) || (src_row_last & 1UL) ||
            (dst_row_first & 1UL) || (dst_row_last & 1UL) ||
            src_row_first < expand_src_guard_lo || src_row_last > expand_src_guard_hi ||
            dst_row_first < expand_dst_guard_lo || dst_row_last > expand_dst_guard_hi) {
            my_kprintf("CP+07p r=%d s=%lX..%lX d=%lX..%lX\n", h - 1 - i, src_row_first, src_row_last, dst_row_first, dst_row_last);
            expand_guard_fault = 1;
            return;
        }

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
        ULONG src_row_first;
        ULONG src_row_last;
        ULONG dst_row_first;
        ULONG dst_row_last;

        src_row_first = (ULONG)src;
        src_row_last = src_row_first + (ULONG)(expand_guard_src_words - 1) * 2UL;
        dst_row_first = (ULONG)dst;
        dst_row_last = dst_row_first + (ULONG)(expand_guard_width - 1) * PIXEL_SIZE;
        if ((src_row_first & 1UL) || (src_row_last & 1UL) ||
            (dst_row_first & 1UL) || (dst_row_last & 1UL) ||
            src_row_first < expand_src_guard_lo || src_row_last > expand_src_guard_hi ||
            dst_row_first < expand_dst_guard_lo || dst_row_last > expand_dst_guard_hi) {
            my_kprintf("CP+07p r=%d s=%lX..%lX d=%lX..%lX\n", h - 1 - i, src_row_first, src_row_last, dst_row_first, dst_row_last);
            expand_guard_fault = 1;
            return;
        }

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
        ULONG src_row_first;
        ULONG src_row_last;
        ULONG dst_row_first;
        ULONG dst_row_last;

        src_row_first = (ULONG)src;
        src_row_last = src_row_first + (ULONG)(expand_guard_src_words - 1) * 2UL;
        dst_row_first = (ULONG)dst;
        dst_row_last = dst_row_first + (ULONG)(expand_guard_width - 1) * PIXEL_SIZE;
        if ((src_row_first & 1UL) || (src_row_last & 1UL) ||
            (dst_row_first & 1UL) || (dst_row_last & 1UL) ||
            src_row_first < expand_src_guard_lo || src_row_last > expand_src_guard_hi ||
            dst_row_first < expand_dst_guard_lo || dst_row_last > expand_dst_guard_hi) {
            my_kprintf("CP+07p r=%d s=%lX..%lX d=%lX..%lX\n", h - 1 - i, src_row_first, src_row_last, dst_row_first, dst_row_last);
            expand_guard_fault = 1;
            return;
        }

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
    ULONG dst_total_bytes;
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    int src_words_used;
    unsigned long src_pos, dst_pos;
    ULONG src_total_bytes;
    ULONG src_first, src_last;
    ULONG dst_first, dst_last;
    ULONG fb_start, fb_end;
    ULONG vram_start, vram_end;
    long src_max_w, src_max_h;
    long dst_max_w, dst_max_h;
    int to_screen;
    long src_bits_per_row;
    int locked;

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_entry));
    ret_entry = (unsigned long)__builtin_return_address(0);
    my_kprintf("CP+07e ra=%lX sp=%lX\n", ret_entry, sp_entry);
    locked = 0;

    GFX_CP_ENTER(CP_EXPAND_AREA);

    if (!vwk || ((long)vwk & 1) || !src || !src->address || w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    /* Temporary containment: crashes consistently cluster around CP+07t.
     * Route transparent expand to fallback until root cause is identified. */
    if (operation == 2) {
        my_kprintf("CP+07z\n");
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

    /* Containment mode: only accelerate writes directly to the visible framebuffer.
     * Off-screen MFDB handling has been implicated in random corruption traces. */
    if (!to_screen) {
        my_kprintf("CP+07q\n");
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
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
    if (src->wdwidth <= 0 || src->height <= 0 || src_wrap <= 0 || (src_wrap & 1)) {
        my_kprintf("CP+07u badsrc wd=%d h=%d wrap=%d\n", src->wdwidth, src->height, src_wrap);
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }
    src_addr = src->address;
    src_pos = (ULONG)src_y * (ULONG)src_wrap + (ULONG)(src_x >> 4) * 2UL;
    src_words_used = ((src_x & 0x000f) + w + 15) >> 4;
    if (src_words_used <= 0 || src_words_used > src->wdwidth) {
        DPRINTF(("c_expand_area: invalid src_words_used=%d wdwid=%d src_x=%ld w=%ld\n\r", src_words_used, src->wdwidth, src_x, w));
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }
    src_total_bytes = (ULONG)(unsigned short)src->height * (ULONG)src_wrap;
    src_first = src_base + src_pos;
    src_last = src_first + (ULONG)(h - 1) * (ULONG)src_wrap + (ULONG)(src_words_used - 1) * 2UL;
    if ((src_first & 1UL) || (src_last & 1UL) ||
        src_last < src_first || src_first < src_base ||
        src_last >= (src_base + src_total_bytes)) {
        my_kprintf("CP+07u s=%lX..%lX lim=%lX..%lX\n", src_first, src_last, src_base, src_base + src_total_bytes);
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }
    src_line_add = src_wrap - src_words_used * 2;

    if (dst_wrap <= 0 || (dst_wrap & 1)) {
        my_kprintf("CP+07n wrap=%d dbase=%lX\n", dst_wrap, dst_base);
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    dst_pos = (ULONG)dst_y * (ULONG)dst_wrap + (ULONG)dst_x * PIXEL_SIZE;
    dst_line_add = dst_wrap - w * PIXEL_SIZE;

    src_addr += src_pos / 2;
    dst_addr += dst_pos / PIXEL_SIZE;
    src_line_add /= 2;
    dst_line_add /= PIXEL_SIZE;         /* Change into pixel count */

    dst_first = (ULONG)dst_addr;
    dst_last = dst_first + (ULONG)(h - 1) * (ULONG)dst_wrap + (ULONG)(w - 1) * PIXEL_SIZE;
    dst_total_bytes = (ULONG)dst_wrap * (ULONG)dst_max_h;
    if ((dst_first & 1UL) || (dst_last & 1UL) ||
        dst_last < dst_first || dst_first < dst_base ||
        dst_last >= (dst_base + dst_total_bytes)) {
        my_kprintf("CP+07n d=%lX..%lX lim=%lX..%lX\n", dst_first, dst_last, dst_base, dst_base + dst_total_bytes);
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    if (to_screen) {
        fb_start = (ULONG)wk->screen.mfdb.address;
        fb_end = fb_start + (ULONG)wk->screen.wrap * (ULONG)wk->screen.mfdb.height;
        vram_start = (ULONG)g_vdp_memory_base;
        vram_end = vram_start + 0x00100000UL;
        if (dst_first < fb_start || dst_last >= fb_end) {
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

    if (expand_busy) {
        my_kprintf("CP+07b\n");
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }
    expand_busy = 1;
    locked = 1;

    dst_addr_fast = wk->screen.shadow.address;  /* May not really be to screen at all, but... */
    expand_src_guard_lo = src_first;
    expand_src_guard_hi = src_last;
    expand_dst_guard_lo = dst_first;
    expand_dst_guard_hi = dst_last;
    expand_guard_src_words = src_words_used;
    expand_guard_width = (int)w;
    expand_guard_fault = 0;

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
        if (expand_guard_fault) {
            if (locked)
                expand_busy = 0;
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
        break;
    case 2:             /* Transparent */
        my_kprintf("CP+07t\n");
        transparent(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        if (expand_guard_fault) {
            if (locked)
                expand_busy = 0;
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
        break;
    case 3:             /* XOR */
        my_kprintf("CP+07x\n");
        xor(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        if (expand_guard_fault) {
            if (locked)
                expand_busy = 0;
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
        break;
    case 4:             /* Reverse transparent */
        my_kprintf("CP+07v\n");
        revtransp(src_addr, src_line_add, dst_addr, 0, dst_line_add, src_x, w, h, foreground, background);
        if (expand_guard_fault) {
            if (locked)
                expand_busy = 0;
            GFX_CP_EXIT(CP_EXPAND_AREA);
            return -1;
        }
        break;
    }

    if (canary != 0x7B91C42DUL) {
        my_kprintf("CP+07k\n");
        if (locked)
            expand_busy = 0;
        GFX_CP_EXIT(CP_EXPAND_AREA);
        return -1;
    }

    __asm__ volatile("move.l %%sp,%0" : "=r"(sp_exit));
    ret_exit = (unsigned long)__builtin_return_address(0);
    my_kprintf("CP+07f ra=%lX sp=%lX\n", ret_exit, sp_exit);
    if (sp_entry != sp_exit || ret_entry != ret_exit) {
        my_kprintf("CP+07s sp=%lX->%lX ra=%lX->%lX\n", sp_entry, sp_exit, ret_entry, ret_exit);
    }
    if (locked)
        expand_busy = 0;

    (void) to_screen;
    (void) dst_addr_fast;
    GFX_CP_EXIT(CP_EXPAND_AREA);
    return 1;       /* Return as completed */
}
