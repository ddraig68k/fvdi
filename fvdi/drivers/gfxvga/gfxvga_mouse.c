#include <stdint.h>
#include "driver.h"
#include "gfxvga.h"

#include "fvdi.h"
#include "../bitplane/bitplane.h"

#define GFXVGA_CURSOR_WIDTH 16
#define GFXVGA_CURSOR_HEIGHT 16
#define GFXVGA_HW_CURSOR_WIDTH 32
#define GFXVGA_HW_CURSOR_HEIGHT 32
#define GFXVGA_CURSOR_WORDS_PER_ROW 4
#define GFXVGA_CURSOR_TOTAL_WORDS (GFXVGA_HW_CURSOR_HEIGHT * GFXVGA_CURSOR_WORDS_PER_ROW)
#define GFXVGA_CURSOR_MAX_X 639
#define GFXVGA_CURSOR_MAX_Y 479

/*
 * Hardware cursor format contract (matches CursorSprite.vhd):
 * - Surface: 32x32 pixels, 2bpp packed.
 * - Storage: 128 x 16-bit words (4 words per row).
 * - Each word encodes 8 pixels as [15:14]=px0 .. [1:0]=px7.
 * - Pixel index mapping: 00 transparent, 01 background, 11 foreground.
 *
 * fVDI cursor input is 16x16 mask/data. We map it 1:1 into the top-left
 * 16x16 region of the 32x32 hardware surface; the rest stays transparent.
 */

static volatile short mouse_draw_busy = 0;
static short cursor_initialized = 0;
static long cursor_colour_state = -1;
static unsigned short cursor_mask[GFXVGA_CURSOR_HEIGHT] = {
    0xffff, 0x7ffe, 0x3ffc, 0x1ff8,
    0x0ff0, 0x07e0, 0x03c0, 0x0180,
    0x0180, 0x03c0, 0x07e0, 0x0ff0,
    0x1ff8, 0x3ffc, 0x7ffe, 0xffff
};
static unsigned short cursor_data[GFXVGA_CURSOR_HEIGHT] = {
    0x0000, 0x3ffc, 0x1ff8, 0x0ff0,
    0x07e0, 0x03c0, 0x0180, 0x0000,
    0x0000, 0x0180, 0x03c0, 0x07e0,
    0x0ff0, 0x1ff8, 0x3ffc, 0x0000
};
static unsigned short cursor_words[GFXVGA_CURSOR_TOTAL_WORDS];

static void build_cursor_data(const unsigned short *mask, const unsigned short *data)
{
    int row;

    for (row = 0; row < GFXVGA_CURSOR_TOTAL_WORDS; row++)
        cursor_words[row] = 0;

    for (row = 0; row < GFXVGA_CURSOR_HEIGHT; row++) {
        unsigned short mask_bits = mask[row];
        unsigned short data_bits = data[row];
        int col;

        for (col = 0; col < GFXVGA_CURSOR_WIDTH; col++) {
            unsigned short bit = (unsigned short)(0x8000u >> col);
            unsigned short idx;
            int word_index;
            int pixel_in_word;
            int shift;

            if (data_bits & bit)
                idx = 3;
            else if (mask_bits & bit)
                idx = 1;
            else
                idx = 0;

            word_index = row * GFXVGA_CURSOR_WORDS_PER_ROW + (col >> 3);
            pixel_in_word = col & 7;
            shift = 14 - (pixel_in_word * 2);
            cursor_words[word_index] |= (unsigned short)(idx << shift);
        }
    }
}

static int clamp_palette_index(int idx, int max_idx)
{
    if (idx < 0)
        return 0;
    if (idx > max_idx)
        return max_idx;
    return idx;
}

static void upload_cursor_palette(Workstation *wk)
{
    unsigned short background = 0x0000;
    unsigned short foreground = 0xffff;
    long colour_state = -1;

    if (wk && wk->screen.palette.colours) {
        Colour *global_palette = wk->screen.palette.colours;
        unsigned short *realp;
        int max_idx = 255;
        int bg_idx;
        int fg_idx;

        if (wk->screen.palette.size > 0 && wk->screen.palette.size - 1 < max_idx)
            max_idx = wk->screen.palette.size - 1;

        bg_idx = clamp_palette_index((int)wk->mouse.colour.background, max_idx);
        fg_idx = clamp_palette_index((int)wk->mouse.colour.foreground, max_idx);

        colour_state = ((unsigned long)(unsigned short)bg_idx << 16)
                     | (unsigned short)fg_idx;
        realp = (unsigned short *)&global_palette[bg_idx].real;
        background = *realp;
        realp = (unsigned short *)&global_palette[fg_idx].real;
        foreground = *realp;
    }

    if (colour_state == cursor_colour_state)
        return;

    vdp_hw_cursor_colors(background, foreground, foreground);
    cursor_colour_state = colour_state;
}

static void upload_cursor_shape(void)
{
    int i;

    vdp_hw_cursor_addr(0);
    for (i = 0; i < GFXVGA_CURSOR_TOTAL_WORDS; i++)
        vdp_hw_cursor_data(cursor_words[i]);
}

static void ensure_cursor_initialized(void)
{
    if (cursor_initialized)
        return;

    build_cursor_data(cursor_mask, cursor_data);
    upload_cursor_shape();
    upload_cursor_palette(NULL);
    vdp_hw_cursor_pos(0, 0);
    vdp_hw_cursor_enable(0);
    cursor_initialized = 1;
}

static void set_mouse_shape(Mouse *mouse)
{
    int i;

    if (!mouse)
        return;

    for (i = 0; i < GFXVGA_CURSOR_HEIGHT; i++) {
        cursor_mask[i] = (unsigned short)mouse->mask[i];
        cursor_data[i] = (unsigned short)mouse->data[i];
    }

    build_cursor_data(cursor_mask, cursor_data);
    upload_cursor_shape();
}

static void hide_mouse(void)
{
    ensure_cursor_initialized();
    vdp_hw_cursor_enable(0);
}

static void show_mouse(Workstation *wk, long x, long y)
{
    long pos_x;
    long pos_y;

    ensure_cursor_initialized();

    pos_x = (short)(x & 0xffffL);
    pos_y = (short)y;
    if (wk) {
        pos_x -= wk->mouse.hotspot.x;
        pos_y -= wk->mouse.hotspot.y;
    }

    if (pos_x < 0)
        pos_x = 0;
    else if (pos_x > GFXVGA_CURSOR_MAX_X)
        pos_x = GFXVGA_CURSOR_MAX_X;

    if (pos_y < 0)
        pos_y = 0;
    else if (pos_y > GFXVGA_CURSOR_MAX_Y)
        pos_y = GFXVGA_CURSOR_MAX_Y;

    vdp_hw_cursor_pos((UWORD)pos_x, (UWORD)pos_y);
    vdp_hw_cursor_enable(1);
}

long CDECL c_mouse_draw(Workstation *wk, long x, long y, Mouse *mouse)
{
    long mouseparm = (long)mouse;

    if (mouse_draw_busy)
        return 0;
    mouse_draw_busy = 1;

    x &= 0xffffL;

    if (mouseparm > 7) {
        ensure_cursor_initialized();
        upload_cursor_palette(wk);
        if (!fix_shape && mouse && (((ULONG)mouse & 1UL) == 0) && ((ULONG)mouse >= 0x1000UL))
            set_mouse_shape(mouse);
        mouse_draw_busy = 0;
        return 0;
    }

    switch (mouseparm) {
    case 0:
    case 3:
    case 4:
        show_mouse(wk, x, y);
        break;

    case 1:
    case 2:
    case 5:
        hide_mouse();
        break;
    }

    mouse_draw_busy = 0;
    return 0;
}
