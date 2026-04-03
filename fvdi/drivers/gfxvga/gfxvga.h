#ifndef INCLUDE_GFXVGA_H
#define INCLUDE_GFXVGA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fvdi.h"
#include "gfxvdp.h"


#define KPRINTF_ADDRESS 0x80517c
typedef int (*kprintf_func_t)(const char *fmt, ...);
extern kprintf_func_t my_kprintf;
#ifdef FVDI_DEBUG
#define DPRINTF(x) my_kprintf x
#else
#define DPRINTF(x)
#endif

/* Function prototypes. */
long CDECL c_get_colour(Virtual *vwk, long colour);

void CDECL c_get_colours(Virtual *vwk, long colour, unsigned long *foreground, unsigned long *background);

void CDECL c_set_colours(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]);

long CDECL c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour);

long CDECL c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y);

long CDECL c_line_draw(Virtual *vwk, long x1, long y1, long x2, long y2, long line_style, long colour, long mode);

long CDECL
c_expand_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
              long operation, long colour);

long CDECL
c_fill_area(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style);

//long CDECL (*fill_poly_r)(Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour, long mode, long interior_style);
long CDECL
c_blit_area(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h,
            long operation);

//long CDECL (*text_area_r)(Virtual *vwk, short *text, long length, long dst_x, long dst_y, short *offsets);
long CDECL c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse);

static inline int is_screen(struct wk_ *wk, MFDB *mfdb)
{
    return (mfdb == NULL || mfdb->address == NULL || mfdb->address == wk->screen.mfdb.address) ? 1 : 0;
}

#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#endif
