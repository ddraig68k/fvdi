/*
 * uae_mous.c - Mouse cursor functions
 * This is part of the WinUAE RTG driver for fVDI
 *
 * Copyright (C) 2017 Vincent Riviere
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*#define ENABLE_KDEBUG*/

#include "fvdi.h"
#include <stdint.h>
#include "v9990.h"

uint8_t ms_arrow[] =
{
        0x7f, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff,
        0x07, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff, 0x00, 0xff, 0xff, 0xff,
        0x00, 0x7f, 0xff, 0xff, 0x00, 0x3f, 0xff, 0xff, 0x00, 0x3f, 0xff, 0xff, 0x01, 0xff, 0xff, 0xff,
        0x00, 0xff, 0xff, 0xff, 0x10, 0xff, 0xff, 0xff, 0x38, 0x7f, 0xff, 0xff, 0xfc, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

uint8_t m_temp[128];

/* External data and functions */
extern Driver *me;
extern long CDECL c_xpand_area (Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation, long colour);
extern long CDECL c_blit_area (Virtual *vwk, MFDB *src, long src_x, long src_y, MFDB *dst, long dst_x, long dst_y, long w, long h, long operation);

/* We must remember if the mouse is visible or not */
static uint8_t mouse_visible = 0;

/* Current mouse shape */
static Mouse *pmouse;

/* MFDB used to draw the mouse */
static MFDB mouse_mfdb = {0, 16, 16, 1, 1, 1};

/* We must save the mouse background */
static short backup_data[16 * 16];
static MFDB mouse_backup_mfdb = {backup_data, 16, 16, 1, 0, 16};
static short backup_x, backup_y, backup_w, backup_h;

static void clip_mouse (Virtual *vwk, short x, short y, short *pw, short *ph)
{
        short w = 16;
        short h = 16;
        short screen_w = vwk->real_address->screen.mfdb.width;
        short screen_h = vwk->real_address->screen.mfdb.height;
        *pw = x + w > screen_w ? screen_w - x : w;
        *ph = y + h > screen_h ? screen_h - y : h;
}

static long draw_mouse (Virtual *vwk, short x, short y)
{
        short w, h;
        uint8_t low_x, low_y, high_x, high_y;
        x -= pmouse->hotspot.x;
        y -= pmouse->hotspot.y;
        clip_mouse (vwk, x, y, &w, &h);
        /* Draw the mask, transparent mode */
        mouse_mfdb.address = pmouse->mask;
        //c_expand_area(vwk, &mouse_mfdb, 0, 0, 0, x, y, w, h, 2, pmouse->colour.background);
        /* Draw the data, transparent mode */
        mouse_mfdb.address = pmouse->data;
        //c_expand_area(vwk, &mouse_mfdb, 0, 0, 0, x, y, w, h, 2, pmouse->colour.foreground);
        low_x = (x & 0xff);
        high_x = ((x >> 8) & 0x03) + 64;
        low_y = (y & 0xff);
        high_y = ((y >> 8) & 0x01);
        //v9990_SetBackdropColor(15-(uint8_t)(buffer[0]&0x07));
        wait_vdp();
        v9990_SetVRAMWrite (VDP_CURSOR0_ATTRIB);
        asm volatile ("move.b   %0, 0x3df600"::"m" (low_y));	//
        asm volatile ("move.b   #0, 0x3df600"); /* pattern no 0 */
        asm volatile ("move.b   %0, 0x3df600"::"m" (high_y));	//
        asm volatile ("move.b   #0, 0x3df600"); /* pattern no 0 */
        asm volatile ("move.b   %0, 0x3df600"::"m" (low_x));	//
        asm volatile ("move.b   #0, 0x3df600"); /* pattern no 0 */
        asm volatile ("move.b   %0, 0x3df600"::"m" (high_x));	//
        asm volatile ("move.b   #0, 0x3df600"); /* pattern no 0 */
        return 1;
}

static void hide_mouse (Virtual *vwk)
{
        if (mouse_visible)
        {
                /* Restore the backup */
                //	c_blit_area(vwk, &mouse_backup_mfdb, 0, 0, 0, backup_x, backup_y, backup_w, backup_h, 3);
                v9990_SpritesDisable();
                mouse_visible = 0;
        }
}

static void show_mouse (Virtual *vwk, short x, short y)
{
        /* If a mouse shape has not yet been set, just ignore */
        if (!pmouse)
        {
                return;
        }

        if (mouse_visible)
        {
                hide_mouse (vwk);
        }

        /* Make a new backup */
        backup_x = x - pmouse->hotspot.x;
        backup_y = y - pmouse->hotspot.y;
        clip_mouse (vwk, backup_x, backup_y, &backup_w, &backup_h);
        //c_blit_area(vwk, 0, backup_x, backup_y, &mouse_backup_mfdb, 0, 0, backup_w, backup_h, 3);
        draw_mouse (vwk, x, y);
        v9990_SpritesEnable();
        mouse_visible = 1;
}

#if 1
static void uploadShapeToVRAM (uint8_t *shape)
{
        uint16_t i;
        VDP_BOX box;
        VDP_COPY_XY_VRAM box2;
        uint8_t shape2[64];
#if 0
        v9990_SetVRAMWrite (VDP_CURSOR0_PAT_DATA); // keep 1024*212
        v9990_CopyRamToVram (shape, 32);
        box.top = 1024;
        box.left = 0;
        box.width = 32;
        box.height = 32;
        //v9990_SetupRamCharToXY(&box);
        //v9990_SetupCopyRamToXY(&box);
        v9990_SetVRAMWrite (VDP_CURSOR0_PAT_DATA); // keep 1024*212

        for (i = 0; i < 128; i++)
        {
                //    asm volatile ("move.b   %0, 0x3df604"::"m" (*shape++));	//
                asm volatile ("move.b   #0xf0, 0x3df600"); // Clear all other sprites
                //shape2[i] = 0xf0;
        }

        box2.sourceX = 0;
        box2.sourceY = 513;
        box2.width = 16;
        box2.height = 16;
        box2.destAddress = VDP_CURSOR0_PAT_DATA;
        v9990_CopyXYTOVram (box2);
        v9990_SetVRAMWrite (VDP_CURSOR0_PAT_DATA); // keep 1024*212
        v9990_CopyRamToVram (shape2, 32);
#endif
        v9990_SetVRAMWrite (VDP_CURSOR0_PAT_DATA); // keep 1024*212

#if 0
        for (i = 0; i < sizeof (ms_arrow); i++)
        {
                //ms_arrow[i]=~ms_arrow[i];
                m_temp[i] = ~ms_arrow[i];
        }
#else
        for (i = 0; i < 64; i++)
        {
                //ms_arrow[i]=~ms_arrow[i];
                m_temp[i++] = *shape++;
                m_temp[i++] = *shape++;
                m_temp[i++] = 0;
                m_temp[i] = 0;
        }
        for (i = 65; i < 128; i++) {
                m_temp[i++] = 0;
        }
#endif

        //v9990_CopyRamToVram (ms_arrow,sizeof (ms_arrow));
        v9990_CopyRamToVram (m_temp, sizeof (m_temp));
        //v9990_CopyRamToVram (shape, 32);
        v9990_SetVRAMWrite (VDP_CURSOR0_ATTRIB);

        for (i = 0; i < (2 * 4); i++)
        {
                asm volatile ("move.b   #0x10, 0x3df600"); // Clear all other sprites
                asm volatile ("move.b   #0x10, 0x3df600"); // clear all other sprites
        }

        WRITE_VDP (VDP_REGSEL, 28); // Sprites
        //WRITE_VDP(VDP_REGDAT,0x2);  // palette number
        WRITE_VDP (VDP_REGDAT, 0x0); // palette number
        WRITE_VDP (VDP_REGSEL, 25); // Sprites base address
        WRITE_VDP (VDP_REGDAT, 0x4); // 20000
        wait_vdp();
        v9990_SetVRAMWrite (VDP_CURSOR0_ATTRIB);
        asm volatile ("move.b   #50, 0x3df600");
        asm volatile ("move.b   #0, 0x3df600");
        asm volatile ("move.b   #0, 0x3df600");
        asm volatile ("move.b   #0, 0x3df600");
        asm volatile ("move.b   #50, 0x3df600");
        asm volatile ("move.b   #0, 0x3df600");
        asm volatile ("move.b   #64, 0x3df600");
        asm volatile ("move.b   #0, 0x3df600");
}
#endif

long CDECL
c_mouse_draw (Workstation *wk, long x, long y, Mouse *mouse)
{
        /* See mouse_timer and wk_r_mouse in engine/mouse.s for parameters meaning */
        Virtual *vwk = me->default_vwk;
//        static ma=0;
//        if(ma==0)
//        {
//		pmouse = mouse;
//                ma=1;
//        }

        //KDEBUG(("c_mouse_draw %ld,%ld %p (old=%lu)\n", x & 0xffff, y, mouse, (unsigned long)x >> 16));
        if ((long)mouse > 7) /* Set new mouse cursor shape */
        {
                uint8_t mouse_was_visible = mouse_visible;

                if (mouse_was_visible)
                {
                        hide_mouse (vwk);
                }

                pmouse = mouse;
                uploadShapeToVRAM ((uint8_t *)pmouse->data);

                if (mouse_was_visible)
                {
                        show_mouse (vwk, (short)x, (short)y);
                }

                return 1;
        }
        else
        {
                switch ((long)mouse)
                {
                case 0: /* Move visible */
                case 4: /* Move visible forced (wk_mouse_forced) */
                        show_mouse (vwk, (short)x, (short)y);
                        return 1;

                case 1: /* Move hidden */
                case 5: /* Move hidden forced (wk_mouse_forced) */
                        return 1;

                case 2: /* Hide */
                        hide_mouse (vwk);
                        return 1;

                case 3: /* Show */
                        show_mouse (vwk, (short)x, (short)y);
                        return 1;
                }
        }

        return 0;
}
