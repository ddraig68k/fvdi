/* 
 * Initialization code for the Y Ddraig GfxVGA FVDI driver.
 */

#include "fvdi.h"
#include "gfxvga.h"
#include <os.h>
#include "stdint.h"
#include "driver.h"
#include "utility.h"
#include "string/memset.h"


static char const r_16[] = { 5, 11, 12, 13, 14, 15 };
static char const g_16[] = { 6,  5,  6,  7,  8,  9, 10 };
static char const b_16[] = { 5,  0,  1,  2,  3,  4 };
static char const none[] = { 0 };

static Mode const mode[1] = {
    { 16, CHECK_PREVIOUS | CHUNKY | TRUE_COLOUR, { r_16, g_16, b_16, none, none, none }, 0, 2, 2, 1 }
};

char driver_name[] = "Y Ddraig GfxVGA";

struct resolution
{
    short used; /* Whether the mode option was used or not. */
    short width;
    short height;
    short bpp;
    short freq;
    short flags;
} resolution =  { 0, 640, 480, 16, 60, 0 };

static struct {
    short width;
    short height;
} pixel;


long CDECL (*write_pixel_r)(Virtual *vwk, MFDB *mfdb, long x, long y, long colour) = c_write_pixel;
long CDECL (*read_pixel_r)(Virtual *vwk, MFDB *mfdb, long x, long y) = c_read_pixel;
long CDECL (*line_draw_r)(Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode) = c_line_draw;
long CDECL (*expand_area_r)(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour) = c_expand_area;
long CDECL (*fill_area_r)(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style) = c_fill_area;
long CDECL (*fill_poly_r)(Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour, long mode, long interior_style) = 0;
long CDECL (*blit_area_r)(Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation) = c_blit_area;
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
    {"debug",      { &debug }, 2 },              /* debug, turn on debugging aids */
    {"fixshape",   { &fix_shape }, 1 },          /* fixed shape; do not allow mouse shape changes */
    {"norestore",  { &no_restore }, 1 },
};

/*
 * Handle any driver specific parameters
 */
long check_token(char *token, const char **ptr)
{
    int i;
    int normal;
    char *xtoken;

    xtoken = token;
    switch (token[0])
    {
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
    for (i = 0; i < (int)(sizeof(options) / sizeof(Option)); i++)
    {
        if (access->funcs.equal(xtoken, options[i].name))
        {
            switch (options[i].type)
            {
            case -1:                /* Function call */
                return (options[i].var.func) (ptr);
            case 0:                 /* Default 1, set to 0 */
                *options[i].var.s = 1 - normal;
                return 1;
            case 1:                 /* Default 0, set to 1 */
                *options[i].var.s = normal;
                return 1;
            case 2:                 /* Increase */
                *options[i].var.s += -1 + 2 * normal;
                return 1;
            case 3:
                if ((*ptr = access->funcs.skip_space(*ptr)) == NULL)
                {
                    ;               /* *********** Error, somehow */
                }
                *ptr = access->funcs.get_token(*ptr, token, 80);
                *options[i].var.s = token[0];
                return 1;
            }
        }
    }

    return 0;
}

static short *screen_address;

static void vbl_handler(void)
{   
    // Clear the VGA interrupt flag
    DRVGA_REG_WRITE(REG_STATUS, 0x04);
}

typedef void (*PFVOID)(void);

static void install_to_vblqueue(PFVOID handler)
{
    PFVOID **vblqueue = (PFVOID **)(0x456); /* Pointer to the VBL queue array */
    PFVOID *vbl_list = NULL;
    if (vblqueue)
        vbl_list = *vblqueue;
    if (vbl_list)
        vbl_list[1] = handler;
}

/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
long CDECL initialize(Virtual *vwk)
{
    Workstation *wk;
    int old_palette_size;
    Colour *old_palette_colours;

    access->funcs.puts("Initializing GfxVGA driver...\r\n");
    
    screen_address = (short *)0xA00000;

    vwk = me->default_vwk;        /* This is what we're interested in */
    wk = vwk->real_address;

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

            /*
     * This code needs more work.
     * Especially if there was no VDI started since before.
     */

    if (loaded_palette)
        access->funcs.copymem(loaded_palette, default_vdi_colors, 256 * 3 * sizeof(short));

    if ((old_palette_size = wk->screen.palette.size) != 256)
    {
        old_palette_colours = wk->screen.palette.colours;

        wk->screen.palette.colours = access->funcs.malloc(256L * sizeof(Colour), 3);    /* Assume malloc won't fail. */
        if (wk->screen.palette.colours)
        {
            wk->screen.palette.size = 256;
            if (old_palette_colours)
                access->funcs.free(old_palette_colours);    /* Release old (small) palette (a workaround) */
        }
        else
            wk->screen.palette.colours = old_palette_colours;
    }
    c_initialize_palette(vwk, 0, wk->screen.palette.size, default_vdi_colors, wk->screen.palette.colours);

    // FIXME: The EmuTOS XBIOS needs to be made Xosera-aware and then made to return the correct Xosera-specific address here.
    wk->screen.mfdb.address = Physbase();

    access->funcs.puts("GfxVGA: Initializing...\r\n");
    char str[80], buf[80];
    str[0] = 0;
    access->funcs.cat("GfxVGA: Configuring, using base address of 0x", str);
    access->funcs.ltoa(buf, (long) gfxvga_mem_base, 16);
    access->funcs.cat(buf, str);
    access->funcs.cat(".\r\n", str);
    access->funcs.puts(str);

    // Install the vertical blank handler
    install_to_vblqueue(vbl_handler);

	g_gfxfpga_base = 0x00F7F500;
	drvga_write_control_reg(DISPMODE_BITMAPHIRES);
    // Enable vblank inerrupt
    DRVGA_REG_WRITE(REG_INTERRUPT, 0x0001);

    device.byte_width = wk->screen.wrap;
    device.address = wk->screen.mfdb.address;

    pixel.width = wk->screen.pixel.width;
    pixel.height = wk->screen.pixel.height;

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

Virtual *CDECL opnwk(Virtual *vwk)
{
    Workstation *wk;

    vwk = me->default_vwk;  /* This is what we're interested in */
    wk = vwk->real_address;

    /* update the settings */
    wk->screen.mfdb.width = (short) resolution.width;
    wk->screen.mfdb.height = (short) resolution.height;
    wk->screen.mfdb.bitplanes = (short) resolution.bpp;

    /*
     * Some things need to be changed from the
     * default workstation settings.
     */
    wk->screen.mfdb.address = (short *) screen_address;

    wk->screen.mfdb.wdwidth = (wk->screen.mfdb.width + 15) / 16;
    wk->screen.wrap = wk->screen.mfdb.width * (wk->screen.mfdb.bitplanes / 8);

    wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
    wk->screen.coordinates.max_y = wk->screen.mfdb.height - 1;

    wk->screen.look_up_table = 0;           /* Was 1 (???)  Shouldn't be needed (graphics_mode) */
    wk->screen.mfdb.standard = 0;

    if (pixel.width > 0)            /* Starts out as screen width */
        wk->screen.pixel.width = (pixel.width * 1000L) / wk->screen.mfdb.width;
    else                                   /*   or fixed DPI (negative) */
        wk->screen.pixel.width = 25400 / -pixel.width;

    if (pixel.height > 0)       /* Starts out as screen height */
        wk->screen.pixel.height = (pixel.height * 1000L) / wk->screen.mfdb.height;
    else                                    /*   or fixed DPI (negative) */
        wk->screen.pixel.height = 25400 / -pixel.height;

    wk->mouse.position.x = ((wk->screen.coordinates.max_x - wk->screen.coordinates.min_x + 1) >> 1) + wk->screen.coordinates.min_x;
    wk->mouse.position.y = ((wk->screen.coordinates.max_y - wk->screen.coordinates.min_y + 1) >> 1) + wk->screen.coordinates.min_y;

    /*
     * FIXME
     *
     * for some strange reason, the driver only works if the palette is (re)initialized here again.
     * Otherwise, everything is drawn black on black (not very useful).
     *
     * I did not yet find what I'm doing differently from other drivers (that apparently do not have this problem).
     */
    c_initialize_palette(vwk, 0, wk->screen.palette.size, default_vdi_colors, wk->screen.palette.colours);

    return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk(Virtual *vwk)
{
    (void) vwk;
}
