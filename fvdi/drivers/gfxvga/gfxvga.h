#ifndef INCLUDE_GFXVGA_H
#define INCLUDE_GFXVGA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "fvdi.h"

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef int BOOL;
typedef void VOID;
typedef ULONG IPTR;

#define TRUE 1
#define FALSE 0


#define KPRINTF_ADDRESS 0x80517c
typedef int (*kprintf_func_t)(const char *fmt, ...);
extern kprintf_func_t my_kprintf;
#ifdef FVDI_DEBUG
#define DPRINTF(x) my_kprintf x
#else
#define DPRINTF(x)
#endif

extern ULONG g_gfxfpga_base;

#define GFX_TEXTBUF_SIZE  2400

#define GFX_REG_WRITE(x, y)  (*((volatile UWORD *) (g_gfxfpga_base + (x))) = (y))
#define GFX_REG_READ(x)      (*((volatile UWORD *) (g_gfxfpga_base + (x))))

#define	REG_STATUS              0x00    // Status register
#define REG_CONTROL             0x02    // Display control register
#define REG_INTERRUPT           0x04    // Interrupt control/ status
#define	REG_COMMAND             0x06    // Command control register
#define REG_DATA                0x08    // Command data register
#define	REG_PARAM_DATA0         0x0A    // Command parameter 0
#define	REG_PARAM_DATA1         0x0C    // Command parameter 1
#define	REG_PARAM_DATA2         0x0E    // Command parameter 2
#define	REG_PARAM_DATA3         0x10    // Command parameter 3
#define	REG_PARAM_DATA4         0x12    // Command parameter 4
#define	REG_PARAM_DATA5         0x14    // Command parameter 5
#define	REG_PARAM_DATA6         0x16    // Command parameter 6
#define	REG_PARAM_DATA7         0x18    // Command parameter 7

#define REG_PALETTE_IDX         0x20    // Palette index register
#define REG_PALETTE_DATA        0x22    // Palette data register
#define REG_PALETTE_CTL         0x24    // Palette control register

#define REG_BITMAP_PTR_L		0x30    // Bitmap data address location (L) 
#define REG_BITMAP_PTR_H		0x32    // Bitmap data address location (H)

#define CMD_NONE                0x0000   
#define CMD_FILL_RECT           0x0001
#define CMD_DRAW_LINE           0x0002

#define CMD_SET_CHARACTER       0x0010
#define CMD_MEMORY_ACCESS       0x0020

#define DRAW_MODE_SOLID        0x0
#define DRAW_MODE_XOR          (0x1 << 7)
#define DRAW_MODE_TRANS        (0x2 << 7)
#define DRAW_MODE_REVTRANS     (0x3 << 7)

#define DRAW_PATTERN(p)         ((p & 0x3f) << 10)

// Status register masks
#define STATUS_READY            0x0001
#define STATUS_ERROR            0x0002
#define STATUS_HSYNC            0x0004
#define STATUS_VSYNC            0x0008

// Control register settings
// Display modes
#define DISPMODE_TEXT			0x0000      // Text mode, default
#define DISPMODE_BITMAP			0x0001      // Bitmap 320x240
#define DISPMODE_BITMAPHIRES	0x0002      // Bitmap 640x480
#define DISPMODE_TILE1			0x0003      // Tile layer 1 (320x240)
#define DISPMODE_TILE2			0x0004      // Tile layer 2 (320x240)
#define DISPMODE_BMAPTILE		0x0005      // Bitmap with Tile overlay (320x240)
#define DISPMODE_BGCOLOR		0x0006      // Background color only
#define DISPMODE_RESERVED		0x0007      // Reserved for future use

// Bitmap mode bits per pixel
#define DISP_DEPTH_12BPP		0x0000
#define DISP_DEPTH_8BPP			0x0008
#define DISP_DEPTH_4BPP			0x0009
#define DISP_DEPTH_2BPP			0x000A
#define DISP_DEPTH_1BPP			0x000B

#define DISP_WIDTH_512			0x0000
#define DISP_WIDTH_1024			0x0040
#define DISP_WIDTH_2048			0x0080
#define DISP_WIDTH_4096			0x00C0


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

void gfx_write_control_reg(UWORD data);
void gfx_wait_busy(void);
void gfx_wait_vblank(void);
void gfx_wait_vblank_clear(void);
void gfx_clear_text(void);
void gfx_write_text(UWORD posx, UWORD posy, char *text);
void gfx_write_char(UWORD posx, UWORD posy, char text);
void gfx_draw_box(UWORD mode, UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color1, UWORD color2);
void gfx_draw_line(UWORD mode, UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color1, UWORD color2, UWORD pattern);
void gfx_set_memory_ptr(ULONG addr);
void gfx_write_memory_block(UWORD *data, ULONG datalen);


#ifdef __GNUC__
#  define UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_ ## x
#endif

#endif
