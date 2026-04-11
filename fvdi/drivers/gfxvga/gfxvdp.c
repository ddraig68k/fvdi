#include <stdbool.h>
#include "gfxvdp.h"

ULONG g_vdp_reg_base = 0x00F7F500;
UWORD *g_vdp_memory_base = 0x00A00000;

#define TILE_BANK_SHIFT(n)   ((n) * 3)   /* 4 tile layers, 3 bits each */
#define TILE_BANK_MASK(n)    (0x07 << TILE_BANK_SHIFT(n))

#define SPRITE_BANK_SHIFT    12          /* bits 13:12 */
#define SPRITE_BANK_MASK     (0x03 << SPRITE_BANK_SHIFT)

void vdp_init(ULONG regaddr, ULONG memaddr)
{
    // Disable all tile layers and spites,
    // Reset pointers to 0
    
    g_vdp_reg_base = regaddr;
    g_vdp_memory_base = (UWORD *)memaddr;

    vdp_set_bitmap_palette(0);
    vdp_set_framebuffer_addr(0);

    vdp_disable_all_sprites();
    vdp_set_sprite_palette(0);
    vdp_sprite_data_addr(0);    

    vdp_set_tile_palette(0, 0);
    vdp_set_tile_palette(1, 0);
    vdp_set_tile_palette(2, 0);
    vdp_set_tile_palette(3, 0);

    vdp_tilemap1_disable();
    vdp_tilemap1_map_addr(0);
    vdp_tilemap1_data_addr(0);
    vdp_tilemap1_scroll_x(0);
    vdp_tilemap1_scroll_y(0);
    vdp_tilemap1_linescr_mode(0);

    vdp_tilemap2_disable();
    vdp_tilemap2_map_addr(0);
    vdp_tilemap2_data_addr(0);
    vdp_tilemap2_scroll_x(0);
    vdp_tilemap2_scroll_y(0);
    vdp_tilemap2_linescr_mode(0);

    vdp_tilemap3_disable();
    vdp_tilemap3_map_addr(0);
    vdp_tilemap3_data_addr(0);
    vdp_tilemap3_scroll_x(0);
    vdp_tilemap3_scroll_y(0);
    vdp_tilemap3_linescr_mode(0);

    vdp_tilemap4_disable();
    vdp_tilemap4_map_addr(0);
    vdp_tilemap4_data_addr(0);
    vdp_tilemap4_scroll_x(0);
    vdp_tilemap4_scroll_y(0);
    vdp_tilemap4_linescr_mode(0);

}

UWORD vdp_get_status(void)
{
    return VDP_REG_READ(REG_STATUS);
}

void vdp_set_control(UWORD mode)
{
    VDP_REG_WRITE(REG_CONTROL, mode);
}

UWORD vdp_get_control(void)
{
    return VDP_REG_READ(REG_CONTROL);
}

void vdp_set_int_control(UWORD intr)
{
    VDP_REG_WRITE(REG_INTERRUPT, intr);
}

UWORD vdp_get_int_control(void)
{
    return VDP_REG_READ(REG_INTERRUPT);
}

void vdp_set_mempage(UWORD page)
{
    VDP_REG_WRITE(REG_RAMPAGE, page);
}

UWORD vdp_get_mempage(void)
{
    return VDP_REG_READ(REG_RAMPAGE);
}

void vdp_wait_busy(void)
{
    volatile UWORD status;
	do 
    {
        status = VDP_REG_READ(REG_STATUS);
    }
    while ((status & STATUS_READY) == 0);
}

void vdp_wait_vblank(void)
{
	volatile UWORD status;
	do 
    {
        status = VDP_REG_READ(REG_STATUS);
    }
    while ((status & 0x8000) == 0);
}

void vdp_wait_vblank_clear(void)
{
	UWORD status = VDP_REG_READ(REG_STATUS);
    while ((status & 0x8000))
    {
	    status = VDP_REG_READ(REG_STATUS);
    }
}

void vdp_clear_text(void)
{
    int count = VDP_TEXTBUF_SIZE;
    VDP_REG_WRITE(REG_TEXT_ADDR, 0);  // Set the text buffer address

    while (count--)
    {
        VDP_REG_WRITE(REG_TEXT_DATA, vdp_make_text_cell(' ', VDP_TEXT_FG_WHITE, 0));
    }
}

void vdp_write_text(UWORD posx, UWORD posy, const char *text)
{
    UWORD pos = (posy * 80) + posx;

    VDP_REG_WRITE(REG_TEXT_ADDR, pos);
    while (*text != 0)
    {
        VDP_REG_WRITE(REG_TEXT_DATA, vdp_make_text_cell((UBYTE)*text++, VDP_TEXT_FG_WHITE, 0));
    }
}

void vdp_write_char(UWORD posx, UWORD posy, UWORD text)
{
    UWORD pos = (posy * 80) + posx;

    VDP_REG_WRITE(REG_TEXT_ADDR, pos);
    VDP_REG_WRITE(REG_TEXT_DATA, text);
}

void vdp_cursor_pos(int x, int y)
{
    UWORD pos = (y & 0x00FF);
    pos = (pos << 8) | (x & 0x00FF);

    VDP_REG_WRITE(REG_CURSOR_POS, pos);
}

void vdp_cursor_size(int top, int bottom)
{
    UWORD pos = (bottom & 0x00FF);
    pos = (pos << 8) | (top & 0x00FF);

    VDP_REG_WRITE(REG_CURSOR_SIZE, pos);
}

void vdp_set_framebuffer_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_FRAMEBUFFER_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_FRAMEBUFFER_H, (addr >> 16));
}

ULONG vdp_get_framebuffer_addr(void)
{
    ULONG addr;
    addr = VDP_REG_READ(REG_FRAMEBUFFER_H);
    addr = (addr << 16) | VDP_REG_READ(REG_FRAMEBUFFER_L);
    return addr << 1;
}

void vdp_set_linescroll_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_LINESCR_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_LINESCR_DATA_H, (addr >> 16));
}

ULONG vdp_get_linescroll_addr(void)
{
    ULONG addr;
    addr = VDP_REG_READ(REG_LINESCR_DATA_H);
    addr = (addr << 16) | VDP_REG_READ(REG_LINESCR_DATA_L);
    return addr << 1;
}

void vdp_set_drawbase_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_DRAW_BASE_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_DRAW_BASE_H, (addr >> 16));
}

ULONG vdp_get_drawbase_addr(void)
{
    ULONG addr;
    addr = VDP_REG_READ(REG_DRAW_BASE_H);
    addr = (addr << 16) | VDP_REG_READ(REG_DRAW_BASE_L);
    return addr << 1;
}

void vdp_set_drawmode(UWORD mode)
{
    
    VDP_REG_WRITE(REG_DRAW_MODE, mode);
}

void vdp_set_draw_color(UWORD col)
{
    VDP_REG_WRITE(REG_DRAW_COLOR0, col);
}

void vdp_set_back_color(UWORD col)
{
    VDP_REG_WRITE(REG_DRAW_COLOR1, col);
}

void vdp_set_pattern(UWORD pat)
{
    VDP_REG_WRITE(REG_DRAW_PATTERN, pat);
}

void vdp_draw_fill_rect(UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, x0);
    VDP_REG_WRITE(REG_PARAM_DATA1, y0);
    VDP_REG_WRITE(REG_PARAM_DATA2, x1);
    VDP_REG_WRITE(REG_PARAM_DATA3, y1);
	vdp_wait_busy();
 	VDP_REG_WRITE(REG_COMMAND, CMD_FILL_RECT);
}

void vdp_draw_fill_tri(UWORD x0, UWORD y0, UWORD x1, UWORD y1,  UWORD x2, UWORD y2)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, x0);
    VDP_REG_WRITE(REG_PARAM_DATA1, y0);
    VDP_REG_WRITE(REG_PARAM_DATA2, x1);
    VDP_REG_WRITE(REG_PARAM_DATA3, y1);
    VDP_REG_WRITE(REG_PARAM_DATA4, x2);
    VDP_REG_WRITE(REG_PARAM_DATA5, y2);
	vdp_wait_busy();
 	VDP_REG_WRITE(REG_COMMAND, CMD_FILL_TRIANGLE);
}

void vdp_draw_line(UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, x0);
    VDP_REG_WRITE(REG_PARAM_DATA1, y0);
    VDP_REG_WRITE(REG_PARAM_DATA2, x1);
    VDP_REG_WRITE(REG_PARAM_DATA3, y1);
	vdp_wait_busy();
	VDP_REG_WRITE(REG_COMMAND, CMD_DRAW_LINE);
}

void vdp_draw_hline(UWORD x, UWORD y)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, x);
    VDP_REG_WRITE(REG_PARAM_DATA1, y);
	vdp_wait_busy();
	VDP_REG_WRITE(REG_COMMAND, CMD_DRAW_HLINE);
}

void vdp_draw_vline(UWORD x, UWORD y)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, x);
    VDP_REG_WRITE(REG_PARAM_DATA1, y);
	vdp_wait_busy();
	VDP_REG_WRITE(REG_COMMAND, CMD_DRAW_VLINE);
}

void vdp_set_bitmap_palette(UWORD bank)
{
    UWORD palctl = VDP_REG_READ(REG_PALETTE_CTRL0);
    palctl = (palctl & ~(0x03)) | (bank & 0x03);
    VDP_REG_WRITE(REG_PALETTE_CTRL0, palctl);
}

void vdp_set_text_palette(UWORD bank)
{
    UWORD palctl = VDP_REG_READ(REG_PALETTE_CTRL0);
    palctl = (palctl & ~(0x1C)) | ((bank & 0x07) << 2);
    VDP_REG_WRITE(REG_PALETTE_CTRL0, palctl);
}

void vdp_set_tile_palette(UWORD layer, UWORD bank)
{
    UWORD palctl = VDP_REG_READ(REG_PALETTE_CTRL1);

    layer &= 0x03; /* 0–3 */
    bank  &= 0x07; /* 3-bit bank */

    palctl = (palctl & ~TILE_BANK_MASK(layer))
           | (bank << TILE_BANK_SHIFT(layer));

    VDP_REG_WRITE(REG_PALETTE_CTRL1, palctl);
}

void vdp_set_sprite_palette(UWORD bank)
{
    UWORD palctl = VDP_REG_READ(REG_PALETTE_CTRL1);

    bank &= 0x03;  /* 2-bit bank */

    palctl = (palctl & ~SPRITE_BANK_MASK)
           | (bank << SPRITE_BANK_SHIFT);

    VDP_REG_WRITE(REG_PALETTE_CTRL1, palctl);
}

void vdp_tilemap1_map_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE1_MAP_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE1_MAP_H, (addr >> 16));

}

void vdp_tilemap1_data_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE1_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE1_DATA_H, (addr >> 16));
}

void vdp_tilemap1_enable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE1_CTRL);
    ctrl |= 1;
    VDP_REG_WRITE(REG_TILE1_CTRL, ctrl);
}

void vdp_tilemap1_disable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE1_CTRL);
    ctrl &= 0xFFFE;
    VDP_REG_WRITE(REG_TILE1_CTRL, ctrl);
}

void vdp_tilemap1_scroll_x(UWORD x)
{
    VDP_REG_WRITE(REG_TILE1_SCROLL_X, x);
}

void vdp_tilemap1_scroll_y(UWORD y)
{
    VDP_REG_WRITE(REG_TILE1_SCROLL_Y, y);
}

void vdp_tilemap1_linescr_mode(UWORD mode)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE1_CTRL);
    ctrl = (ctrl & 0x01) | ((mode & 0x03) << 1);
    VDP_REG_WRITE(REG_TILE1_CTRL, ctrl);
}

void vdp_tilemap2_map_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE2_MAP_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE2_MAP_H, (addr >> 16));

}

void vdp_tilemap2_data_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE2_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE2_DATA_H, (addr >> 16));
}

void vdp_tilemap2_enable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE2_CTRL);
    ctrl |= 1;
    VDP_REG_WRITE(REG_TILE2_CTRL, ctrl);
}

void vdp_tilemap2_disable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE2_CTRL);
    ctrl &= 0xFFFE;
    VDP_REG_WRITE(REG_TILE2_CTRL, ctrl);
}

void vdp_tilemap2_scroll_x(UWORD x)
{
    VDP_REG_WRITE(REG_TILE2_SCROLL_X, x);
}

void vdp_tilemap2_scroll_y(UWORD y)
{
    VDP_REG_WRITE(REG_TILE2_SCROLL_Y, y);
}

void vdp_tilemap2_linescr_mode(UWORD mode)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE2_CTRL);
    ctrl = (ctrl & 0x01) | ((mode & 0x03) << 1);
    VDP_REG_WRITE(REG_TILE2_CTRL, ctrl);
}

void vdp_tilemap3_map_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE3_MAP_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE3_MAP_H, (addr >> 16));

}

void vdp_tilemap3_data_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE3_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE3_DATA_H, (addr >> 16));
}

void vdp_tilemap3_enable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE3_CTRL);
    ctrl |= 1;
    VDP_REG_WRITE(REG_TILE3_CTRL, ctrl);
}

void vdp_tilemap3_disable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE3_CTRL);
    ctrl &= 0xFFFE;
    VDP_REG_WRITE(REG_TILE3_CTRL, ctrl);
}

void vdp_tilemap3_scroll_x(UWORD x)
{
    VDP_REG_WRITE(REG_TILE3_SCROLL_X, x);
}

void vdp_tilemap3_scroll_y(UWORD y)
{
    VDP_REG_WRITE(REG_TILE3_SCROLL_Y, y);
}

void vdp_tilemap3_linescr_mode(UWORD mode)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE3_CTRL);
    ctrl = (ctrl & 0x01) | ((mode & 0x03) << 1);
    VDP_REG_WRITE(REG_TILE3_CTRL, ctrl);
}

void vdp_tilemap4_map_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE4_MAP_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE4_MAP_H, (addr >> 16));

}

void vdp_tilemap4_data_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_TILE4_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_TILE4_DATA_H, (addr >> 16));
}

void vdp_tilemap4_enable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE4_CTRL);
    ctrl |= 1;
    VDP_REG_WRITE(REG_TILE4_CTRL, ctrl);
}

void vdp_tilemap4_disable(void)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE4_CTRL);
    ctrl &= 0xFFFE;
    VDP_REG_WRITE(REG_TILE4_CTRL, ctrl);
}

void vdp_tilemap4_scroll_x(UWORD x)
{
    VDP_REG_WRITE(REG_TILE4_SCROLL_X, x);
}

void vdp_tilemap4_scroll_y(UWORD y)
{
    VDP_REG_WRITE(REG_TILE4_SCROLL_Y, y);
}

void vdp_tilemap4_linescr_mode(UWORD mode)
{
    UWORD ctrl = VDP_REG_READ(REG_TILE4_CTRL);
    ctrl = (ctrl & 0x01) | ((mode & 0x03) << 1);
    VDP_REG_WRITE(REG_TILE4_CTRL, ctrl);
}

void vdp_sprite_data_addr(ULONG addr)
{
    // Convert to word based address
    addr >>= 1;   
    VDP_REG_WRITE(REG_SPRITE_DATA_L, (addr & 0xFFFF));
    VDP_REG_WRITE(REG_SPRITE_DATA_H, (addr >> 16));
}


void vdp_vram_write(ULONG addr, const UWORD *src, ULONG count)
{
    size_t words = count >> 1;
    UWORD saved_page = vdp_get_mempage();

    /* Convert to word address once */
    ULONG vram_word = addr >> 1;

    while (words > 0)
    {
        /* 1MB window = 512K words */
        ULONG page = (vram_word >> 19) & 1;
        vdp_set_mempage(page);

        /* Offset within page in words */
        ULONG page_offset = vram_word & 0x7FFFF;

        /* How many words fit before page crossing */
        size_t chunk_words = 0x80000 - page_offset;
        if (chunk_words > words)
            chunk_words = words;

        UWORD *dst = (UWORD *)g_vdp_memory_base + page_offset;

        size_t i;
        for (i = 0; i < chunk_words; i++)
            dst[i] = src[i];

        src        += chunk_words;
        vram_word  += chunk_words;
        words      -= chunk_words;
    }

    vdp_set_mempage(saved_page);
}

void vdp_init_tile_layer(tile_layer_t *layer)
{
    switch (layer->id)
    {
        case TILEMAP1:
            vdp_tilemap1_disable();
            vdp_tilemap1_map_addr(layer->map_addr);
            vdp_tilemap1_data_addr(layer->data_addr);
            vdp_tilemap1_scroll_x(0);
            vdp_tilemap1_scroll_y(0);
            vdp_tilemap1_linescr_mode(0);
            if (layer->enabled)
                vdp_tilemap1_enable();
            break;

        case TILEMAP2:
            vdp_tilemap2_disable();
            vdp_tilemap2_map_addr(layer->map_addr);
            vdp_tilemap2_data_addr(layer->data_addr);
            vdp_tilemap2_scroll_x(0);
            vdp_tilemap2_scroll_y(0);
            vdp_tilemap2_linescr_mode(0);
            if (layer->enabled)
                vdp_tilemap2_enable();
            break;

        case TILEMAP3:
            vdp_tilemap3_disable();
            vdp_tilemap3_map_addr(layer->map_addr);
            vdp_tilemap3_data_addr(layer->data_addr);
            vdp_tilemap3_scroll_x(0);
            vdp_tilemap3_scroll_y(0);
            vdp_tilemap3_linescr_mode(0);
            if (layer->enabled)
                vdp_tilemap3_enable();
            break;

        case TILEMAP4:
            vdp_tilemap4_disable();
            vdp_tilemap4_map_addr(layer->map_addr);
            vdp_tilemap4_data_addr(layer->data_addr);
            vdp_tilemap4_scroll_x(0);
            vdp_tilemap4_scroll_y(0);
            vdp_tilemap4_linescr_mode(0);
            if (layer->enabled)
                vdp_tilemap4_enable();
            break;
    }
}

void vdp_disable_all_sprites(void)
{
    UWORD i;
    for (i = 0; i < 256; i++)
    {
        VDP_REG_WRITE(REG_SPRITE_IDX, i);
        VDP_REG_WRITE(REG_SPRITE_ATTRIB, 0);
    }
}

void vdp_fill_memory(ULONG addr, UWORD length, UWORD value)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, (addr >> 16));
    VDP_REG_WRITE(REG_PARAM_DATA1, (UWORD)addr);
    VDP_REG_WRITE(REG_PARAM_DATA2, length);
    VDP_REG_WRITE(REG_PARAM_DATA3, value);
    vdp_wait_busy();
    VDP_REG_WRITE(REG_COMMAND, CMD_MEM_FILL_LINEAR);
}

void vdp_copy_memory(ULONG src, ULONG dest, UWORD length)
{
    VDP_REG_WRITE(REG_PARAM_DATA0, (src >> 16));
    VDP_REG_WRITE(REG_PARAM_DATA1, (UWORD)src);
    VDP_REG_WRITE(REG_PARAM_DATA2, (dest >> 16));
    VDP_REG_WRITE(REG_PARAM_DATA3, (UWORD)dest);
    VDP_REG_WRITE(REG_PARAM_DATA4, length);
    vdp_wait_busy();
    VDP_REG_WRITE(REG_COMMAND, CMD_MEM_COPY_LINEAR);
}


void vdp_copy_2d(
    ULONG src_base_word,
    ULONG dst_base_word,
    UWORD src_x,
    UWORD src_y,
    UWORD dst_x,
    UWORD dst_y,
    UWORD width_pixels,
    UWORD height_lines,
    UWORD screen_width_pixels)
{
    ULONG pitch_words = (ULONG)screen_width_pixels; /* 16bpp: 1 pixel == 1 word */
    UWORD width_words = width_pixels;

    /* Sanity checks: zero width/height would be nonsensical */
    if (!width_pixels || !height_lines) {
        /* Cannot use DPRINTF here as kprintf_func_t not available in this unit.
         * Skip silent check instead - let hardware detect error if it occurs. */
        return;
    }

    ULONG src_addr = src_base_word
                      + (ULONG)src_y * pitch_words
                      + (ULONG)src_x;

    ULONG dst_addr = dst_base_word
                      + (ULONG)dst_y * pitch_words
                      + (ULONG)dst_x;

    vdp_wait_busy();

    VDP_REG_WRITE(REG_PARAM_DATA0, (src_addr >> 16));
    VDP_REG_WRITE(REG_PARAM_DATA1, (UWORD)src_addr);
    VDP_REG_WRITE(REG_PARAM_DATA2, (dst_addr >> 16));
    VDP_REG_WRITE(REG_PARAM_DATA3, (UWORD)dst_addr);
    VDP_REG_WRITE(REG_PARAM_DATA4, width_words);
    VDP_REG_WRITE(REG_PARAM_DATA5, height_lines);
    VDP_REG_WRITE(REG_PARAM_DATA6, pitch_words);

    VDP_REG_WRITE(REG_COMMAND, CMD_MEM_COPY_2D);
}
