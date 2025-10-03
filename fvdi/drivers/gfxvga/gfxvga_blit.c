/*
 * Implements FVDI driver API functions c_expand_area() and c_blit_area().
 */

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define FVDI_DEBUG 1
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
            v32s = *--src_addr;
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
    int src_wrap, dst_wrap;
    int src_line_add, dst_line_add;
    unsigned long src_pos, dst_pos;
    int to_screen;

    if (w <= 0 || h <= 0)
        return 1;

    wk = vwk->real_address;

    if (!src || !src->address || (src->address == wk->screen.mfdb.address)) {       /* From screen? */
        src_wrap = wk->screen.wrap;
        if (!(src_addr = wk->screen.shadow.address))
            src_addr = wk->screen.mfdb.address;
    } else {
        src_wrap = (long)src->wdwidth * 2 * src->bitplanes;
        src_addr = src->address;
    }
    src_pos = (short)src_y * (long)src_wrap + src_x * PIXEL_SIZE;
    src_line_add = src_wrap - w * PIXEL_SIZE;

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

    if (src_y < dst_y) {
        src_pos += (short)(h - 1) * (long)src_wrap;
        src_line_add -= src_wrap * 2;
        dst_pos += (short)(h - 1) * (long)dst_wrap;
        dst_line_add -= dst_wrap * 2;
    }

    src_addr += src_pos / PIXEL_SIZE;
    dst_addr += dst_pos / PIXEL_SIZE;
    src_line_add /= PIXEL_SIZE;     /* Change into pixel count */
    dst_line_add /= PIXEL_SIZE;

    dst_addr_fast = wk->screen.shadow.address;  /* May not really be to screen at all, but... */

    if ((src_y == dst_y) && (src_x < dst_x)) {
        src_addr += w;		/* To take backward copy into account */
        dst_addr += w;
        src_line_add += 2 * w;
        dst_line_add += 2 * w;
        switch(operation) {
        case 3:
            DPRINTF(("c_blit_area: mode=pen_backcopy sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            pan_backwards_copy(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
            break;
        case 7:
            DPRINTF(("c_blit_area: mode=pen_back_or sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            pan_backwards_or(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
            break;
        default:
            DPRINTF(("c_blit_area: mode=pen_back sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            pan_backwards(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
            break;
        }
    } else {
        switch(operation) {
        case 3:
            DPRINTF(("c_blit_area: mode=blit_copy sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            blit_copy(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
            break;
        case 7:
            DPRINTF(("c_blit_area: mode=blit_or sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            blit_or(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h);
            break;
        default:
            DPRINTF(("c_blit_area: mode=blit_default sx=%ld,sy=%ld,dx=%ld,dy=%ld\n\r", src_x, src_y, dst_x, dst_y));
            blit_16b(src_addr, src_line_add, dst_addr, 0, dst_line_add, w, h, operation);
            break;
        }
    }

    (void) to_screen;
    (void) dst_addr_fast;
    
    return 1;   /* Return as completed */
}
