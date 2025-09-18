/* 
 * Initialization code for the Y Ddraig GfxVGA FVDI driver.
 */

#include "fvdi.h"
#include "driver.h"
#include "relocate.h"
#include "../bitplane/bitplane.h"
#include "string/memset.h"
#include "gfxvga.h"

static char const r_16[] = { 5, 11, 12, 13, 14, 15 };
static char const g_16[] = { 6,  5,  6,  7,  8,  9, 10 };
static char const b_16[] = { 5,  0,  1,  2,  3,  4 };
static char const none[] = { 0 };

static Mode const mode[1] = {
    { 16, CHUNKY | TRUE_COLOUR, { r_16, g_16, b_16, none, none, none }, 0, 2, 2, 1 }
};

char driver_name[] = "Y Ddraig GfxVGA";

long CDECL (*write_pixel_r)(Virtual *vwk, MFDB *mfdb, long x, long y, long colour) = c_write_pixel;
long CDECL (*read_pixel_r)(Virtual *vwk, MFDB *mfdb, long x, long y) = c_read_pixel;
long CDECL (*line_draw_r)(Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode) = 0;
long CDECL (*expand_area_r)(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour) = 0;
long CDECL(*fill_area_r)(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style) = 0;
long CDECL (*fill_poly_r)(Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour, long mode, long interior_style) = 0;
long CDECL (*blit_area_r)(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation) = 0;
long CDECL (*text_area_r)(Virtual *vwk, short *text, long length, long dst_x, long dst_y, short *offsets) = 0;
long CDECL (*mouse_draw_r)(Workstation *wk, long x, long y, Mouse *mouse) = c_mouse_draw;

long CDECL (*get_colour_r)(Virtual *vwk, long colour) = c_get_colour;
void CDECL (*get_colours_r)(Virtual *vwk, long colour, unsigned long *foreground, unsigned long *background) = 0;
void CDECL (*set_colours_r)(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]) = c_set_colours;

long wk_extend = 0;
short accel_s = 0;
short accel_c = A_SET_PAL | A_GET_COL | A_SET_PIX | A_GET_PIX | A_MOUSE;

const Mode *graphics_mode = &mode[0];

uint32_t gfxvga_mem_base = 0xA00000;

short fix_shape = 0;
short no_restore = 0;


static Option const options[] = {
        {"debug", {&debug}, 2},
};

/*
 * Handle any driver specific parameters
 */
long check_token(char *token, const char **ptr)
{
    int i;
    short normal;
    char *xtoken;

    xtoken = token;
    switch (token[0]) {
        case '+':
            xtoken++;
            normal = 1;
            break;
        case '-':
            xtoken++;
            normal = 0;
            break;
        default:
            normal = 1;
            break;
    }
    for (i = 0; i < (int) (sizeof(options) / sizeof(Option)); i++) {
        if (access->funcs.equal(xtoken, options[i].name)) {
            switch (options[i].type) {
                case -1:                /* Function call */
                    return (options[i].var.func)(ptr);
                case 0:                 /* Default 1, set to 0 */
                    *options[i].var.s = (short)(1 - normal);
                    return 1;
                case 1:                 /* Default 0, set to 1 */
                    *options[i].var.s = normal;
                    return 1;
                case 2:                 /* Increase */
                    *options[i].var.s += -1 + 2 * normal;
                    return 1;
                case 3:
                    if ((*ptr = access->funcs.skip_space(*ptr)) ==
                        NULL) { ;               /* *********** Error, somehow */
                    }
                    *ptr = access->funcs.get_token(*ptr, token, 80);
                    *options[i].var.s = token[0];
                    return 1;
            }
        }
    }
    return 0;
}

/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */

long CDECL initialize(Virtual *vwk)
{
    access->funcs.puts("Initializing GfxVGA driver...\r\n");

    Workstation *wk;

    vwk = me->default_vwk;        /* This is what we're interested in */
    wk = vwk->real_address;

    const short width = 640, height = 480, bits_per_pixel = 16;

    /* update the settings */
    wk->screen.mfdb.width = width;
    wk->screen.mfdb.height = height;
    wk->screen.mfdb.bitplanes = bits_per_pixel;

    wk->screen.mfdb.wdwidth = (short) (wk->screen.mfdb.width * wk->screen.mfdb.bitplanes / 16);
    wk->screen.wrap = (short) (wk->screen.mfdb.width * wk->screen.mfdb.bitplanes / 8);

    wk->screen.coordinates.max_x = (short) (wk->screen.mfdb.width - 1);
    wk->screen.coordinates.max_y = (short) (wk->screen.mfdb.height - 1);

    wk->screen.look_up_table = 0;
    wk->screen.mfdb.standard = 0;

    if (wk->screen.pixel.width > 0)  /* Starts out as screen width in millimeters. Convert to microns per pixel. */
        wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
    else                                   /*   or fixed DPI (negative) */
        wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
    if (wk->screen.pixel.height > 0) /* Starts out as screen height in millimeters. Convert to microns per pixel. */
        wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
    else                                    /*   or fixed DPI (negative) */
        wk->screen.pixel.height = 25400 / -wk->screen.pixel.height;

    // FIXME: The EmuTOS XBIOS needs to be made Xosera-aware and then made to return the correct Xosera-specific address here.
    wk->screen.mfdb.address = (short *)0xA00000; //Physbase();

    device.address = (void *) gfxvga_mem_base;

    access->funcs.puts("GfxVGA: Initializing...\r\n");
    char str[80], buf[80];
    str[0] = 0;
    access->funcs.cat("GfxVGA: Configuring, using base address of 0x", str);
    access->funcs.ltoa(buf, (long) gfxvga_mem_base, 16);
    access->funcs.cat(buf, str);
    access->funcs.cat(".\r\n", str);
    access->funcs.puts(str);

    /*
     * This code needs more work.
     * Especially if there was no VDI started since before.
     */
    if (loaded_palette) {
        access->funcs.puts("We are loading a palette.\n");
        access->funcs.copymem(loaded_palette, default_vdi_colors, 16 * 3 * sizeof(short));
    } else {
        access->funcs.puts("We are NOT loading a palette.\n");
    }
    wk->screen.palette.size = bits_per_pixel == 4 ? 16 : 256;

    Colour *default_palette = (Colour *) access->funcs.malloc(wk->screen.palette.size * sizeof(Colour), 3);
    if (default_palette == NULL) {
        access->funcs.error("Can't allocate memory for default palette.\n", NULL);
    }
    wk->screen.palette.colours = default_palette;

    // This call is required to initialize the palette.
    c_initialize_palette(vwk, 0, wk->screen.palette.size, default_vdi_colors, wk->screen.palette.colours);

    device.byte_width = wk->screen.wrap;
    access->funcs.puts("GfxVGA driver init: done.\r\n");

    return 1;
}

/*
 *
 */
long CDECL setup(long type, long value)
{
    long ret;

    ret = -1;
    switch (type) {
        case Q_NAME:
            ret = (long) driver_name;
            break;
        case S_DRVOPTION:
            ret = tokenize((char *) value);
            break;
    }

    return ret;
}

Virtual *CDECL opnwk(Virtual *UNUSED(vwk))
{
    return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
    (void) vwk;
}
