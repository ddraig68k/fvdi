/*
 * A 16 bit graphics blit routine, by Johan Klockars.
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

#if 1
#define FAST		/* Write in FastRAM buffer */
#define BOTH		/* Write in both FastRAM and on screen */
#else
#undef FAST
#undef BOTH
#endif

#include "fvdi.h"
#include <stdint.h>
#include "relocate.h"
#include "os.h"
#include "v9990.h"

extern Access *access;

#define PIXEL		short
#define PIXEL_SIZE	sizeof(PIXEL)

typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;

//uint8_t *chunky; // [4096];
u16 bitmask;
u32 plane_offset;
s16 plane_index;
u16 *bitmap;
u16 w0,w1,w2,w3;
u32 bitmap_increment;
uint8_t i, idx, c;
uint8_t clut[] = {0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1};
uint8_t chunky[8];

#if 1
#define DEBUG

#ifdef DEBUG
#include "relocate.h"
extern short debug;
extern Access *access;
extern char err_msg[];

void debug_out (char *text1, int w, int old_w, int h, int src_x, int src_y, int dst_x, int dst_y)
{
        char buf[10];
        access->funcs.puts (text1);
        access->funcs.ltoa (buf, w, 10);
        access->funcs.puts (buf);

        if (old_w > 0)
        {
                access->funcs.puts ("(");
                access->funcs.ltoa (buf, old_w, 10);
                access->funcs.puts (buf);
                access->funcs.puts (")");
        }

        access->funcs.puts (",");
        access->funcs.ltoa (buf, h, 10);
        access->funcs.puts (buf);
        access->funcs.puts (" from ");
        access->funcs.ltoa (buf, src_x, 10);
        access->funcs.puts (buf);
        access->funcs.puts (",");
        access->funcs.ltoa (buf, src_y, 10);
        access->funcs.puts (buf);
        access->funcs.puts (" to ");
        access->funcs.ltoa (buf, dst_x, 10);
        access->funcs.puts (buf);
        access->funcs.puts (",");
        access->funcs.ltoa (buf, dst_y, 10);
        access->funcs.puts (buf);
        access->funcs.puts ("\x0d\x0a");
}
#endif


/*
 * Make it as easy as possible for the C compiler.
 * The current code is written to produce reasonable results with Lattice C.
 * (long integers, optimize: [x xx] time)
 * - One function for each operation -> more free registers
 * - 'int' is the default type
 * - some compilers aren't very smart when it comes to *, / and %
 * - some compilers can't deal well with *var++ constructs
 */

#ifdef BOTH
static void
s_blit_copy (PIXEL *src_addr, int src_line_add,
             PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
             int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *src_addr++;
#ifdef BOTH
                        * (volatile PIXEL *)dst_addr_fast++ = v, 0;  /* Silly compiler... */
#endif
                        *dst_addr++ = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
s_blit_or (PIXEL *src_addr, int src_line_add,
           PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
           int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *src_addr++;
#ifdef BOTH
                        v |= *dst_addr_fast;
                        * (volatile PIXEL *)dst_addr_fast++ = v, 0;  /* Silly compiler... */
                        *dst_addr++ = v;
#else
                        *dst_addr++ |= v;
#endif
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
s_blit (PIXEL *src_addr, int src_line_add,
        PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
        int w, int h, int operation)
{
        int i, j;
        PIXEL v, vs, vd;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        vs = *src_addr++;
#ifdef BOTH
                        vd = *dst_addr_fast;
#else
                        vd = *dst_addr;
#endif

                        switch (operation)
                        {
                        case 0:
                                v = 0;
                                break;

                        case 1:
                                v = vs & vd;
                                break;

                        case 2:
                                v = vs & ~vd;
                                break;

                        case 3:
                                v = vs;
                                break;

                        case 4:
                                v = ~vs & vd;
                                break;

                        case 5:
                                v = vd;
                                break;

                        case 6:
                                v = vs ^ vd;
                                break;

                        case 7:
                                v = vs | vd;
                                break;

                        case 8:
                                v = ~ (vs | vd);
                                break;

                        case 9:
                                v = ~ (vs ^ vd);
                                break;

                        case 10:
                                v = ~vd;
                                break;

                        case 11:
                                v = vs | ~vd;
                                break;

                        case 12:
                                v = ~vs;
                                break;

                        case 13:
                                v = ~vs | vd;
                                break;

                        case 14:
                                v = ~ (vs & vd);
                                break;

                        case 15:
                                v = 0xffff;
                                break;
                        }

#ifdef BOTH
                        * (volatile PIXEL *)dst_addr_fast++ = v, 0;  /* Silly compiler... */
#endif
                        *dst_addr++ = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
s_pan_backwards_copy (PIXEL *src_addr, int src_line_add,
                      PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                      int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *--src_addr;
#ifdef BOTH
                        * (volatile PIXEL *)--dst_addr_fast = v, 0;  /* Silly compiler... */
#endif
                        *--dst_addr = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
s_pan_backwards_or (PIXEL *src_addr, int src_line_add,
                    PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                    int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *--src_addr;
#ifdef BOTH
                        v |= *--dst_addr_fast;
                        * (volatile PIXEL *)dst_addr_fast = v, 0;  /* Silly compiler... */
                        *--dst_addr = v;
#else
                        *--dst_addr |= v;
#endif
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
s_pan_backwards (PIXEL *src_addr, int src_line_add,
                 PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                 int w, int h, int operation)
{
        int i, j;
        PIXEL v, vs, vd;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        vs = *--src_addr;
#ifdef BOTH
                        vd = *--dst_addr_fast;
#else
                        vd = *--dst_addr;
#endif

                        switch (operation)
                        {
                        case 0:
                                v = 0;
                                break;

                        case 1:
                                v = vs & vd;
                                break;

                        case 2:
                                v = vs & ~vd;
                                break;

                        case 3:
                                v = vs;
                                break;

                        case 4:
                                v = ~vs & vd;
                                break;

                        case 5:
                                v = vd;
                                break;

                        case 6:
                                v = vs ^ vd;
                                break;

                        case 7:
                                v = vs | vd;
                                break;

                        case 8:
                                v = ~ (vs | vd);
                                break;

                        case 9:
                                v = ~ (vs ^ vd);
                                break;

                        case 10:
                                v = ~vd;
                                break;

                        case 11:
                                v = vs | ~vd;
                                break;

                        case 12:
                                v = ~vs;
                                break;

                        case 13:
                                v = ~vs | vd;
                                break;

                        case 14:
                                v = ~ (vs & vd);
                                break;

                        case 15:
                                v = 0xffff;
                                break;
                        }

#ifdef BOTH
                        * (volatile PIXEL *)dst_addr_fast = v, 0;  /* Silly compiler... */
#endif
                        *dst_addr = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}

#define BOTH_WAS_ON
#endif
#undef BOTH

/*
 * The functions below are exact copies of those above.
 * The '#undef BOTH' makes sure that this works as it should
 * when no shadow buffer is available
 */

static void
blit_copy (PIXEL *src_addr, int src_line_add,
           PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
           int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *src_addr++;
#ifdef BOTH
                        *dst_addr_fast++ = v;
#endif
                        *dst_addr++ = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
blit_or (PIXEL *src_addr, int src_line_add,
         PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
         int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *src_addr++;
#ifdef BOTH
                        v |= *dst_addr_fast;
                        *dst_addr_fast++ = v;
                        *dst_addr++ = v;
#else
                        *dst_addr++ |= v;
#endif
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
blit (PIXEL *src_addr, int src_line_add,
      PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
      int w, int h, int operation)
{
        int i, j;
        PIXEL v, vs, vd;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        vs = *src_addr++;
#ifdef BOTH
                        vd = *dst_addr_fast;
#else
                        vd = *dst_addr;
#endif

                        switch (operation)
                        {
                        case 0:
                                v = 0;
                                break;

                        case 1:
                                v = vs & vd;
                                break;

                        case 2:
                                v = vs & ~vd;
                                break;

                        case 3:
                                v = vs;
                                break;

                        case 4:
                                v = ~vs & vd;
                                break;

                        case 5:
                                v = vd;
                                break;

                        case 6:
                                v = vs ^ vd;
                                break;

                        case 7:
                                v = vs | vd;
                                break;

                        case 8:
                                v = ~ (vs | vd);
                                break;

                        case 9:
                                v = ~ (vs ^ vd);
                                break;

                        case 10:
                                v = ~vd;
                                break;

                        case 11:
                                v = vs | ~vd;
                                break;

                        case 12:
                                v = ~vs;
                                break;

                        case 13:
                                v = ~vs | vd;
                                break;

                        case 14:
                                v = ~ (vs & vd);
                                break;

                        case 15:
                                v = 0xffff;
                                break;
                        }

#ifdef BOTH
                        *dst_addr_fast++ = v;
#endif
                        *dst_addr++ = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
pan_backwards_copy (PIXEL *src_addr, int src_line_add,
                    PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                    int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *--src_addr;
#ifdef BOTH
                        *--dst_addr_fast = v;
#endif
                        *--dst_addr = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
pan_backwards_or (PIXEL *src_addr, int src_line_add,
                  PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
                  int w, int h)
{
        int i, j;
        PIXEL v;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        v = *--src_addr;
#ifdef BOTH
                        v |= *--dst_addr_fast;
                        *dst_addr_fast = v;
                        *--dst_addr = v;
#else
                        *--dst_addr |= v;
#endif
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


static void
pan_backwards (PIXEL *src_addr, int src_line_add,
               PIXEL *dst_addr, PIXEL *dst_addr_fast, int dst_line_add,
               int w, int h, int operation)
{
        int i, j;
        PIXEL v, vs, vd;

        for (i = h - 1; i >= 0; i--)
        {
                for (j = w - 1; j >= 0; j--)
                {
                        vs = *--src_addr;
#ifdef BOTH
                        vd = *--dst_addr_fast;
#else
                        vd = *--dst_addr;
#endif

                        switch (operation)
                        {
                        case 0:
                                v = 0;
                                break;

                        case 1:
                                v = vs & vd;
                                break;

                        case 2:
                                v = vs & ~vd;
                                break;

                        case 3:
                                v = vs;
                                break;

                        case 4:
                                v = ~vs & vd;
                                break;

                        case 5:
                                v = vd;
                                break;

                        case 6:
                                v = vs ^ vd;
                                break;

                        case 7:
                                v = vs | vd;
                                break;

                        case 8:
                                v = ~ (vs | vd);
                                break;

                        case 9:
                                v = ~ (vs ^ vd);
                                break;

                        case 10:
                                v = ~vd;
                                break;

                        case 11:
                                v = vs | ~vd;
                                break;

                        case 12:
                                v = ~vs;
                                break;

                        case 13:
                                v = ~vs | vd;
                                break;

                        case 14:
                                v = ~ (vs & vd);
                                break;

                        case 15:
                                v = 0xffff;
                                break;
                        }

#ifdef BOTH
                        *dst_addr_fast = v;
#endif
                        *dst_addr = v;
                }

                src_addr += src_line_add;
                dst_addr += dst_line_add;
#ifdef BOTH
                dst_addr_fast += dst_line_add;
#endif
        }
}


#ifdef BOTH_WAS_ON
#define BOTH
#endif

#endif

void *memcpy(void *dest, const void *src, size_t n)
{
size_t i;
    for (i = 0; i < n; i++)
    {
        ((char*)dest)[i] = ((char*)src)[i];
    }
}


void convertVdiBitmapToIndexed(MFDB *mfdb, uint8_t *indexmap)
{
   s16 y, xw;
   u16 bitmask;
   uint8_t index;
   u32 plane_offset;
   s16 plane_index;
   u16 *bitmap;
   u16 w0,w1,w2,w3;
   u32 bitmap_increment;
   uint8_t i, idx, c;
   //uint8_t clut[] = {0, 15, 1, 2, 4, 6, 3, 5, 7, 8, 9, 10, 12, 14, 13};
   uint8_t clut[] = {0, 2, 3, 6, 4, 7, 5, 8, 9, 10, 11, 14, 12, 15, 13, 1};

   bitmap = (u16 *)(mfdb->address);
   bitmap_increment = mfdb->standard ? 1 : mfdb->bitplanes;
   plane_offset = mfdb->standard ? mfdb->wdwidth * mfdb->height : 1;

   for (y = 0; y < mfdb->height; y++)
   {
      for (xw = 0; xw < mfdb->wdwidth; xw++)
      {
#if 0
         for(bitmask = 0x8000; bitmask; bitmask >>= 1)
         //bitmask = 0x8000;
         //for(i=0;i<8;i++)
         {
            index = 0;
           
            for(plane_index = mfdb->bitplanes - 1; plane_index >= 0; plane_index--)
            //for(plane_index = 0; plane_index < mfdb->bitplanes ; plane_index++)
            {
            	index += index;

            	if(bitmap[plane_offset * plane_index] & bitmask)
            	   index += 1;
            }

            bitmask >>= 1;

            for(plane_index = mfdb->bitplanes - 1; plane_index >= 0; plane_index--)
            //for(plane_index = 0; plane_index < mfdb->bitplanes ; plane_index++)
            {
            	index += index;

            	if(bitmap[plane_offset * plane_index] & bitmask)
            	   index += 1;
            }


            *indexmap++ = index;
            bitmask >>= 1;
         }
         
         bitmap += bitmap_increment;
   //debug_out("Blitting: 2", 1, 1, 1, 1, 1, 1, 1);
#else

            bitmask=0x8000;
            w0=bitmap[0];
            w1=bitmap[plane_offset];
            w2=bitmap[plane_offset*2];
            w3=bitmap[plane_offset*3];
            for (i=0;i<8;i++)
            {
              // Pour décoder le pixel n°i, il faut traiter les 4 words sur leur bit
              // correspondant à celui du masque
            
              idx=((w0 & bitmask)?0x01:0x00) |
                     ((w1 & bitmask)?0x02:0x00) |
                     ((w2 & bitmask)?0x04:0x00) |
                     ((w3 & bitmask)?0x08:0x00);
              bitmask>>=1;
              *indexmap = clut[idx];
              *indexmap <<= 4;
              idx=((w0 & bitmask)?0x01:0x00) |
                     ((w1 & bitmask)?0x02:0x00) |
                     ((w2 & bitmask)?0x04:0x00) |
                     ((w3 & bitmask)?0x08:0x00);
              *indexmap++ +=clut[idx];
              bitmask>>=1;
            }
            bitmap += bitmap_increment;
#endif
      }
   }
}

long CDECL
c_blit_area (Virtual *vwk, MFDB *src, long src_x, long src_y,
             MFDB *dst, long dst_x, long dst_y,
             long w, long h, long operation)
{
        Workstation *wk;
        PIXEL *src_addr, *dst_addr, *dst_addr_fast;
        uint8_t *addrcounter;
        int src_wrap, dst_wrap;
        int src_line_add, dst_line_add;
        unsigned long src_pos, dst_pos;
        int from_screen, to_screen;
        VDP_COPY_XY_XY blitbox;
        VDP_BOX ramblitbox;
        uint8_t direction;
        uint16_t pixelcount, count;
        uint8_t pixl;
        uint8_t *srcaddr, *dstaddr;
        wk = vwk->real_address;
        from_screen = 0;

        if (!src || !src->address || (src->address == wk->screen.mfdb.address))  		/* From screen? */
        {
                src_wrap = wk->screen.wrap;

                if (! (src_addr = wk->screen.shadow.address))
                {
                        src_addr = wk->screen.mfdb.address;
                }

                from_screen = 1;
        }
        else
        {
                src_wrap = (long)src->wdwidth * 2 * src->bitplanes;
                src_addr = src->address;
                srcaddr = (uint8_t *)src->address;

        }

        //src_pos = (short)src_y * (long)src_wrap + (src_x >> 1); //* PIXEL_SIZE;
        src_pos = (short)src_y * (long)src_wrap + src_x * PIXEL_SIZE;
        //src_line_add = src_wrap - w * PIXEL_SIZE;
        src_line_add = (src_wrap - w) >> 1;// * PIXEL_SIZE;
        to_screen = 0;

        if (!dst || !dst->address || (dst->address == wk->screen.mfdb.address))  		/* To screen? */
        {
                dst_wrap = wk->screen.wrap;
                dst_addr = wk->screen.mfdb.address;
                to_screen = 1;
        }
        else
        {
                dst_wrap = (long)dst->wdwidth * 2 * dst->bitplanes;
                dst_addr = dst->address;
                dstaddr = (uint8_t *)dst->address;
        }

        dst_pos = (short)dst_y * (long)dst_wrap + (dst_x >> 1); //* PIXEL_SIZE;
        //dst_line_add = dst_wrap - w * PIXEL_SIZE;
        dst_line_add = (dst_wrap - w) >> 1; // * PIXEL_SIZE;

        if (from_screen && to_screen) /* From screen to screen (VRAM to VRAM blits only) */
        {
                direction = 0;
                blitbox.sourceX = src_x;
                blitbox.sourceY = src_y;
                blitbox.destX = dst_x;
                blitbox.destY = dst_y;
                blitbox.width = w;
                blitbox.height = h;

                if (src_x < dst_x) /* move right */
                {
                        direction |= 4;
                        blitbox.sourceX += w - 1;
                        blitbox.destX += w - 1;
                }

                if (src_y < dst_y) /* move down */
                {
                        direction |= 8;
                        blitbox.sourceY += h - 1;
                        blitbox.destY += h - 1;
                }

                VDPWriteReg (VDP_ARG, (uint8_t)direction);
#if 1

                switch (operation)
                {
                case 1:				/* Replace */
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCANDSC);
                        break;

                case 2:				/* Transparent */
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCORSC);
                        break;

                case 3:
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                        break;

                case 6:
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCEORSC);
                        break;

                case 7:
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCORSC);
                        break;

                case 10:
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCNOTSC);
                        break;

                default:
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                        break;
                }

#else
                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
#endif
                v9990_SetCmdWriteMask (0xffff);
                v9990_CopyXYToXY (&blitbox);
                wait_vdp();
                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                VDPWriteReg (VDP_ARG, 0);
        }
        else
                if (!from_screen && to_screen) /* From memory to screen */
                {
                        ramblitbox.left = dst_x;
                        ramblitbox.top = dst_y;
                        ramblitbox.width = src->wdwidth << 4; //w;
                        ramblitbox.height = h;

#if 0
                        if(src->bitplanes == 4 && !src->standard) {
                            chunky = (uint8_t *)access->funcs.allocate_block(src->height * (src->wdwidth >> 1) );	/* Assume malloc won't fail. */
                            convertVdiBitmapToIndexed(src, chunky);
                        }
#endif
                        VDPWriteReg (VDP_ARG, 0);
#if 1

                        switch (operation)
                        {
                        case 1:				/* Replace */
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                break;

                        case 2:				/* Transparent */
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                break;

                        case 3:
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                break;

                        case 7:
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCORSC);
                                break;

                        default:
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                break;
                        }

#else
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
#endif
                        v9990_SetCmdWriteMask (0xffff);
                        v9990_SetupCopyRamToXY (&ramblitbox);
                        pixelcount = (h * ramblitbox.width) >> 1;
                        if(src->bitplanes == 4 && !src->standard) {
#if 0
                            v9990_CopyRamToXY (chunky, pixelcount);
			    access->funcs.free_block(chunky);	/* Release chunky buffer */
#endif
                            bitmap_increment = src->standard ? 1 : src->bitplanes;
                            plane_offset = src->standard ? src->wdwidth * src->height : 1;
                            //bitmap = (u16 *)(src->address + src_pos);
                            bitmap = (u16 *)(src->address + src_pos);
                            pixelcount >> 3;
                            while(pixelcount--) {
                                bitmask=0x8000;
                                w0=bitmap[0];
                                w1=bitmap[plane_offset];
                                w2=bitmap[plane_offset*2];
                                w3=bitmap[plane_offset*3];
                                for (i=0;i<8;i++)
                                {
                                  // Pour décoder le pixel n°i, il faut traiter les 4 words sur leur bit
                                  // correspondant à celui du masque
                                
                                  idx=((w0 & bitmask)?0x01:0x00) |
                                         ((w1 & bitmask)?0x02:0x00) |
                                         ((w2 & bitmask)?0x04:0x00) |
                                         ((w3 & bitmask)?0x08:0x00);
                                  bitmask>>=1;
                                  chunky[i] = clut[idx];
                                  chunky[i] <<= 4;
                                  idx=((w0 & bitmask)?0x01:0x00) |
                                         ((w1 & bitmask)?0x02:0x00) |
                                         ((w2 & bitmask)?0x04:0x00) |
                                         ((w3 & bitmask)?0x08:0x00);
                                  chunky[i] +=clut[idx];
                                  bitmask>>=1;
                                }
                                bitmap += bitmap_increment;
                                v9990_CopyRamToXY (chunky, 8); /* 16px/4bpp => 8 bytes */
                            }
                        } else {
                            v9990_CopyRamToXY (srcaddr, pixelcount);
                        }
                        VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                        VDPWriteReg (VDP_ARG, 0);
                }
                else
                        if (from_screen && !to_screen) /* From screen to memory */
                        {
                                ramblitbox.left = src_x;
                                ramblitbox.top = src_y;
                                ramblitbox.width = dst->wdwidth << 4; //w;
                                ramblitbox.height = h;
                                v9990_SetCmdWriteMask (0xffff);
                                VDPWriteReg (VDP_LOP, VDP_LOP_WCSC);
                                VDPWriteReg (VDP_ARG, 0);
                                v9990_SetupCopyXYToRam (&ramblitbox);
                                pixelcount = (h * ramblitbox.width) >> 1;
                                dst->standard = 1;
                                //pixelcount = dst->wdwidth >> 1;
                                //addrcounter = dstaddr + dst_pos;
                                //dstaddr += dst_pos >> 2;
                                v9990_CopyXYToRam (dstaddr, pixelcount);
                                //while(h--)
                                //{
                                //        for(count=0;count<pixelcount;count++)
                                //        {
                                //                *addrcounter++ = READ_VDP(VDP_CMDDAT);
                                //        }
                                //        addrcounter += dst_line_add;
                                //}
                        }
                        else
                        {
                                return 0;
                        }

        return 1;	/* Return as completed */
}
