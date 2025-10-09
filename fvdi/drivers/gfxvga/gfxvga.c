#include "gfxvga.h"

ULONG g_gfxfpga_base = 0x00F7F500;

void gfx_write_control_reg(UWORD data)
{
    GFX_REG_WRITE(REG_CONTROL, data);
}

void gfx_wait_busy()
{
	volatile UWORD status;
	do 
    {
        status = GFX_REG_READ(REG_STATUS);
    }
    while ((status & STATUS_READY) == 0);
}

void gfx_wait_vblank()
{
	volatile UWORD status;
	do 
    {
        status = GFX_REG_READ(REG_STATUS);
    }
    while ((status & 0x8000) == 0);
}

void gfx_wait_vblank_clear()
{
	UWORD status = GFX_REG_READ(REG_STATUS);
    while ((status & 0x8000))
    {
	    status = GFX_REG_READ(REG_STATUS);
    }
}

void gfx_clear_text()
{
    int count = GFX_TEXTBUF_SIZE;
    GFX_REG_WRITE(REG_PARAM_DATA0, 0);  // Set the text buffer address

    while (count--)
    {
        GFX_REG_WRITE(REG_PARAM_DATA1, ' ');
        GFX_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
    }
}

void gfx_write_text(UWORD posx, UWORD posy, char *text)
{
    UWORD pos = (posy * 80) + posx;

    GFX_REG_WRITE(REG_PARAM_DATA0, pos);
    while (*text != 0)
    {
        GFX_REG_WRITE(REG_PARAM_DATA1, *text++);
        GFX_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
    }
}

void gfx_write_char(UWORD posx, UWORD posy, char text)
{
    UWORD pos = (posy * 80) + posx;

    GFX_REG_WRITE(REG_PARAM_DATA0, pos);
    GFX_REG_WRITE(REG_PARAM_DATA1, text);
    GFX_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
}

void gfx_draw_box(UWORD mode, UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color1, UWORD color2)
{
    gfx_wait_busy();
    GFX_REG_WRITE(REG_PARAM_DATA0, x0);
    GFX_REG_WRITE(REG_PARAM_DATA1, y0);
    GFX_REG_WRITE(REG_PARAM_DATA2, x1);
    GFX_REG_WRITE(REG_PARAM_DATA3, y1);
	GFX_REG_WRITE(REG_PARAM_DATA4, color1);
	GFX_REG_WRITE(REG_PARAM_DATA5, color2);

	GFX_REG_WRITE(REG_COMMAND, mode | CMD_FILL_RECT);
}

void gfx_draw_line(UWORD mode, UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color1, UWORD color2, UWORD pattern)
{
    gfx_wait_busy();
    GFX_REG_WRITE(REG_PARAM_DATA0, x0);
    GFX_REG_WRITE(REG_PARAM_DATA1, x1);
    GFX_REG_WRITE(REG_PARAM_DATA2, y0);
    GFX_REG_WRITE(REG_PARAM_DATA3, y1);
	GFX_REG_WRITE(REG_PARAM_DATA4, color1);
	GFX_REG_WRITE(REG_PARAM_DATA5, color2);
	GFX_REG_WRITE(REG_PARAM_DATA6, pattern);


	GFX_REG_WRITE(REG_COMMAND, mode | CMD_DRAW_LINE);
}

void gfx_set_memory_ptr(ULONG addr)
{
    GFX_REG_WRITE(REG_PARAM_DATA0, (UWORD)addr);
    GFX_REG_WRITE(REG_PARAM_DATA1, (UWORD)(addr >> 16));
	GFX_REG_WRITE(REG_COMMAND, CMD_MEMORY_ACCESS);
}

void gfx_write_memory_block(UWORD *data, ULONG datalen)
{
    while (datalen)
    {
        GFX_REG_WRITE(REG_DATA, *data++);
        datalen--;
    }
}


