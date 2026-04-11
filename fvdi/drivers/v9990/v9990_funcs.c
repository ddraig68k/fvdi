/* V9990 library for Kiwi
 * v0.1, August 26 2012
 * Simon Ferber */

#include "v9990.h"
//#include <stdio.h>

static uint8_t scrollmode = 0;

/* wait for command finish */
inline void wait_vdp()
{
        //while ((READ_VDP(VDP_STATUS) & VDP_STATUS_TR));
        while ((READ_VDP (VDP_STATUS) & VDP_STATUS_CE))
                ;
}

inline uint8_t VDPReadReg (uint8_t VDP_register)
{
        WRITE_VDP (VDP_REGSEL, VDP_register);
        return READ_VDP (VDP_REGDAT);
}

void v9990_Reset()
{
        uint16_t test;
}

/* setting screen mode
 * mode :
 * bpp :
 * XWidth :
 * interlace :
 * PaletteCntrlReg :
 */
void v9990_SetScreenMode (char mode, char bpp, int XWidth, char interlace, char PaletteCntrlReg)
{
        char MODE_REG6;
        char MODE_REG7;
        char MODE_PORT7;

        switch (mode)
        {
        case VDP_MODE_P1:
                MODE_REG6 = VDP_SCR0_P1 + VDP_SCR0_DTCLK4;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_P2:
                MODE_REG6 = VDP_SCR0_P2 + VDP_SCR0_DTCLK2;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_B1:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK4;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_B2:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK2;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_MCKIN;
                break;

        case VDP_MODE_B3:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK2;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_B4:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_MCKIN;
                break;

        case VDP_MODE_B5:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK;
                MODE_REG7 = VDP_SCR1_HSCN;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_B6:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK;
                MODE_REG7 = VDP_SCR1_HSCN + VDP_SCR1_C25M;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;

        case VDP_MODE_B7:
                MODE_REG6 = VDP_SCR0_BITMAP + VDP_SCR0_DTCLK;
                MODE_REG7 = 0;
                MODE_PORT7 = VDP_SYS_CTRL_XTAL;
                break;
        }

        MODE_REG6 |= bpp + XWidth;
        MODE_REG7 |= interlace;
        wait_vdp();
        VDPWriteReg (VDP_SCREEN_MODE0, MODE_REG6);
        VDPWriteReg (VDP_SCREEN_MODE1, MODE_REG7);
        VDPWriteReg (VDP_PALETTE_CTRL, PaletteCntrlReg);
        WRITE_VDP (VDP_SYSCTRL, MODE_PORT7);
}

/* set VRAM destination address for
 * a following write to VRAM
 */
void v9990_SetVRAMWrite (uint32_t address)
{
        VDPWriteReg (VDP_WRITE_ADDR, (uint8_t)address);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (address >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (address >> 16) & 0x7);
}

/* set VRAM source address for
 * a following read from VRAM
 */
void v9990_SetVRAMRead (uint32_t address)
{
        VDPWriteReg (VDP_READ_ADDR, (uint8_t)address);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (address >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (address >> 16) & 0x7);
}

/* Enable display hardware
 */
void v9990_DisplayEnable()
{
        uint8_t enable;
        enable = VDPReadReg (VDP_CTRL);
        enable |= VDP_CTRL_DISP;
        VDPWriteReg (VDP_CTRL, enable);
}

/* Disable display hardware (black screen)
 */
void v9990_DisplayDisable()
{
        uint8_t enable;
        enable = VDPReadReg (VDP_CTRL);
        enable &= 255 - VDP_CTRL_DISP;
        VDPWriteReg (VDP_CTRL, enable);
}

/* Switch on sprite hardware.
 */
void v9990_SpritesEnable()
{
        uint8_t enable;
        enable = VDPReadReg (VDP_CTRL);
        enable &= 255 - VDP_CTRL_DIS_SPD;
        VDPWriteReg (VDP_CTRL, enable);
}

/* Switch off sprite hardware.
 */
void v9990_SpritesDisable()
{
        uint8_t enable;
        enable = VDPReadReg (VDP_CTRL);
        enable |= VDP_CTRL_DIS_SPD;
        VDPWriteReg (VDP_CTRL, enable);
}

/* Write colour palette into the V9990
 * *palette : pointer to an array of VDP_COLOR structs
 * palnum : number of first colour to be written
 * count : total count of colours to be written
 */
void v9990_WritePalette (VDP_COLOR *palette, uint8_t palnum, int count)
{
        int colours;
        VDPWriteReg (VDP_PALETTE_PTR, palnum);

        for (colours = 0; colours < count; colours++)
        {
                WRITE_VDP (VDP_PALETTE_REG, palette[colours].red);
                WRITE_VDP (VDP_PALETTE_REG, palette[colours].green);
                WRITE_VDP (VDP_PALETTE_REG, palette[colours].blue);
        }
}

void v9990_ReadPalette (VDP_COLOR *palette, uint8_t palnum, int count)
{
        int colours;
        VDPWriteReg (VDP_PALETTE_PTR, palnum);

        for (colours = 0; colours < count; colours++)
        {
                palette[colours].red = READ_VDP (VDP_PALETTE_REG);
                palette[colours].green = READ_VDP (VDP_PALETTE_REG);
                palette[colours].blue = READ_VDP (VDP_PALETTE_REG);
        }
}

void v9990_SetAdjust (char x, char y)
{
        VDPWriteReg (VDP_DISPLAY_ADJUST, ((x & 0x0f) + ((y << 4) & 0xf0)));
}

void v9990_SetBackdropColour (uint8_t colour)
{
        VDPWriteReg (VDP_BACK_DROP_COLOR, (colour & 63));
}

void v9990_SetScrollX (uint16_t x)
{
        VDPWriteReg (VDP_SCROLL_LOW_X, (uint8_t)x & 7);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x >> 3));
}

void v9990_SetScrollY (uint16_t y)
{
        VDPWriteReg (VDP_SCROLL_LOW_Y, (uint8_t)y);
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((y >> 8) & 223) | scrollmode);
}

void v9990_SetScrollXB (uint16_t x)
{
        VDPWriteReg (VDP_SCROLL_LOW_X_B, (uint8_t)x & 7);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x >> 3) & 63);
}

void v9990_SetScrollYB (uint16_t y)
{
        VDPWriteReg (VDP_SCROLL_LOW_Y_B, (uint8_t)y);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y >> 8) & 1);
}

void v9990_SetScrollMode (uint8_t mode)
{
        scrollmode = (mode & 3) << 6;
}

void v9990_DrawFilledBox (VDP_BOX *box, uint16_t colour)
{
        VDPWriteReg (VDP_DS_X, (uint8_t)box->left);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->left >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->top);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height >> 8));
        WRITE_VDP (VDP_REGDAT, 0);
        VDPWriteReg (VDP_FC, (uint8_t)colour);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (colour >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_LMMV);
}

void v9990_DrawLine (int x1, int y1, int x2, int y2, int colour)
{
        int mj, mi;
        uint8_t maj = 0;

        if (x2 < 0)
        {
                maj += 4;
                x2 = -1 * x2;
        }

        if (y2 < 0)
        {
                maj += 8;
                y2 = -1 * y2;
        }

        if (x2 >= y2)
        {
                mj = x2;
                mi = y2;
                maj += 0;
        }
        else
        {
                mj = y2;
                mi = x2;
                maj += 1;
        }

        wait_vdp();
        VDPWriteReg (VDP_DS_X, (uint8_t)x1);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x1 >> 8) & 7);
        WRITE_VDP (VDP_REGDAT, (uint8_t)y1);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y1 >> 8) & 15);
        WRITE_VDP (VDP_REGDAT, (uint8_t)mj);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (mj >> 8) & 15);
        WRITE_VDP (VDP_REGDAT, (uint8_t)mi);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (mi >> 8) & 15);
        WRITE_VDP (VDP_REGDAT, (uint8_t)maj);
        VDPWriteReg (VDP_FC, (uint8_t)colour);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (colour >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_LINE);
}

void v9990_DrawBox (VDP_BOX *box, uint16_t colour)
{
        v9990_DrawLine (box->left + box->width, box->top, 0, box->height, colour); // RIGHT - top right -> bottom right
        v9990_DrawLine (box->left, box->top, box->width, 0, colour); // TOP - top left -> top right
        v9990_DrawLine (box->left, box->top, 0, box->height, colour); // LEFT - top left -> bottom left
        v9990_DrawLine (box->left, (box->top) + (box->height), box->width, 0, colour); // BOTTOM - bottom left -> bottom right
}

int16_t v9990_SearchBordercolour (uint16_t x, uint16_t y, uint16_t colour, uint8_t direction)
{
        uint8_t status;
        uint16_t bdx;
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)x);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y >> 8));
        VDPWriteReg (VDP_ARG, (uint8_t)direction);
        VDPWriteReg (VDP_FC, (uint8_t)colour);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (colour >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_SRCH);
        wait_vdp();

        if (! (READ_VDP (VDP_STATUS) & VDP_STATUS_BD))
        {
                /* Border colour not detected */
                return -1;
        }

        bdx = VDPReadReg (VDP_BDC_LOW_X) + (VDPReadReg (VDP_BDC_HIGH_X) << 8);
        return bdx;
}

void v9990_SetPoint (uint16_t x, uint16_t y)
{
        wait_vdp();
        VDPWriteReg (VDP_DS_X, (uint8_t)x);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_PSET);
}

uint8_t v9990_GetColour (uint16_t x, uint16_t y)
{
        uint8_t colour = 0;
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)x);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (x >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (y >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_POINT);
        colour = READ_VDP (VDP_CMDDAT);
}

void v9990_SetupCopyRamCharToXY (VDP_BOX *box)
{
        wait_vdp();
        VDPWriteReg (VDP_DS_X, (uint8_t)box->left);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->left >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->top);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_CMMC);
}

void v9990_SetupCopyRamToXY (VDP_BOX *box)
{
        wait_vdp();
        VDPWriteReg (VDP_DS_X, (uint8_t) (box->left & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((box->left >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((box->top >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((box->width >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((box->height >> 8) & 255));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_LMMC);
}

void v9990_CopyRamToXY (uint8_t *buffer, uint32_t count)
{
        while (count--)
        {
                WRITE_VDP (VDP_CMDDAT, *buffer++);
        }
}

void v9990_SetupCopyXYToRam (VDP_BOX *box)
{
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)box->left);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->left >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->top);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top >> 8));
        VDPWriteReg (VDP_NX, (uint8_t)box->width);
        //WRITE_VDP(VDP_REGDAT,(uint8_t)box->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_LMCM);
}

void v9990_CopyXYToRam (uint8_t *buffer, uint32_t count)
{
        while (count--)
        {
                *buffer++ = READ_VDP (VDP_CMDDAT);
        }
}

//inline void v9990_CopyXYToXY(VDP_COPY_XY_XY *xybox)
inline void v9990_CopyXYToXY (VDP_COPY_XY_XY *xybox)
{
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)xybox->sourceX);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->sourceX >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->sourceY & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((xybox->sourceY >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->destX & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((xybox->destX >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->destY & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t) ((xybox->destY >> 8) & 255));
        WRITE_VDP (VDP_REGDAT, (uint8_t)xybox->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)xybox->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (xybox->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_LMMM);
}

void v9990_CopyVRamCharToXY (VDP_BOX *box, uint32_t vram)
{
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)vram);
        VDPWriteReg (VDP_SC_Y, (uint8_t) (vram >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vram >> 16) & 7);
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->left);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->left >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->top);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_CMMM);
}

void v9990_CopyVRamToXY (VDP_BOX *box, uint32_t vram)
{
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)vram);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vram >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vram >> 16) & 7);
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->left);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->left >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->top);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->top >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)box->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (box->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_BMXL);
}

void v9990_CopyXYToVram (VDP_COPY_XY_VRAM *vrambox)
{
        wait_vdp();
        VDPWriteReg (VDP_SC_X, (uint8_t)vrambox->sourceX);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->sourceX >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)vrambox->sourceY);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->sourceY >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)vrambox->destAddress);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->destAddress >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->destAddress >> 16));
        WRITE_VDP (VDP_REGDAT, (uint8_t)vrambox->width);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->width >> 8));
        WRITE_VDP (VDP_REGDAT, (uint8_t)vrambox->height);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (vrambox->height >> 8));
        VDPWriteReg (VDP_OPCODE, VDP_OPCODE_BMLX);
}

void v9990_CopyXYToRegisterXY()
{
}

void v9990_SetCmdWriteMask (uint16_t writemask)
{
        wait_vdp();
        uint8_t masklo, maskhi;
        masklo = (uint8_t) (writemask & 255);
        maskhi = (uint8_t) ((writemask >> 8) & 255);
        WRITE_VDP (VDP_REGSEL, VDP_WRITE_MASK);
        WRITE_VDP (VDP_REGDAT, masklo);
        WRITE_VDP (VDP_REGDAT, maskhi);
}

void v9990_SetCmdColour (uint16_t colour)
{
        VDPWriteReg (VDP_FC, (uint8_t)colour);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (colour >> 8));
}

void v9990_SetCmdBackColour (uint16_t colour)
{
        VDPWriteReg (VDP_BC, (uint8_t)colour);
        WRITE_VDP (VDP_REGDAT, (uint8_t) (colour >> 8));
}

void v9990_CopyRamToVram (uint8_t *buffer, uint16_t size)
{
        while (size--)
        {
                //WRITE_VDP(VDP_BASE,*buffer++);
                asm volatile ("move.b   %0, 0x3df600"::"m" (* (buffer++)));
        }
}

void v9990_SetPatternData (uint8_t *buffer, uint16_t patnumber, uint8_t plane)
{
        VDP_BOX box;

        if (!plane)
                // plane A
        {
                box.left = patnumber % 256;
                box.top = patnumber / 256;
        }
        else
                // plane B
        {
                box.left = 0;
                box.top = 0;
        }

        box.width = 8; // pattern width 8 pixel
        box.height = 8; // pattern height 8 pixel
        v9990_SetupCopyRamToXY (&box);
        v9990_CopyRamToXY (buffer, 8 * 4); // pattern mode = 4bpp (2 pixel per byte), 8x8 pixel need 8x(8/2) byte
}

/*

short v9990_PatternData();
short v9990_SetPattern();
short v9990_GetPattern();
*/

