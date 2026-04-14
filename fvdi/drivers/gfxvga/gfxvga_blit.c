/*
 * Implements FVDI driver API functions c_expand_area() and c_blit_area().
 */

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

//#define FVDI_DEBUG 1
#include "gfxvga.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)
#define PIXEL_32    long

#define MOVE_L " move.w "
#define ASR_L  " asr.w "
#define AND_L  " and.w "
#define OR_W(val, ptr, dec, inc)   " or.w %[" val "]," dec "(%[" ptr "])" inc "\n"
#define DBRA(reg, label) " dbra " reg "," label "\n"
#define REGL short

#define DO_OP(v) \
    switch(operation) { \
    case 0: \
    default: \
        v = 0; \
        break; \
    case 1: \
        v = v ## s & v ## d; \
        break; \
    case 2: \
        v = v ## s & ~v ## d; \
        break; \
    case 3: \
        v = v ## s; \
        break; \
    case 4: \
        v = ~v ## s & v ## d; \
        break; \
    case 5: \
        v = v ## d; \
        break; \
    case 6: \
        v = v ## s ^ v ## d; \
        break; \
    case 7: \
        v = v ## s | v ## d; \
        break; \
    case 8: \
        v = ~(v ## s | v ## d); \
        break; \
    case 9: \
        v = ~(v ## s ^ v ## d); \
        break; \
    case 10: \
        v = ~v ## d; \
        break; \
    case 11: \
        v = v ## s | ~v ## d; \
        break; \
    case 12: \
        v = ~v ## s; \
        break; \
    case 13: \
        v = ~v ## s | v ## d; \
        break; \
    case 14: \
        v = ~(v ## s & v ## d); \
        break; \
    case 15: \
        v = -1; \
        break; \
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

static void blit_copy(PIXEL *src_addr, int src_line_add,
    PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
    short int w, short int h)
{
    REGL x, y, x4, xR;
#define COPY_LOOP \
        __asm__ __volatile__( \
            MOVE_L "%[x],%[x4]\n" \
            MOVE_L "%[x],%[xR]\n" \
            ASR_L "#2,%[x4]\n" \
            AND_L "#3,%[xR]\n" \
            " jbra 2f\n" \
            "1:\n" \
            " move.l (%[src_addr])+,(%[dst_addr])+\n" \
            " move.l (%[src_addr])+,(%[dst_addr])+\n" \
            "2:\n" \
            DBRA("%[x4]","1b") \
            " jbra 4f\n" \
            "3:\n" \
            " move.w (%[src_addr])+,(%[dst_addr])+\n" \
            "4:\n" \
            DBRA("%[xR]","3b") \
            : [dst_addr]"+a"(dst_addr), \
              [src_addr]"+a"(src_addr), \
              [x4]"=d"(x4), \
              [xR]"=d"(xR) \
            : [x]"d"(x) \
            : "cc", "memory")
#define NEXTLINE \
        src_addr += src_line_add; \
        dst_addr += dst_line_add

    (void) dst_addr_fast;

    x = w;
    y = h;
    while (y--)
    {
        COPY_LOOP;
        NEXTLINE;
    }

#undef COPY_LOOP
#undef NEXTLINE
}


static void blit_or(PIXEL *src_addr, int src_line_add,
        PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
        short int w, short int h)
{
    REGL x, y, x4, xR;
    PIXEL_32 v32;

#define COPY_LOOP \
        __asm__ __volatile__( \
            MOVE_L "%[x],%[x4]\n" \
            MOVE_L "%[x],%[xR]\n" \
            ASR_L "#2,%[x4]\n" \
            AND_L "#3,%[xR]\n" \
            " jbra 2f\n" \
            "1:\n" \
            " move.l (%[src_addr])+,%[v32]\n" \
            " or.l %[v32],(%[dst_addr])+\n" \
            " move.l (%[src_addr])+,%[v32]\n" \
            " or.l %[v32],(%[dst_addr])+\n" \
            "2:\n" \
            DBRA("%[x4]","1b") \
            " jbra 4f\n" \
            "3:\n" \
            " move.w (%[src_addr])+,%[v32]\n" \
            OR_W("v32","dst_addr","","+") \
            "4:\n" \
            DBRA("%[xR]","3b") \
            : [dst_addr]"+a"(dst_addr), \
              [src_addr]"+a"(src_addr), \
              [x4]"=d"(x4), \
              [xR]"=d"(xR), \
              [v32]"=d"(v32) \
            : [x]"d"(x) \
            : "cc", "memory")
#define NEXTLINE \
        src_addr += src_line_add; \
        dst_addr += dst_line_add

    (void) dst_addr_fast;

    x = w;
    y = h;
    while (y--)
    {
        COPY_LOOP;
        NEXTLINE;
    }

#undef COPY_LOOP
#undef NEXTLINE
}


static void blit_16b(PIXEL *src_addr, int src_line_add,
     PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
     short int w, short int h, short int operation)
{
    short int i, j;
    PIXEL v, vs, vd;
    PIXEL_32 v32, v32s, v32d;
    PIXEL_32 *src_addr32;
    PIXEL_32 *dst_addr32;

    (void) dst_addr_fast;
    /* Tell gcc that this cannot happen (already checked in c_blit_area() below) */
    if (w <= 0 || h <= 0)
        unreachable();
    for (i = h - 1; i >= 0; i--)
    {
        if (w & 1)
        {
            vs = *src_addr++;
            vd = *dst_addr;
            DO_OP(v);
            *dst_addr++ = v;
        }
        src_addr32 = (PIXEL_32 *)src_addr;
        dst_addr32 = (PIXEL_32 *)dst_addr;

        for(j = (w >> 1) - 1; j >= 0; j--) {
            v32s = *src_addr32++;
            v32d = *dst_addr32;
            DO_OP(v32);
            *dst_addr32++ = v32;
        }
        src_addr = (PIXEL *)src_addr32;
        dst_addr = (PIXEL *)dst_addr32;
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}


static void
pan_backwards_copy(PIXEL *src_addr, int src_line_add,
                   PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                   short int w, short int h)
{
    REGL x, y, x4, xR;
#define COPY_LOOP \
        __asm__ __volatile__( \
            MOVE_L "%[x],%[x4]\n" \
            MOVE_L "%[x],%[xR]\n" \
            ASR_L "#2,%[x4]\n" \
            AND_L "#3,%[xR]\n" \
            " jbra 2f\n" \
            "1:\n" \
            " move.l -(%[src_addr]),-(%[dst_addr])\n" \
            " move.l -(%[src_addr]),-(%[dst_addr])\n" \
            "2:\n" \
            DBRA("%[x4]","1b") \
            " jbra 4f\n" \
            "3:\n" \
            " move.w -(%[src_addr]),-(%[dst_addr])\n" \
            "4:\n" \
            DBRA("%[xR]","3b") \
            : [dst_addr]"+a"(dst_addr), \
              [src_addr]"+a"(src_addr), \
              [x4]"=d"(x4), \
              [xR]"=d"(xR) \
            : [x]"d"(x) \
            : "cc", "memory")
#define NEXTLINE \
        src_addr += src_line_add; \
        dst_addr += dst_line_add

    (void) dst_addr_fast;

    x = w;
    y = h;
    while (y--)
    {
        COPY_LOOP;
        NEXTLINE;
    }

#undef COPY_LOOP
#undef NEXTLINE
}


static void
pan_backwards_or(PIXEL *src_addr, int src_line_add,
                 PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                 short int w, short int h)
{
    REGL x, y, x4, xR;
    PIXEL_32 v32;

#define COPY_LOOP \
        __asm__ __volatile__( \
            MOVE_L "%[x],%[x4]\n" \
            MOVE_L "%[x],%[xR]\n" \
            ASR_L "#2,%[x4]\n" \
            AND_L "#3,%[xR]\n" \
            " jbra 2f\n" \
            "1:\n" \
            " move.l -(%[src_addr]),%[v32]\n" \
            " or.l %[v32],-(%[dst_addr])\n" \
            " move.l -(%[src_addr]),%[v32]\n" \
            " or.l %[v32],-(%[dst_addr])\n" \
            "2:\n" \
            DBRA("%[x4]","1b") \
            " jbra 4f\n" \
            "3:\n" \
            " move.w -(%[src_addr]),%[v32]\n" \
            OR_W("v32","dst_addr","-","") \
            "4:\n" \
            DBRA("%[xR]","3b") \
            : [dst_addr]"+a"(dst_addr), \
              [src_addr]"+a"(src_addr), \
              [x4]"=d"(x4), \
              [xR]"=d"(xR), \
              [v32]"=d"(v32) \
            : [x]"d"(x) \
            : "cc", "memory")
#define NEXTLINE \
        src_addr += src_line_add; \
        dst_addr += dst_line_add

    (void) dst_addr_fast;

    x = w;
    y = h;
    while (y--)
    {
        COPY_LOOP;
        NEXTLINE;
    }

#undef COPY_LOOP
#undef NEXTLINE
}


static void
pan_backwards(PIXEL *src_addr, int src_line_add,
              PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
              short int w, short int h, short int operation)
{
    short int i, j;
    PIXEL v, vs, vd;
    PIXEL_32 v32, v32s, v32d;
    PIXEL_32 *src_addr32;
    PIXEL_32 *dst_addr32;
    
    (void) dst_addr_fast;
    /* Tell gcc that this cannot happen (already checked in c_blit_area() below) */
    if (w <= 0 || h <= 0)
        unreachable();
    for (i = h - 1; i >= 0; i--)
    {
        if (w & 1)
        {
            vs = *--src_addr;
            vd = *--dst_addr;
            DO_OP(v);
            *dst_addr = v;
        }
        src_addr32 = (PIXEL_32 *)src_addr;
        dst_addr32 = (PIXEL_32 *)dst_addr;
        for (j = (w >> 1) - 1; j >= 0; j--)
        {
            v32s = *--src_addr32;
            v32d = *--dst_addr32;
            DO_OP(v32);
            *dst_addr32 = v32;
        }
        src_addr = (PIXEL *)src_addr32;
        dst_addr = (PIXEL *)dst_addr32;
        src_addr += src_line_add;
        dst_addr += dst_line_add;
    }
}

#undef DO_OP

long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y,
            MFDB *dst, long dst_x, long dst_y,
            long w, long h, long operation)
{
    Workstation *wk;
    PIXEL *src_addr, *dst_addr, *dst_addr_fast;
    ULONG src_base, dst_base;
    ULONG src_pos0, dst_pos0;
    ULONG src_first, src_last, dst_first, dst_last;
    ULONG src_size_bytes, dst_size_bytes;
    long src_max_w, src_max_h, dst_max_w, dst_max_h;
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    unsigned long src_pos, dst_pos;
    int from_screen;
    int to_screen;

    GFX_CP_ENTER(CP_BLIT_AREA);

    if (w <= 0 || h <= 0) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return 1;
    }

    if (!vwk || ((long)vwk & 1)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    wk = vwk->real_address;
    if (!wk || !wk->screen.mfdb.address) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    DPRINTF(("c_blit_area: entering op=%ld sx=%ld sy=%ld dx=%ld dy=%ld w=%ld h=%ld\n", operation, src_x, src_y, dst_x, dst_y, w, h));

    if (src && ((ULONG)src & 1UL)) {
        my_kprintf("CP+09u srcmfdb=%lX\n", (ULONG)src);
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }
    if (dst && ((ULONG)dst & 1UL)) {
        my_kprintf("CP+09u dstmfdb=%lX\n", (ULONG)dst);
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    DPRINTF(("c_blit_area called: sx=%ld,sy=%ld,dx=%ld,dy=%ld w=%ld,h=%ld\n\r", src_x, src_y, dst_x, dst_y, w, h));
    DPRINTF(("c_blit_area: src MFDB addr=%lX w=%d, h=%d, wdwid=%d, std=%d, bitpl=%d\n\r",
             src ? (ULONG)src->address : 0UL,
             src ? src->width : 0,
             src ? src->height : 0,
             src ? src->wdwidth : 0,
             src ? src->standard : 0,
             src ? src->bitplanes : 0));
    DPRINTF(("c_blit_area: dst MFDB addr=%lX w=%d, h=%d, wdwid=%d, std=%d, bitpl=%d\n\r",
             dst ? (ULONG)dst->address : 0UL,
             dst ? dst->width : 0,
             dst ? dst->height : 0,
             dst ? dst->wdwidth : 0,
             dst ? dst->standard : 0,
             dst ? dst->bitplanes : 0));

    from_screen = 0;
    if (!src || !src->address || (src->address == wk->screen.mfdb.address)) {       /* From screen? */
        src_wrap = wk->screen.wrap;
        if (!(src_addr = wk->screen.shadow.address))
            src_addr = wk->screen.mfdb.address;
        src_max_w = wk->screen.mfdb.width;
        src_max_h = wk->screen.mfdb.height;
        from_screen = 1;
    } else {
        src_wrap = (long)src->wdwidth * 2 * src->bitplanes;
        src_addr = src->address;
        src_max_w = src->width;
        src_max_h = src->height;
    }

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

    if (src_wrap <= 0 || dst_wrap <= 0 || (src_wrap & 1) || (dst_wrap & 1)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

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
    if (src_x + w > src_max_w) {
        w = src_max_w - src_x;
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
        GFX_CP_EXIT(CP_BLIT_AREA);
        return 1;
    }

    /*
     * Use VDP hardware 2D copy for screen->screen plain copies.
     * Decide by MFDB identity (from_screen/to_screen), not by effective source
     * pointer, so shadow-buffer presence does not randomly disable acceleration.
     * For non-overlapping or forward-safe copies, use direct copy.
     * For overlapping unsafe-backward copies, use staging buffer (still fully HW accelerated).
     * Fall back to CPU blit only for non-copy ROPs or shadow buffer sources.
     */
    if (from_screen && to_screen && operation == 3) {
        int overlap;
        int forward_safe;

        overlap = !((dst_x + w <= src_x) ||
                    (src_x + w <= dst_x) ||
                    (dst_y + h <= src_y) ||
                    (src_y + h <= dst_y));

        forward_safe = (dst_y < src_y) ||
                       ((dst_y == src_y) && (dst_x <= src_x));

        ULONG screen_base_word;

        screen_base_word = ((ULONG)wk->screen.mfdb.address - (ULONG)g_vdp_memory_base) >> 1;
        
        if (!overlap || forward_safe) {
            /* Direct copy is safe - no overlap or copy direction is forward */
            vdp_copy_2d(screen_base_word, screen_base_word,
                        (UWORD)src_x, (UWORD)src_y,
                        (UWORD)dst_x, (UWORD)dst_y,
                        (UWORD)w, (UWORD)h,
                        (UWORD)wk->screen.mfdb.width);
        } else {
            /* Overlapping with unsafe backward direction - use staging buffer */
            vdp_copy_2d_via_staging(screen_base_word, screen_base_word,
                                    (UWORD)src_x, (UWORD)src_y,
                                    (UWORD)dst_x, (UWORD)dst_y,
                                    (UWORD)w, (UWORD)h,
                                    (UWORD)wk->screen.mfdb.width);
        }
        GFX_CP_EXIT(CP_BLIT_AREA);
        return 1;
    }

    src_base = (ULONG)src_addr;
    dst_base = (ULONG)dst_addr;
    if ((src_base & 1UL) || (dst_base & 1UL)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    src_pos0 = (ULONG)src_y * (ULONG)src_wrap + (ULONG)src_x * PIXEL_SIZE;
    dst_pos0 = (ULONG)dst_y * (ULONG)dst_wrap + (ULONG)dst_x * PIXEL_SIZE;
    src_size_bytes = (ULONG)src_wrap * (ULONG)src_max_h;
    dst_size_bytes = (ULONG)dst_wrap * (ULONG)dst_max_h;
    src_first = src_base + src_pos0;
    src_last = src_first + (ULONG)(h - 1) * (ULONG)src_wrap + (ULONG)(w - 1) * PIXEL_SIZE;
    dst_first = dst_base + dst_pos0;
    dst_last = dst_first + (ULONG)(h - 1) * (ULONG)dst_wrap + (ULONG)(w - 1) * PIXEL_SIZE;
    if ((src_first & 1UL) || (src_last & 1UL) ||
        src_last < src_first || src_first < src_base || src_last >= (src_base + src_size_bytes)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }
    if ((dst_first & 1UL) || (dst_last & 1UL) ||
        dst_last < dst_first || dst_first < dst_base || dst_last >= (dst_base + dst_size_bytes)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    src_pos = src_pos0;
    dst_pos = dst_pos0;
    src_line_add = src_wrap - w * PIXEL_SIZE;
    dst_line_add = dst_wrap - w * PIXEL_SIZE;

    if (src_y < dst_y) {
        src_pos += (ULONG)(h - 1) * (ULONG)src_wrap;
        src_line_add -= src_wrap * 2;
        dst_pos += (ULONG)(h - 1) * (ULONG)dst_wrap;
        dst_line_add -= dst_wrap * 2;
    }

    src_addr += src_pos / PIXEL_SIZE;
    dst_addr += dst_pos / PIXEL_SIZE;
    src_line_add /= PIXEL_SIZE;     /* Change into pixel count */
    dst_line_add /= PIXEL_SIZE;
    if (((ULONG)src_addr & 1UL) || ((ULONG)dst_addr & 1UL)) {
        GFX_CP_EXIT(CP_BLIT_AREA);
        return -1;
    }

    dst_addr_fast = wk->screen.shadow.address;  /* May not really be to screen at all, but... */

    if ((src_y == dst_y) && (src_x < dst_x)) {
        src_addr += w;		/* To take backward copy into account */
        dst_addr += w;
        src_line_add += 2 * w;
        dst_line_add += 2 * w;
        switch(operation) {
        default:
            DPRINTF(("c_blit_area: mode=pen_back saddr=%lx,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
            pan_backwards(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
            break;
        }
    } else {
        switch(operation) {
        default:
            DPRINTF(("c_blit_area: mode=blit_default saddr=%lx,sline=%d,daddr=%lX,dline=%d srx=%ld,w=%ld,h=%ld\n\r", (ULONG)src_addr, src_line_add, (ULONG)dst_addr, dst_line_add, src_x, w, h));
            blit_16b(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
            break;
        }
    }

    (void) to_screen;
    (void) dst_addr_fast;

    GFX_CP_EXIT(CP_BLIT_AREA);
    return 1;   /* Return as completed */
}
