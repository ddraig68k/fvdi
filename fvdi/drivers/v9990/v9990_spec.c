/*
 * A 16 bit mode specification/initialization file, by Johan Klockars.
 *
 * $Id: 16b_spec.c,v 1.5 2005-05-25 14:02:00 johan Exp $
 *
 * This file is an example of how to write an
 * fVDI device driver routine in C.
 *
 * You are encouraged to use this file as a starting point
 * for other accelerated features, or even for supporting
 * other graphics modes. This file is therefore put in the
 * public domain. It's not copyrighted or under any sort
 * of license.
 */

#include "fvdi.h"
#include "relocate.h"
#include "driver.h"
#include "string/memset.h"

#include "v9990.h"

#define V9990_B6

#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

char red[] = {5};
char green[] = {5};
char blue[] = {5};
char none[] = {0};
#if 0
char red[] = {5, 11, 12, 13, 14, 15};
char green[] = {5, 6, 7, 8, 9, 10};
char blue[] = {5, 0, 1, 2, 3, 4};
char alpha[] = {0};
char genlock[] = {0};
char unused[] = {1, 5};
#endif

Mode mode[1] =
{{4, CHUNKY | CHECK_PREVIOUS, {red, green, blue, none, none, none}, 0,  2, 1, 1}};

extern Device device;

char driver_name[] = "Kiwi (v9990)";

extern Driver *me;
extern Access *access;

extern short *loaded_palette;

//extern short colours[][3];
extern void CDECL initialize_palette (Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
extern void CDECL c_initialize_palette (Virtual *vwk, long start, long entries, short requested[][3], Colour palette[]);
//extern void *c_set_colours;		/* Just to check if the routine is available */
long CDECL c_write_pixel (Virtual *vwk, MFDB *dst, long x, long y, long colour);
long CDECL c_read_pixel (Virtual *vwk, MFDB *src, long x, long y);
long CDECL c_line_draw (Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode);
long CDECL c_expand_area (Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour);
long CDECL c_vfill_area (Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style);
long CDECL c_blit_area (Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation);
long CDECL c_mouse_draw (Workstation *wk, long x, long y, Mouse *mouse);
void CDECL c_get_colours (Virtual *vwk, long colour, long *foreground, long* background);
void CDECL c_set_colours (Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]);
void CDECL c_set_colour (Virtual *vwk, long paletteIndex, long red, long green, long blue);


extern long tokenize (const char *ptr);

//extern void *c_write_pixel;
//extern void *c_read_pixel;
//extern void *c_line_draw;
//extern void *c_expand_area;
//extern void *c_vfill_area;
//extern void *c_fill_area;
//extern void *c_blit_area;
//extern void *c_mouse_draw;
//extern void *c_set_colours;
//extern void *c_get_colours;
//extern void *c_get_colour;

//void *write_pixel_r = &c_write_pixel;
//void *read_pixel_r  = &c_read_pixel;
//void *line_draw_r   = &c_line_draw;
//void *expand_area_r = &c_expand_area;
//void *fill_area_r   = &c_vfill_area;
//void *fill_poly_r   = 0;
//void *blit_area_r   = &c_blit_area;
//void *text_area_r   = 0;
//void *mouse_draw_r  = &c_mouse_draw;
//void *set_colours_r = &c_set_colours;
//void *get_colours_r = 0;
//void *get_colour_r  = &c_get_colours;

long CDECL(*write_pixel_r) (Virtual *vwk, MFDB * mfdb, long x, long y, long colour) = c_write_pixel;
long CDECL(*read_pixel_r) (Virtual *vwk, MFDB * mfdb, long x, long y) = c_read_pixel;
long CDECL(*line_draw_r) (Virtual *vwk, long x1, long y1, long x2, long y2, long pattern, long colour, long mode) = c_line_draw;
long CDECL(*expand_area_r) (Virtual *vwk, MFDB * src, long src_x, long src_y, MFDB * dst, long dst_x, long dst_y, long w, long h, long operation, long colour) = c_expand_area;
long CDECL(*fill_area_r) (Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style) = c_vfill_area;
long CDECL(*fill_poly_r) (Virtual *vwk, short points[], long n, short index[], long moves, short *pattern, long colour, long mode, long interior_style) = 0;
long CDECL(*blit_area_r) (Virtual *vwk, MFDB * src, long src_x, long src_y, MFDB * dst, long dst_x, long dst_y, long w, long h, long operation) = c_blit_area;
long CDECL(*text_area_r) (Virtual *vwk, short *text, long length, long dst_x, long dst_y, short *offsets) = 0;
long CDECL(*mouse_draw_r) (Workstation *wk, long x, long y, Mouse * mouse) = c_mouse_draw;

long CDECL(*get_colour_r) (Virtual *vwk, long colour) = c_get_colours;
void CDECL(*get_colours_r) (Virtual *vwk, long colour, unsigned long *foreground, unsigned long *background) = 0;
void CDECL(*set_colours_r) (Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]) = c_set_colours;
//long CDECL (*default_fill)(Virtual *vwk, long x, long y, long w, long h, short *pattern, long colour, long mode, long interior_style);
long wk_extend = 0;

short accel_s = 0;
/* short accel_c = A_SET_PAL | A_GET_COL | A_SET_PIX | A_GET_PIX | A_BLIT | A_FILL | A_LINE; */
short accel_c = A_SET_PAL | A_GET_COL | A_SET_PIX | A_GET_PIX | A_BLIT | A_EXPAND | A_MOUSE | A_FILL | A_LINE;
//short accel_c = A_SET_PAL | A_GET_COL | A_SET_PIX | A_GET_PIX | A_BLIT | A_MOUSE | A_FILL | A_LINE;

const Mode *graphics_mode = &mode[0];

//short debug = 0;


Option options[] =
{
        {"debug",      &debug,          2}   /* debug, turn on debugging aids */
};


/*
 * Handle any driver specific parameters
 */
//long check_token (char *token, const char **ptr)
//{
//        int i;
//        int normal;
//        char *xtoken;
//        xtoken = token;
//
//        switch (token[0])
//        {
//        case '+':
//                xtoken++;
//                normal = 1;
//                break;
//
//        case '-':
//                xtoken++;
//                normal = 0;
//                break;
//
//        default:
//                normal = 1;
//                break;
//        }
//
//        for (i = 0; i < sizeof (options) / sizeof (Option); i++)
//        {
//                if (access->funcs.equal (xtoken, options[i].name))
//                {
//                        switch (options[i].type)
//                        {
//                        case -1:     /* Function call */
//                                return ((long (*) (const char **))options[i].varfunc) (ptr);
//
//                        case 0:      /* Default 1, set to 0 */
//                                * (short *)options[i].varfunc = 1 - normal;
//                                return 1;
//
//                        case 1:     /* Default 0, set to 1 */
//                                * (short *)options[i].varfunc = normal;
//                                return 1;
//
//                        case 2:     /* Increase */
//                                * (short *)options[i].varfunc += -1 + 2 * normal;
//                                return 1;
//
//                        case 3:
//                                if (! (*ptr = access->funcs.skip_space (*ptr)))
//                                        ;  /* *********** Error, somehow */
//
//                                *ptr = access->funcs.get_token (*ptr, token, 80);
//                                * (short *)options[i].varfunc = token[0];
//                                return 1;
//                        }
//                }
//        }
//
//        return 0;
//}
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
            case -1:      /* Function call */
                return (options[i].var.func)(ptr);
            case 0:        /* Default 1, set to 0 */
                *options[i].var.s = 1 - normal;
                return 1;
            case 1:      /* Default 0, set to 1 */
                *options[i].var.s = normal;
                return 1;
            case 2:      /* Increase */
                *options[i].var.s += -1 + 2 * normal;
                return 1;
            case 3:
                if ((*ptr = access->funcs.skip_space(*ptr)) == NULL)
                {
                    ;  /* *********** Error, somehow */
                }
                *ptr = access->funcs.get_token(*ptr, token, 80);
                *options[i].var.s = token[0];
                return 1;
            }
        }
    }

    return 0;
}

void vdp_clearscreen (void)
{
        VDP_BOX box;
        int i;
        uint16_t color = 0x0000;
        box.left = 0;
        box.top = 0;
#ifdef V9990_B5
        box.width = 640; //512;
        box.height = 400; //216;
#elif defined(V9990_B6)
        box.width = 640; //512;
        box.height = 480; //216;
#else
        box.width = 512;
        box.height = 216;
#endif
        v9990_SetCmdWriteMask (0xffff);
        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
        v9990_DrawFilledBox (&box, color);
}

/*
 * Do whatever setup work might be necessary on boot up
 * and which couldn't be done directly while loading.
 * Supplied is the default fVDI virtual workstation.
 */
long CDECL initialize (Virtual *vwk)
{
        //access->funcs.puts("Driver1 (initialize())\r\n");
        Workstation *wk;
        char *buf;
        int old_palette_size;
        Colour *old_palette_colours;
        int fast_w_bytes;
        vwk = me->default_vwk;	/* This is what we're interested in */
        wk = vwk->real_address;
        //default_fill = (void *)wk->r.fill;
        /* update the settings */
        //wk->screen.mfdb.width = 512;
        //wk->screen.mfdb.height = 212;
        //wk->screen.mfdb.bitplanes = 8;
        /*
         * Some things need to be changed from the
         * default workstation settings.
         */
        //wk->screen.mfdb.address = 0;
        wk->screen.mfdb.wdwidth = (wk->screen.mfdb.width + 15) / 16;
        wk->screen.wrap = wk->screen.mfdb.width * (wk->screen.mfdb.bitplanes / 8);
        wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
        wk->screen.coordinates.max_y = wk->screen.mfdb.height - 1;
        wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
        wk->screen.mfdb.standard = 0;

        if (wk->screen.pixel.width > 0)        /* Starts out as screen width */
                //wk->screen.pixel.width = 512; /* (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width; */
        {
                wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
        }
        else                                   /*   or fixed DPI (negative) */
        {
                wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
        }

        if (wk->screen.pixel.height > 0)        /* Starts out as screen height */
                //wk->screen.pixel.height = 212; /* (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height; */
        {
                wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
        }
        else                                    /*   or fixed DPI (negative) */
        {
                wk->screen.pixel.height = 25400 / -wk->screen.pixel.height;
        }

        /*
         * This code needs more work.
         * Especially if there was no VDI started since before.
         */

        if (loaded_palette)
        {
        //        access->funcs.copymem (loaded_palette, colours, 16 * 3 * sizeof (short));
        access->funcs.copymem(loaded_palette, default_vdi_colors, 256 * 3 * sizeof(short));
        }

        wk->screen.palette.size = 16;
//	if ((old_palette_size = wk->screen.palette.size) != 16) {	/* Started from different graphics mode? */
//		old_palette_colours = wk->screen.palette.colours;
//		wk->screen.palette.colours = (Colour *)access->funcs.malloc(16L * sizeof(Colour), 3);	/* Assume malloc won't fail. */
//		if (wk->screen.palette.colours) {
//			wk->screen.palette.size = 16;
//			if (old_palette_colours)
//				access->funcs.free(old_palette_colours);	/* Release old (small) palette (a workaround) */
//		} else
//			wk->screen.palette.colours = old_palette_colours;
//	}
        //c_initialize_palette (vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
        c_initialize_palette(vwk, 0, wk->screen.palette.size, default_vdi_colors, wk->screen.palette.colours);
//	if (*(short *)&c_set_colours != 0x4e75)		/* Look for C... */
//		c_initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
//	else
//		initialize_palette(vwk, 0, wk->screen.palette.size, colours, wk->screen.palette.colours);
        device.byte_width = wk->screen.wrap;
        device.address = wk->screen.mfdb.address;
        //if (!wk->screen.shadow.address)
        //	driver_name[20] = 0;
#ifdef V9990_B5
        /* B5 mode: 640x400@4bpp */
        v9990_SetScreenMode (VDP_MODE_B5, VDP_SCR0_4BIT, VDP_SCR0_XIM1024, VDP_SCR1_PAL, VDP_PAL_CTRL_PAL);
#elif defined(V9990_B6)
        /* B5 mode: 640x480@4bpp */
        v9990_SetScreenMode (VDP_MODE_B6, VDP_SCR0_4BIT, VDP_SCR0_XIM1024, VDP_SCR1_PAL, VDP_PAL_CTRL_PAL);
#else
        /* B3 mode: 512x212@4bpp */
        v9990_SetScreenMode (VDP_MODE_B3, VDP_SCR0_4BIT, VDP_SCR0_XIM1024, VDP_SCR1_PAL, VDP_PAL_CTRL_PAL);
#endif
        v9990_SetScrollX (0);
        v9990_SetScrollY (0);
        v9990_SetBackdropColour (0);
        vdp_clearscreen();
        //v9990_SpritesEnable();
        v9990_SpritesDisable();
        //access->funcs.puts("Driver2\r\n");
        return 1;
}

/*
 *
 */
long CDECL setup (long type, long value)
{
        long ret;
        //access->funcs.puts("Driver_setup 1\r\n");
        ret = -1;

        switch (type)
        {
        case Q_NAME:
                ret = (long)driver_name;
                break;

        case S_DRVOPTION:
                ret = tokenize ((char *)value);
                break;
        }

        //access->funcs.puts("Driver_setup 2\r\n");
        return ret;
}

/*
 * Initialize according to parameters (boot and sent).
 * Create new (or use old) Workstation and default Virtual.
 * Supplied is the default fVDI virtual workstation.
 */
Virtual* CDECL opnwk (Virtual *vwk)
{
        Workstation *wk;
        vwk = me->default_vwk;  /* This is what we're interested in */
        wk = vwk->real_address;
        /* update the settings */
#ifdef V9990_B5
        wk->screen.mfdb.width = 640;
        wk->screen.mfdb.height = 400;
#elif defined(V9990_B6)
        wk->screen.mfdb.width = 640;
        wk->screen.mfdb.height = 480;
#else
        wk->screen.mfdb.width = 512;
        wk->screen.mfdb.height = 212;
#endif
        wk->screen.mfdb.bitplanes = 4;
        /*
         * Some things need to be changed from the
         * default workstation settings.
         */
        //wk->screen.mfdb.address = 0;
        //device.address = wk->screen.mfdb.address;
        wk->screen.mfdb.wdwidth = (wk->screen.mfdb.width + 15) / 16;
        wk->screen.wrap = wk->screen.mfdb.width * (wk->screen.mfdb.bitplanes / 8);
        wk->screen.coordinates.max_x = wk->screen.mfdb.width - 1;
        wk->screen.coordinates.max_y = wk->screen.mfdb.height - 1;
        wk->screen.look_up_table = 0;			/* Was 1 (???)	Shouldn't be needed (graphics_mode) */
        wk->screen.mfdb.standard = 0;

        if (wk->screen.pixel.width > 0)			/* Starts out as screen width */
        {
                wk->screen.pixel.width = (wk->screen.pixel.width * 1000L) / wk->screen.mfdb.width;
        }
        else								   /*	or fixed DPI (negative) */
        {
                wk->screen.pixel.width = 25400 / -wk->screen.pixel.width;
        }

        if (wk->screen.pixel.height > 0)		/* Starts out as screen height */
        {
                wk->screen.pixel.height = (wk->screen.pixel.height * 1000L) / wk->screen.mfdb.height;
        }
        else									/*	 or fixed DPI (negative) */
        {
                wk->screen.pixel.height = 25400 / -wk->screen.pixel.height;
        }

        return 0;
}

/*
 * 'Deinitialize'
 */
void CDECL clswk (Virtual *vwk)
{
}
