#include <stdint.h>
#include "driver.h"
#include "gfxvga.h"

#include "fvdi.h"
#include "driver.h"
#include "../bitplane/bitplane.h"

#define GFXVGA_CURSOR_SPRITE_ADDR 0x001FF000UL
#define GFXVGA_CURSOR_WIDTH 16
#define GFXVGA_CURSOR_HEIGHT 16
#define GFXVGA_CURSOR_BYTES_PER_ROW 8
#define GFXVGA_CURSOR_SPRITE_BYTES (GFXVGA_CURSOR_HEIGHT * GFXVGA_CURSOR_BYTES_PER_ROW)
#define GFXVGA_CURSOR_PALETTE_BANK 1
#define GFXVGA_CURSOR_PALETTE_BASE (GFXVGA_CURSOR_PALETTE_BANK * 256)
#define GFXVGA_CURSOR_BG_INDEX 1
#define GFXVGA_CURSOR_FG_INDEX 2
#define GFXVGA_CURSOR_OVERLAY_WIDTH 320
#define GFXVGA_CURSOR_OVERLAY_HEIGHT 240

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
static unsigned short cursor_sprite_words[GFXVGA_CURSOR_SPRITE_BYTES / sizeof(unsigned short)];

static unsigned short pack_sprite_word(unsigned short p0,
                                       unsigned short p1,
                                       unsigned short p2,
                                       unsigned short p3)
{
    return (unsigned short)((p0 << 12) | (p1 << 8) | (p2 << 4) | p3);
}

static void build_cursor_sprite(const unsigned short *mask, const unsigned short *data)
{
    int row;

    for (row = 0; row < GFXVGA_CURSOR_HEIGHT; row++) {
        int src_row = row * 2;
        unsigned short mask_bits = 0;
        unsigned short data_bits = 0;
        int group;

        if (src_row < GFXVGA_CURSOR_HEIGHT) {
            mask_bits = mask[src_row];
            data_bits = data[src_row];
        }

        for (group = 0; group < 4; group++) {
            unsigned short pixels[4];
            int pixel;

            for (pixel = 0; pixel < 4; pixel++) {
                int out_col = group * 4 + pixel;

                if (out_col < 8 && src_row < GFXVGA_CURSOR_HEIGHT) {
                    int src_col = out_col * 2;
                    unsigned short bit = (unsigned short)(0x8000u >> src_col);

                    if (data_bits & bit)
                        pixels[pixel] = GFXVGA_CURSOR_FG_INDEX;
                    else if (mask_bits & bit)
                        pixels[pixel] = GFXVGA_CURSOR_BG_INDEX;
                    else
                        pixels[pixel] = 0;
                } else {
                    pixels[pixel] = 0;
                }
            }

            cursor_sprite_words[row * 4 + group] = pack_sprite_word(pixels[0], pixels[1], pixels[2], pixels[3]);
        }
    }
}

static short scale_axis(short value, short src_extent, short dst_extent)
{
    long scaled;

    if (src_extent <= 0)
        return value;

    scaled = (long)value * (long)dst_extent;
    if (scaled >= 0)
        scaled += src_extent / 2;
    else
        scaled -= src_extent / 2;

    return (short)(scaled / src_extent);
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

    VDP_REG_WRITE(REG_PALETTE_IDX, GFXVGA_CURSOR_PALETTE_BASE);
    VDP_REG_WRITE(REG_PALETTE_DATA, 0x0000);
    VDP_REG_WRITE(REG_PALETTE_DATA, background);
    VDP_REG_WRITE(REG_PALETTE_DATA, foreground);
    cursor_colour_state = colour_state;
}

static void upload_cursor_shape(void)
{
    vdp_sprite_data_addr(GFXVGA_CURSOR_SPRITE_ADDR);
    vdp_vram_write(GFXVGA_CURSOR_SPRITE_ADDR, cursor_sprite_words, GFXVGA_CURSOR_SPRITE_BYTES);
    vdp_set_sprite_index(0);
    vdp_write_sprite_tile(0);
}

static void ensure_cursor_initialized(void)
{
    if (cursor_initialized)
        return;

    build_cursor_sprite(cursor_mask, cursor_data);
    upload_cursor_shape();
    upload_cursor_palette(NULL);
    vdp_set_control(vdp_get_control() | SPRITE_ENABLE);
    vdp_set_sprite_palette(GFXVGA_CURSOR_PALETTE_BANK);
    vdp_set_sprite_index(0);
    vdp_write_sprite_x(-32);
    vdp_write_sprite_y(-32);
    vdp_write_sprite_tile(0);
    vdp_write_sprite_attr(0, 0, 0, 0);
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

    build_cursor_sprite(cursor_mask, cursor_data);
    upload_cursor_shape();
}

static void hide_mouse(void)
{
    ensure_cursor_initialized();
    vdp_set_sprite_index(0);
    vdp_write_sprite_x(-32);
    vdp_write_sprite_y(-32);
    vdp_write_sprite_tile(0);
    vdp_write_sprite_attr(0, 0, 0, 0);
}

static void show_mouse(Workstation *wk, long x, long y)
{
    SWORD pos_x;
    SWORD pos_y;
    short screen_width;
    short screen_height;

    ensure_cursor_initialized();

    pos_x = (SWORD)(x & 0xffffL);
    pos_y = (SWORD)y;
    if (wk) {
        pos_x -= wk->mouse.hotspot.x;
        pos_y -= wk->mouse.hotspot.y;
        screen_width = wk->screen.mfdb.width;
        screen_height = wk->screen.mfdb.height;
        pos_x = scale_axis(pos_x, screen_width, GFXVGA_CURSOR_OVERLAY_WIDTH);
        pos_y = scale_axis(pos_y, screen_height, GFXVGA_CURSOR_OVERLAY_HEIGHT);
    }

    vdp_set_sprite_index(0);
    vdp_write_sprite_x(pos_x);
    vdp_write_sprite_y(pos_y);
    vdp_write_sprite_tile(0);
    vdp_write_sprite_attr(0, 0, 0, 0);
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
