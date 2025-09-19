/*
 * Line drawing routines
 *
 * Implements FVDI driver API function c_line_draw().
 *
 */

#include "gfxvga.h"
#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)
#define PIXEL_32    long

/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

static void line_replace(PIXEL *addr, PIXEL *addr_fast, int count,
                    int d, int incrE, int incrNE, int one_step, int both_step,
                    PIXEL foreground, PIXEL background)
{
    *addr = foreground;
    (void) addr_fast;
    (void) background;

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }
        *addr = foreground;
    }
}

static void line_replace_p(PIXEL *addr, PIXEL *addr_fast, long pattern, int count,
                      int d, int incrE, int incrNE, int one_step, int both_step,
                      PIXEL foreground, PIXEL background)
{
    unsigned short mask = 0x8000;

    (void) addr_fast;
    if (pattern & mask) {
        *addr = foreground;
    } else {
        *addr = background;
    }

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }

        if (!(mask >>= 1))
            mask = 0x8000;

        if (pattern & mask) {
            *addr = foreground;
        } else {
            *addr = background;
        }
    }
}

static void line_transparent(PIXEL *addr, PIXEL *addr_fast, int count,
                        int d, int incrE, int incrNE, int one_step, int both_step,
                        PIXEL foreground, PIXEL background)
{
    *addr = foreground;
    (void) addr_fast;
    (void) background;

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }
        *addr = foreground;
    }
}

static void line_transparent_p(PIXEL *addr, PIXEL *addr_fast, long pattern, int count,
                          int d, int incrE, int incrNE, int one_step, int both_step,
                          PIXEL foreground, PIXEL background)
{
    unsigned short mask = 0x8000;

    (void) addr_fast;
    (void) foreground;
    (void) background;
    if (pattern & mask) {
        *addr = foreground;
    }

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }

        if (!(mask >>= 1))
            mask = 0x8000;

        if (pattern & mask) {
            *addr = foreground;
        }
    }
}

static void line_xor(PIXEL *addr, PIXEL *addr_fast, int count,
                int d, int incrE, int incrNE, int one_step, int both_step,
                PIXEL foreground, PIXEL background)
{
    int v;

    (void) addr_fast;
    (void) foreground;
    (void) background;
    v = ~*addr;
    *addr = v;

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }
        v = ~*addr;
        *addr = v;
    }
}

static void line_xor_p(PIXEL *addr, PIXEL *addr_fast, long pattern, int count,
                  int d, int incrE, int incrNE, int one_step, int both_step,
                  PIXEL foreground, PIXEL background)
{
    int v;
    unsigned short mask = 0x8000;

    (void) addr_fast;
    (void) foreground;
    (void) background;
    if (pattern & mask) {
        v = ~*addr;
        *addr = v;
    }

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }

        if (!(mask >>= 1))
            mask = 0x8000;

        if (pattern & mask) {
            v = ~*addr;
            *addr = v;
        }
    }
}

static void line_revtransp(PIXEL *addr, PIXEL *addr_fast, int count,
                      int d, int incrE, int incrNE, int one_step, int both_step,
                      PIXEL foreground, PIXEL background)
{
    *addr = foreground;
    (void) addr_fast;
    (void) background;

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }
        *addr = foreground;
    }
}

static void line_revtransp_p(PIXEL *addr, PIXEL *addr_fast, long pattern, int count,
                        int d, int incrE, int incrNE, int one_step, int both_step,
                        PIXEL foreground, PIXEL background)
{
    unsigned short mask = 0x8000;

    (void) addr_fast;
    (void) background;
    if (!(pattern & mask)) {
        *addr = foreground;
    }

    for(--count; count >= 0; count--) {
        if (d < 0) {
            d += incrE;
            addr += one_step;
        } else {
            d += incrNE;
            addr += both_step;
        }

        if (!(mask >>= 1))
            mask = 0x8000;

        if (!(pattern & mask)) {
            *addr = foreground;
        }
    }
}

long CDECL c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2,
                       long pattern, long colour, long mode)
{
    Workstation *wk;
    PIXEL *addr, *addr_fast;
    unsigned long foreground, background;
    int line_add;
    long pos;
    int x_step, y_step;
    int dx, dy;
    int one_step, both_step;
    int d, count;
    int incrE, incrNE;

    if ((long)vwk & 1) {
        return -1;          /* Don't know about anything yet */
    }

    if (!clip_line(vwk, &x1, &y1, &x2, &y2))
        return 1;

    c_get_colours(vwk, colour, &foreground, &background);

    wk = vwk->real_address;

    pos = (short)y1 * (long)wk->screen.wrap + x1 * 2;
    addr = wk->screen.mfdb.address;
    line_add = wk->screen.wrap >> 1;


    x_step = 1;
    y_step = line_add;

    dx = x2 - x1;
    if (dx < 0) {
        dx = -dx;
        x_step = -x_step;
    }
    dy = y2 - y1;
    if (dy < 0) {
        dy = -dy;
        y_step = -y_step;
    }

    if (dx > dy) {
        count = dx;
        one_step = x_step;
        incrE = 2 * dy;
        incrNE = 2 * dy - 2 * dx;
        d = 2 * dy - dx;
    } else {
        count = dy;
        one_step = y_step;
        incrE = 2 * dx;
        incrNE = 2 * dx - 2 * dy;
        d = 2 * dx - dy;
    }
    both_step = x_step + y_step;

    addr += pos >> 1;
    if ((pattern & 0xffff) == 0xffff) {
        switch (mode) {
        case 1:             /* Replace */
            line_replace(addr, addr_fast, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 2:             /* Transparent */
            line_transparent(addr, addr_fast, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 3:             /* XOR */
            line_xor(addr, addr_fast, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 4:             /* Reverse transparent */
            line_revtransp(addr, addr_fast, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        }
    } else {
        switch (mode) {
        case 1:             /* Replace */
            line_replace_p(addr, addr_fast, pattern, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 2:             /* Transparent */
            line_transparent_p(addr, addr_fast, pattern, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 3:             /* XOR */
            line_xor_p(addr, addr_fast, pattern, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        case 4:             /* Reverse transparent */
            line_revtransp_p(addr, addr_fast, pattern, count, d, incrE, incrNE, one_step, both_step, foreground, background);
            break;
        }
    }
    return 1;       /* Return as completed */
}
