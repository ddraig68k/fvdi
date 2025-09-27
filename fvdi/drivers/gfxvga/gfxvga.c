#include "gfxvga.h"

ULONG g_gfxfpga_base = 0x00F7F500;

void drvga_write_control_reg(UWORD data)
{
    DRVGA_REG_WRITE(REG_CONTROL, data);
}

void drvga_wait_busy()
{
	volatile UWORD status;
	do 
    {
        status = DRVGA_REG_READ(REG_STATUS);
    }
    while ((status & STATUS_READY) == 0);
}

void drvga_wait_vblank()
{
	volatile UWORD status;
	do 
    {
        status = DRVGA_REG_READ(REG_STATUS);
    }
    while ((status & 0x8000) == 0);
}

void drvga_wait_vblank_clear()
{
	UWORD status = DRVGA_REG_READ(REG_STATUS);
    while ((status & 0x8000))
    {
	    status = DRVGA_REG_READ(REG_STATUS);
    }
}


void drvga_clear_text()
{
    int count = DRVGA_TEXTBUF_SIZE;
    DRVGA_REG_WRITE(REG_PARAM_DATA0, 0);  // Set the text buffer address

    while (count--)
    {
        DRVGA_REG_WRITE(REG_PARAM_DATA1, ' ');
        DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
    }
}

void drvga_write_text(UWORD posx, UWORD posy, char *text)
{
    UWORD pos = (posy * 80) + posx;

    DRVGA_REG_WRITE(REG_PARAM_DATA0, pos);
    while (*text != 0)
    {
        DRVGA_REG_WRITE(REG_PARAM_DATA1, *text++);
        DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
    }
}

void drvga_write_char(UWORD posx, UWORD posy, char text)
{
    UWORD pos = (posy * 80) + posx;

    DRVGA_REG_WRITE(REG_PARAM_DATA0, pos);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, text);
    DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
}

void drvga_clear_screen(UWORD color)
{
    // Set the screen fill color
	DRVGA_REG_WRITE(REG_PARAM_DATA0, color);
	DRVGA_REG_WRITE(REG_COMMAND, CMD_CLEAR_SCREEN);
    drvga_wait_busy();
}

void drvga_solid_box(UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, x0);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, x1);
    DRVGA_REG_WRITE(REG_PARAM_DATA2, y0);
    DRVGA_REG_WRITE(REG_PARAM_DATA3, y1);
	DRVGA_REG_WRITE(REG_PARAM_DATA4, color);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_FILL_RECT);
    drvga_wait_busy();
}

void drvga_solid_line(UWORD x0, UWORD y0, UWORD x1, UWORD y1, UWORD color)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, x0);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, x1);
    DRVGA_REG_WRITE(REG_PARAM_DATA2, y0);
    DRVGA_REG_WRITE(REG_PARAM_DATA3, y1);
	DRVGA_REG_WRITE(REG_PARAM_DATA4, color);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_DRAW_LINE);
    drvga_wait_busy();
}

void drvga_set_memory_ptr(ULONG addr)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, (UWORD)addr);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, (UWORD)(addr >> 16));
	DRVGA_REG_WRITE(REG_COMMAND, CMD_MEMORY_ACCESS);
}

void drvga_write_memory_block(UWORD *data, ULONG datalen)
{
    while (datalen)
    {
        DRVGA_REG_WRITE(REG_DATA, *data++);
        datalen--;
    }
}


