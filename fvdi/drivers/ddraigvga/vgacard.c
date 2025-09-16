#include "vgacard.h"
#include <stdio.h>

uint32_t g_gfxfpga_base = 0;

void drvga_write_control_reg(uint16_t data)
{
    DRVGA_REG_WRITE(REG_CONTROL, data);
}

void drvga_wait_busy()
{
	volatile uint16_t status;
	do 
    {
        status = DRVGA_REG_READ(REG_STATUS);
    }
    while ((status & STATUS_READY) == 0);
}

void drvga_wait_vblank()
{
	volatile uint16_t status;
	do 
    {
        status = DRVGA_REG_READ(REG_STATUS);
        //printf("VB Status: %04X\n", status);
    }
    while ((status & 0x8000) == 0);
}

void drvga_wait_vblank_clear()
{
	uint16_t status = DRVGA_REG_READ(REG_STATUS);
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

void drvga_write_text(uint16_t posx, uint16_t posy, char *text)
{
    uint16_t pos = (posy * 80) + posx;

    DRVGA_REG_WRITE(REG_PARAM_DATA0, pos);
    while (*text != 0)
    {
        DRVGA_REG_WRITE(REG_PARAM_DATA1, *text++);
        DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
    }
}

void drvga_write_char(uint16_t posx, uint16_t posy, char text)
{
    uint16_t pos = (posy * 80) + posx;

    DRVGA_REG_WRITE(REG_PARAM_DATA0, pos);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, text);
    DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_CHARACTER);
}

void drvga_set_text_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, x0);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, y0);
    DRVGA_REG_WRITE(REG_PARAM_DATA2, x1);
    DRVGA_REG_WRITE(REG_PARAM_DATA3, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_SET_TEXTAREA);
}

void drvga_clear_screen(uint16_t color)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
	DRVGA_REG_WRITE(REG_COMMAND, CMD_CLEAR_SCREEN);
}

void drvga_draw_box(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
    DRVGA_REG_WRITE(REG_PARAM_X0, x0);
    DRVGA_REG_WRITE(REG_PARAM_X1, x1);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y0);
    DRVGA_REG_WRITE(REG_PARAM_Y1, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_FILL_RECT);
}

void drvga_draw_line(uint16_t color, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);
    DRVGA_REG_WRITE(REG_PARAM_X0, x0);
    DRVGA_REG_WRITE(REG_PARAM_X1, x1);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y0);
    DRVGA_REG_WRITE(REG_PARAM_Y1, y1);

	DRVGA_REG_WRITE(REG_COMMAND, CMD_DRAW_LINE);
}

void drvga_draw_pixel(uint16_t color, uint16_t x, uint16_t y)
{
	DRVGA_REG_WRITE(REG_PARAM_COLOR, color);

    DRVGA_REG_WRITE(REG_PARAM_X0, x);
    DRVGA_REG_WRITE(REG_PARAM_Y0, y);
	DRVGA_REG_WRITE(REG_COMMAND, CMD_DRAW_PIXEL);
}

void drvga_set_memory_ptr(uint32_t addr)
{
    DRVGA_REG_WRITE(REG_PARAM_DATA0, (uint16_t)addr);
    DRVGA_REG_WRITE(REG_PARAM_DATA1, (uint16_t)(addr >> 16));
	DRVGA_REG_WRITE(REG_COMMAND, CMD_MEMORY_ACCESS);
}

void drvga_write_memory_block(uint16_t *data, uint32_t datalen)
{
    while (datalen)
    {
        DRVGA_REG_WRITE(REG_DATA, *data++);
        datalen--;
    }
}


