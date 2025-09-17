#include "fvdi.h"
#include "driver.h"
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
        //printf("VB Status: %04X\n", status);
    }
    while ((status & 0x8000) == 0);
}

void drvga_wait_vblank_clear()
{
	UWORD status = DRVGA_REG_READ(REG_STATUS);
    while ((status & 0x8000))
    {
	    status = DRVGA_REG_READ(REG_STATUS);
        //printf("Status: %04X\n", status);
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

void drvga_set_text_area(UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, x0);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, y0);
    DRVGA_REG_WRITE(REG_PARAM_DATA2, x1);
    DRVGA_REG_WRITE(REG_PARAM_DATA3, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_TEXTAREA);
}

void drvga_clear_screen(UWORD color)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
	DRVGA_REG_WRITE(REG_COMMAND, CMD_CLEAR_SCREEN);
}

void drvga_draw_box(UWORD color, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
    DRVGA_REG_WRITE(REG_PARAM_X0, x0);
    DRVGA_REG_WRITE(REG_PARAM_X1, x1);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y0);
    DRVGA_REG_WRITE(REG_PARAM_Y1, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_FILL_RECT);
}

void drvga_draw_line(UWORD color, UWORD x0, UWORD y0, UWORD x1, UWORD y1)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
    DRVGA_REG_WRITE(REG_PARAM_X0, x0);
    DRVGA_REG_WRITE(REG_PARAM_X1, x1);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y0);
    DRVGA_REG_WRITE(REG_PARAM_Y1, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_DRAW_LINE);
}

void drvga_draw_pixel(UWORD color, UWORD x, UWORD y)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);

    DRVGA_REG_WRITE(REG_PARAM_X0, x);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y);
	DRVGA_REG_WRITE(REG_COMMAND, CMD_DRAW_PIXEL);
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


