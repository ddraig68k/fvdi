#ifndef _DDRAIGVDP_H_
#define _DDRAIGVDP_H_

#include <stddef.h>
#include "fvdi.h"

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;

typedef signed char SBYTE;
typedef signed short SWORD;
typedef signed long SLONG;

typedef int BOOL;
typedef void VOID;
typedef ULONG IPTR;

#define TRUE 1
#define FALSE 0

extern ULONG g_vdp_reg_base;
extern UWORD *g_vdp_memory_base;

#define VDP_TEXTBUF_SIZE  4800  /* Max of 80x60*/

#define VDP_REG_WRITE(x, y)     (*((volatile UWORD *) (g_vdp_reg_base + (x))) = (y))
#define VDP_REG_READ(x)         (*((volatile UWORD *) (g_vdp_reg_base + (x))))

#define	REG_STATUS              0x00    // Status register
#define REG_CONTROL             0x02    // Display control register
#define REG_INTERRUPT           0x04    // Interrupt control/ status
#define REG_RAMPAGE             0x06    // SRAM Page control

#define REG_FRAMEBUFFER_L		0x10    // Bitmap data address location (L) 
#define REG_FRAMEBUFFER_H		0x12    // Bitmap data address location (H)
#define REG_TEXT_BASE_L		    0x14    /* Text base address (L) */
#define REG_TEXT_BASE_H		    0x16    /* Text base address (H) */

#define REG_LINESCR_DATA_L		0x18    // Tile scroll table data address location (L) 
#define REG_LINESCR_DATA_H		0x1A    // Tile scroll table tile data address location (H)

#define REG_TEXT_ADDR		    0x20    // Text buffer index
#define REG_TEXT_DATA		    0x22    // Text buffer data
#define REG_CURSOR_POS		    0x24    // Cursor position Y,X
#define REG_CURSOR_SIZE		    0x26    // Cursor size bottom,top

#define REG_PALETTE_IDX         0x28    // Palette index register
#define REG_PALETTE_DATA        0x2A    // Palette data register
#define REG_PALETTE_CTRL0       0x2C    // Palette control register (Text and Bitmap)
#define REG_PALETTE_CTRL1       0x2E    // Palette control register (Tiles and Sprites)

#define	REG_COMMAND             0x30    // Command control register
#define REG_DRAW_BASE_L		    0x32    // Draw base address (L)
#define REG_DRAW_BASE_H		    0x34    // Draw base address (H)
#define	REG_DRAW_COLOR0         0x36    // Draw color 0
#define	REG_DRAW_COLOR1         0x38    // Draw color 1
#define	REG_DRAW_MODE           0x3A    // Draw mode, solid, xor, ...
#define	REG_DRAW_PATTERN        0x3C    // Draw pattern index/data

#define	REG_PARAM_DATA0         0x40    // Command parameter 0
#define	REG_PARAM_DATA1         0x42    // Command parameter 1
#define	REG_PARAM_DATA2         0x44    // Command parameter 2
#define	REG_PARAM_DATA3         0x46    // Command parameter 3
#define	REG_PARAM_DATA4         0x48    // Command parameter 4
#define	REG_PARAM_DATA5         0x4A    // Command parameter 5
#define	REG_PARAM_DATA6         0x4C    // Command parameter 6

#define REG_TILE1_CTRL		    0x50    // Tilemap 0 control register
#define REG_TILE1_DATA_L        0x52    // Tilemap 0 16x16 tile data address register (L)
#define REG_TILE1_DATA_H        0x54    // Tilemap 0 16x16 tile data address register (H)
#define REG_TILE1_MAP_L		    0x56    // Tilemap 0 map data address (L)
#define REG_TILE1_MAP_H		    0x58    // Tilemap 0 map data address (H)
#define REG_TILE1_SCROLL_X      0x5A    // Tilemap 0 horizontal scroll position
#define REG_TILE1_SCROLL_Y	    0x5C    // Tilemap 0 vertical scroll position

#define REG_TILE2_CTRL		    0x60    // Tilemap 1 control register
#define REG_TILE2_DATA_L        0x62    // Tilemap 1 16x16 tile data address register (L)
#define REG_TILE2_DATA_H        0x64    // Tilemap 1 16x16 tile data address register (H)
#define REG_TILE2_MAP_L		    0x66    // Tilemap 1 map data address (L)
#define REG_TILE2_MAP_H		    0x68    // Tilemap 1 map data address (H)
#define REG_TILE2_SCROLL_X      0x6A    // Tilemap 1 horizontal scroll position
#define REG_TILE2_SCROLL_Y	    0x6C    // Tilemap 1 vertical scroll position

#define REG_TILE3_CTRL		    0x70    // Tilemap 2 control register
#define REG_TILE3_DATA_L        0x72    // Tilemap 2 8x8 tile data address register (L)
#define REG_TILE3_DATA_H        0x74    // Tilemap 2 8x8 tile data address register (H)
#define REG_TILE3_MAP_L		    0x76    // Tilemap 2 map data address (L)
#define REG_TILE3_MAP_H		    0x78    // Tilemap 2 map data address (H)
#define REG_TILE3_SCROLL_X      0x7A    // Tilemap 2 horizontal scroll position
#define REG_TILE3_SCROLL_Y	    0x7C    // Tilemap 2 vertical scroll position

#define REG_TILE4_CTRL		    0x80    // Tilemap 3 control register
#define REG_TILE4_DATA_L        0x82    // Tilemap 3 8x8 tile data address register (L)
#define REG_TILE4_DATA_H        0x84    // Tilemap 3 8x8 tile data address register (H)
#define REG_TILE4_MAP_L		    0x86    // Tilemap 3 map data address (L)
#define REG_TILE4_MAP_H		    0x88    // Tilemap 3 map data address (H)
#define REG_TILE4_SCROLL_X      0x8A    // Tilemap 3 horizontal scroll position
#define REG_TILE4_SCROLL_Y	    0x8C    // Tilemap 3 vertical scroll position

#define REG_SPRITE_DATA_L		0x90    // Sprite data address location (L) 
#define REG_SPRITE_DATA_H		0x92    // Sprite data address location (H)
#define REG_SPRITE_IDX          0x94
#define REG_SPRITE_POS_X        0x96
#define REG_SPRITE_POS_Y        0x98
#define REG_SPRITE_TILE         0x9A
#define REG_SPRITE_ATTRIB       0x9C

#define REG_FONT_ADDR		    0xA0    // Pattern address index 
#define REG_FONT_DATA		    0xA2    // Pattern data

#define REG_PATTERN_ADDR		0xA4    // Pattern address index 
#define REG_PATTERN_DATA		0xA6    // Pattern data

#define REG_INT_LINE_Y          0xA8    // Line interrupt control value

#define REG_HW_CURSOR_CTRL      0xB0
#define REG_HW_CURSOR_X         0xB2
#define REG_HW_CURSOR_Y         0xB4
#define REG_HW_CURSOR_ADDR      0xB6
#define REG_HW_CURSOR_DATA      0xB8
#define REG_HW_CURSOR_COL1      0xBA
#define REG_HW_CURSOR_COL2      0xBC
#define REG_HW_CURSOR_COL3      0xBE

#define CMD_NONE                0x00   
#define CMD_FILL_RECT           0x01
#define CMD_DRAW_LINE           0x02
#define CMD_DRAW_VLINE          0x03
#define CMD_DRAW_HLINE          0x04
#define CMD_FILL_TRIANGLE       0x05
#define CMD_MEM_COPY_LINEAR     0x06
#define CMD_MEM_COPY_2D         0x07
#define CMD_MEM_FILL_LINEAR     0x08

#define DRAW_MODE_SOLID        0x0
#define DRAW_MODE_XOR          0x1
#define DRAW_MODE_TRANS        0x2
#define DRAW_MODE_REVTRANS     0x3

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
#define DISPMODE_TILEMAP	    0x0003      // Tile layer (320x240)
// Bitmap mode bits per pixel
#define DISP_DEPTH_RGB 		    0x0000
#define DISP_DEPTH_8BPP			0x0008
#define DISP_DEPTH_4BPP			0x0009
#define DISP_DEPTH_2BPP			0x000A
#define DISP_DEPTH_1BPP			0x000B
// Text control 
#define ENABLE_CURSOR           0x0100
#define ENABLE_BLINK            0x0200
#define TEXT_MODE_60            0x0400
#define SPRITE_ENABLE           0x0800
#define SCANLINE_NONE           0x0000
#define SCANLINE_BLACK          0x1000
#define SCANLINE_GREY           0x2000
#define SCANLINE_COLOR          0x3000
#define TEXT_OVERLAY            0x4000

#define TILEMAP1                0x00
#define TILEMAP2                0x01
#define TILEMAP3                0x02
#define TILEMAP4                0x03

#define TILEMAP1_WIDTH          32
#define TILEMAP1_HEIGHT         32
#define TILEMAP2_WIDTH          32
#define TILEMAP2_HEIGHT         32
#define TILEMAP3_WIDTH          64
#define TILEMAP3_HEIGHT         64
#define TILEMAP4_WIDTH          64
#define TILEMAP4_HEIGHT         64

#define MAX_SPRITES             256

typedef struct tile_layer 
{
    UBYTE id;
    UBYTE enabled;
    UWORD map_width;
    UWORD map_height;
    ULONG map_addr;
    ULONG data_addr;
} tile_layer_t;

void vdp_init(ULONG regaddr, ULONG memaddr);

// Control registers
UWORD vdp_get_status(void);
void vdp_set_control(UWORD mode);
UWORD vdp_get_control(void);
void vdp_set_int_control(UWORD intr);
UWORD vdp_get_int_control(void);
void vdp_set_mempage(UWORD page);
UWORD vdp_get_mempage(void);

void vdp_wait_busy(void);
void vdp_wait_vblank(void);
void vdp_wait_vblank_clear(void);

static inline ULONG vdp_get_regbase_addr(void)
{
    return g_vdp_reg_base;
}

static inline UWORD *vdp_get_memory_base(void)
{
    return g_vdp_memory_base;
}

// Framebuffer pointer
void vdp_set_framebuffer_addr(ULONG addr);
ULONG vdp_get_framebuffer_addr(void);

// Line scroll pointer
void vdp_set_linescroll_addr(ULONG addr);
ULONG vdp_get_linescroll_addr(void);

// Text attributes
#define VDP_TEXT_FG_BLACK           0x0000
#define VDP_TEXT_FG_BLUE            0x0100
#define VDP_TEXT_FG_GREEN           0x0200
#define VDP_TEXT_FG_CYAN            0x0300
#define VDP_TEXT_FG_RED             0x0400
#define VDP_TEXT_FG_MAGENTA         0x0500
#define VDP_TEXT_FG_BROWN           0x0600
#define VDP_TEXT_FG_LIGHTGRAY       0x0700
#define VDP_TEXT_FG_GRAY            0x0800
#define VDP_TEXT_FG_LIGHTBLUE       0x0900
#define VDP_TEXT_FG_LIGHTGREEN      0x0A00
#define VDP_TEXT_FG_LIGHTCYAN       0x0B00
#define VDP_TEXT_FG_LIGHTRED        0x0C00
#define VDP_TEXT_FG_LIGHTMAGENTA    0x0D00
#define VDP_TEXT_FG_YELLOW          0x0E00
#define VDP_TEXT_FG_WHITE           0x0F00

static inline UWORD vdp_make_text_cell(UBYTE ch, UWORD fg, UWORD bg)
{
    return (UWORD)(ch | fg | bg);
}

// Text commands
void vdp_clear_text(void);
void vdp_write_text(UWORD posx, UWORD posy, const char *text);
void vdp_write_char(UWORD posx, UWORD posy, UWORD text);
void vdp_cursor_pos(int x, int y);
void vdp_cursor_size(int top, int bottom);
void vdp_hw_cursor_enable(UWORD enable);
void vdp_hw_cursor_pos(UWORD x, UWORD y);
void vdp_hw_cursor_addr(UWORD addr);
void vdp_hw_cursor_data(UWORD data);
void vdp_hw_cursor_colors(UWORD col1, UWORD col2, UWORD col3);

// Draw command base address
void vdp_set_drawbase_addr(ULONG addr);
ULONG vdp_get_drawbase_addr(void);

// Drawing commands
void vdp_set_drawmode(UWORD mode);
void vdp_set_draw_color(UWORD col);
void vdp_set_back_color(UWORD col);
void vdp_set_pattern(UWORD pat);
void vdp_draw_fill_rect(UWORD x0, UWORD y0, UWORD x1, UWORD y1);
void vdp_draw_line(UWORD x0, UWORD y0, UWORD x1, UWORD y1);
void vdp_draw_hline(UWORD x, UWORD y);
void vdp_draw_vline(UWORD x, UWORD y);
void vdp_draw_fill_tri(UWORD x0, UWORD y0, UWORD x1, UWORD y1,  UWORD x2, UWORD y2);

void vdp_set_bitmap_palette(UWORD bank);
void vdp_set_text_palette(UWORD bank);
void vdp_set_tile_palette(UWORD layer, UWORD bank);
void vdp_set_sprite_palette(UWORD bank);

// Tilemap functions
void vdp_tilemap1_map_addr(ULONG addr);
void vdp_tilemap1_data_addr(ULONG addr);
void vdp_tilemap1_enable(void);
void vdp_tilemap1_disable(void);
void vdp_tilemap1_scroll_x(UWORD x);
void vdp_tilemap1_scroll_y(UWORD y);
void vdp_tilemap1_linescr_mode(UWORD mode);

void vdp_tilemap2_map_addr(ULONG addr);
void vdp_tilemap2_data_addr(ULONG addr);
void vdp_tilemap2_enable(void);
void vdp_tilemap2_disable(void);
void vdp_tilemap2_scroll_x(UWORD x);
void vdp_tilemap2_scroll_y(UWORD y);
void vdp_tilemap2_linescr_mode(UWORD mode);

void vdp_tilemap3_map_addr(ULONG addr);
void vdp_tilemap3_data_addr(ULONG addr);
void vdp_tilemap3_enable(void);
void vdp_tilemap3_disable(void);
void vdp_tilemap3_scroll_x(UWORD x);
void vdp_tilemap3_scroll_y(UWORD y);
void vdp_tilemap3_linescr_mode(UWORD mode);

void vdp_tilemap4_map_addr(ULONG addr);
void vdp_tilemap4_data_addr(ULONG addr);
void vdp_tilemap4_enable(void);
void vdp_tilemap4_disable(void);
void vdp_tilemap4_scroll_x(UWORD x);
void vdp_tilemap4_scroll_y(UWORD y);
void vdp_tilemap4_linescr_mode(UWORD mode);

void vdp_sprite_data_addr(ULONG addr);

void vdp_vram_write(ULONG addr, const UWORD *src, size_t size);
void vdp_init_tile_layer(tile_layer_t *layer);

void vdp_disable_all_sprites(void);

static inline void vdp_set_sprite_index(UBYTE index)
{
    VDP_REG_WRITE(REG_SPRITE_IDX, index);    
}

static inline void vdp_write_sprite_x(SWORD x)
{
    VDP_REG_WRITE(REG_SPRITE_POS_X, x);
}

static inline void vdp_write_sprite_y(SWORD y)
{
    VDP_REG_WRITE(REG_SPRITE_POS_Y, y);
}

static inline void vdp_write_sprite_tile(UWORD tile)
{
    VDP_REG_WRITE(REG_SPRITE_TILE, tile);
}

static inline void vdp_write_sprite_attr(UBYTE palette,
                           UBYTE priority,
                           UBYTE flip_x,
                           UBYTE flip_y)
{
    UWORD attrib = 0x8000; // enable
    attrib |= (palette & 0x0F) | (flip_x << 4) | (flip_y << 5);
    attrib |= (UWORD)(priority & 0xF) << 6; 
    VDP_REG_WRITE(REG_SPRITE_ATTRIB, attrib);
}

void vdp_fill_memory(ULONG addr, UWORD length, UWORD value);
void vdp_copy_memory(ULONG src, ULONG dest, UWORD length);
void vdp_copy_2d(ULONG src_base_word, ULONG dst_base_word,
    UWORD src_x, UWORD src_y, UWORD dst_x, UWORD dst_y,
    UWORD width_pixels, UWORD height_lines, UWORD screen_width_pixels);

#endif
