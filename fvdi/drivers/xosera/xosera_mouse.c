/*
 * Mouse cursor functions for Xosera
 * This is part of the Xosera driver for fVDI.
 *
 * Copyright (C) 2024 Rob Gowin
 * Copyright (C) 2023 Daniel Cliche
 * Copyright (C) 2023 Xark
 */

#include <stdint.h>
#include "xosera.h"
#include "driver.h"

static void hide_mouse(volatile xmreg_t *const xosera_ptr)
{
    xreg_setw(POINTER_H, 0x0000);
    xreg_setw(POINTER_V, 0x0000);
}

static void
show_mouse(volatile xmreg_t *const xosera_ptr, short x, short y, uint16_t mouse_x_offset, uint16_t mouse_y_scale)
{
    xreg_setw(POINTER_H, x + mouse_x_offset);
    xreg_setw(POINTER_V, 0xF000 | (y * mouse_y_scale));
}

static void set_mouse_image(volatile xmreg_t *const xosera_ptr, uint16_t *mask, uint16_t *data)
{
    // TOS will provide a 16x16x1 mouse image that we need to store into a 32x32x4 array.
    uint16_t pointer_sprite[256];
    uint16_t exp_mask[4], exp_data[4];

    memset(pointer_sprite, 0, 256 * sizeof(uint16_t));
    // Expand the mouse data. Each word of incoming data gets expanded to four words. The other four words on the
    // raster line are skipped and left as zero.
    for (int i = 0; i < 16; i++) {
        // Expand the 1bpp mask and data to 4ppp words.
        // FIXME: This should deal with colors.
        expand_word(exp_mask, mask[i], 0xF, 0x0, 0);
        expand_word(exp_data, data[i], 0x1, 0xF, 0);
        for (int j = 0; j < 4; j++) {
            pointer_sprite[i * 8 + j] = exp_data[j] & exp_mask[j];
        }
    }
    xmem_setw_next_addr(XR_POINTER_ADDR);
    for (int i = 0; i < 256; i++) {
        xmem_setw_next_wait(pointer_sprite[i]);
    }
}

long CDECL
c_mouse_draw(Workstation *UNUSED(wk), long x, long y, Mouse *mouse)
{
    static uint16_t mouse_x_offset = 154;
    static uint16_t mouse_y_scale = 2;
    long mouse_val = (long) mouse;
    short sx = (short) x, sy = (short) y;
    volatile xmreg_t *const xosera_ptr = xv_prep_dyn(me->device);
    switch (mouse_val) {
        case 0: /* Move visible */
        case 4: /* Move visible forced (wk_mouse_forced) */
            show_mouse(xosera_ptr, sx, sy, mouse_x_offset, mouse_y_scale);
            return 1;
        case 2: /* Hide the mouse. */
            hide_mouse(xosera_ptr);
            return 1;
        case 3: /* Show the mouse. */
            show_mouse(xosera_ptr, sx, sy, mouse_x_offset, mouse_y_scale);
            return 1;
        default:
            if (mouse_val > 7) {  /* Set new mouse cursor shape */
                set_mouse_image(xosera_ptr, (uint16_t *) mouse->mask, (uint16_t *) mouse->data);
                return 1;
            } else {
                char numbuf[32];
                access->funcs.ltoa(numbuf, mouse_val, 10);
                access->funcs.puts("xosera_mouse.c: unexpected mouse value ");
                access->funcs.puts(numbuf);
                access->funcs.puts("\n");
            }
            break;
    }
    return 0;
}