/* V9990 library for Kiwi
 * v0.1, August 26 2012
 * Simon Ferber */

#include <stdint.h>

/* v9990 related defines */
#define VDP_BASE	0x3df600
#define VDP_PALETTE_REG	0x02
#define VDP_CMDDAT	0x04
#define VDP_REGDAT	0x06
#define VDP_REGSEL	0x08
#define VDP_STATUS	0x0a
#define VDP_IFP		0x0c
#define VDP_SYSCTRL	0x0e
#define READ_VDP(x)	(*((volatile char *) VDP_BASE + (x) ))
#define WRITE_VDP(x, y)	(*((volatile char *) VDP_BASE + (x) ) = (y) )

#define VDPWriteReg(x, y) ( WRITE_VDP(VDP_REGSEL, x) ); \
                          ( WRITE_VDP(VDP_REGDAT, y) )

#define VDP_SYS_CTRL_SRS	2	// Power on reset state
#define VDP_SYS_CTRL_MCKIN	1	// Select MCKIN terminal
#define VDP_SYS_CTRL_XTAL	0	// Select XTAL

/* V9990 Register defines */
#define VDP_WRITE_ADDR		0	// W
#define VDP_READ_ADDR		3	// W
#define VDP_SCREEN_MODE0	6	// R/W
#define VDP_SCREEN_MODE1	7	// R/W
#define VDP_CTRL		8	// R/W
#define VDP_INT_ENABLE          9       // R/W
#define VDP_INT_V_LINE_LO	10	// R/W	
#define VDP_INT_V_LINE_HI	11	// R/W
#define VDP_INT_H_LINE		12	// R/W	
#define VDP_PALETTE_CTRL	13	// W
#define VDP_PALETTE_PTR		14	// W
#define VDP_BACK_DROP_COLOR 	15      // R/W
#define VDP_DISPLAY_ADJUST	16	// R/W
#define VDP_SCROLL_LOW_Y	17      // R/W
#define VDP_SCROLL_HIGH_Y	18      // R/W
#define VDP_SCROLL_LOW_X	19      // R/W
#define VDP_SCROLL_HIGH_X	20      // R/W
#define VDP_SCROLL_LOW_Y_B	21      // R/W
#define VDP_SCROLL_HIGH_Y_B	22      // R/W
#define VDP_SCROLL_LOW_X_B	23      // R/W
#define VDP_SCROLL_HIGH_X_B	24      // R/W
#define VDP_PAT_GEN_TABLE   	25      // R/W
#define VDP_LCD_CTRL        	26      // R/W
#define VDP_PRIORITY_CTRL  	27      // R/W
#define VDP_SPR_PAL_CTRL	28	// W
#define VDP_SC_X		32	// W
#define VDP_SC_Y		34	// W
#define VDP_DS_X		36	// W
#define VDP_DS_Y		38	// W
#define VDP_NX			40	// W
#define VDP_NY			42	// W
#define VDP_ARG			44	// W
#define VDP_LOP			45	// W
#define VDP_WRITE_MASK		46	// W
#define VDP_FC			48	// W
#define VDP_BC			50	// W
#define VDP_OPCODE		52	// W
#define VDP_BDC_LOW_X		53	// R
#define VDP_BDC_HIGH_X		54	// R

/* Register Select options */
#define VDP_DIS_INC_READ	64
#define VDP_DIS_INC_WRITE	128

/* Bit defines SCREEN_MODE0 (register 6) */
#define VDP_SCR0_STANDBY	192	// Stand by mode
#define VDP_SCR0_BITMAP		128	// Select Bit map mode
#define VDP_SCR0_P2		64	// Select P1 mode
#define VDP_SCR0_P1		0	// Select P1 mode
#define VDP_SCR0_DTCLK		32	// Master Dot clock not divided
#define VDP_SCR0_DTCLK2		16	// Master Dot clock divided by 2
#define VDP_SCR0_DTCLK4		0	// Master Dot clock divided by 4
#define VDP_SCR0_XIM2048	12	// Image size = 2048
#define VDP_SCR0_XIM1024	8	// Image size = 1024
#define VDP_SCR0_XIM512		4	// Image size = 512
#define VDP_SCR0_XIM256		0	// Image size = 256
#define VDP_SCR0_16BIT		3	// 16 bits/dot
#define VDP_SCR0_8BIT		2	// 8 bits/dot
#define VDP_SCR0_4BIT		1	// 4 bits/dot
#define VDP_SCR0_2BIT		0	// 2 bits/dot

/* Bit defines SCREEN_MODE1 (register 7) */
#define VDP_SCR1_C25M		64	// Select 640*480 mode
#define VDP_SCR1_SM1		32	// Selection of 263 lines during non interlace , else 262
#define VDP_SCR1_SM		16	// Selection of horizontal frequency 1H=fsc/227.5
#define VDP_SCR1_PAL		8	// Select PAL, else NTSC
#define VDP_SCR1_EO		4	// Select of vertical resolution of twice the non-interlace resolution
#define VDP_SCR1_IL		2	// Select Interlace
#define VDP_SCR1_HSCN		1	// Select High scan mode

/* Bit defines CTRL    (Register 8) */
#define VDP_CTRL_DISP		128	// Display VRAM
#define VDP_CTRL_DIS_SPD	64	// Disable display sprite (cursor)
#define VDP_CTRL_YSE		32	// /YS Enable
#define VDP_CTRL_VWTE		16	// VRAM Serial data bus control during digitization
#define VDP_CTRL_VWM		8	// VRAM write control during digitization
#define VDP_CTRL_DMAE		4	// Enable DMAQ output
#define VDP_CTRL_VRAM512	2	// VRAM=512KB
#define VDP_CTRL_VRAM256	1	// VRAM=256KB
#define VDP_CTRL_VRAM128	0	// VRAM=128KB

/* Bit defines INT_ENABLE (register 9) */
#define VDP_INT_IECE	        4       // Command end interrupt enable control
#define VDP_INT_IEH	        2       // Display position interrupt enable
#define VDP_INT_IEV	        1       // Int. enable during vertical retrace line interval

/* Bit Defines PALETTE_CTRL  (Register 13) */
#define VDP_PAL_CTRL_YUV	192	// YUV mode
#define VDP_PAL_CTRL_YJK	128	// YJK mode
#define VDP_PAL_CTRL_256	64	// 256 colour mode
#define VDP_PAL_CTRL_PAL	0	// Pallete mode
#define VDP_PAL_CTRL_YAE	32	// Enable YUV/YJK RGB mixing mode

/* Bit defines LOP           (Register 45) */
#define VDP_LOP_TP		16
#define VDP_LOP_WCSC		12
#define VDP_LOP_WCNOTSC		3
#define VDP_LOP_WCANDSC		8
#define VDP_LOP_WCORSC		14
#define VDP_LOP_WCEORSC		6

/* Bit defines ARG */
#define VDP_ARG_MAJ		1
#define VDP_ARG_NEG		2
#define VDP_ARG_DIX		4
#define VDP_ARG_DIY		8

/* Blitter Commands OPCODE    (Register 52) */
#define VDP_OPCODE_STOP		0x00	// Command being excuted is stopped 
#define VDP_OPCODE_LMMC		0x10     // Data is transferred from CPU to VRAM rectangle area
#define VDP_OPCODE_LMMV		0x20     // VRAM rectangle area is painted out
#define VDP_OPCODE_LMCM		0x30     // VRAM rectangle area is transferred to CPU
#define VDP_OPCODE_LMMM		0x40     // Rectangle area os transferred from VRAM to VRAM
#define VDP_OPCODE_CMMC		0x50    // CPU character data is colour-developed and transferred to VRAM rectangle area
#define VDP_OPCODE_CMMK		0x60    // Kanji ROM data is is colour-developed and transferred to VRAM rectangle area
#define VDP_OPCODE_CMMM		0x70    // VRAM character data is colour-developed and transferred to VRAM rectangle area 
#define VDP_OPCODE_BMXL		0x80    // Data on VRAM linear address is transferred to VRAM rectangle area
#define VDP_OPCODE_BMLX		0x90    // VRAM rectangle area is transferred to VRAM linear address 
#define VDP_OPCODE_BMLL		0xA0    // Data on VRAM linear address is transferred to VRAM linear address 
#define VDP_OPCODE_LINE		0xB0    // Straight line is drawer on X-Y co-ordinates
#define VDP_OPCODE_SRCH		0xC0    // Border colour co-ordinates on X-Y are detected
#define VDP_OPCODE_POINT	0xD0    // Colour code on specified point on X-Y is read out
#define VDP_OPCODE_PSET		0xE0    // Drawing is executed at drawing point on X-Y co-ordinates
#define VDP_OPCODE_ADVN		0xF0    // Drawing point on X-Y co-ordinates is shifted

/* Bit defines STATUS */
#define VDP_STATUS_TR           128
#define VDP_STATUS_VR           64
#define VDP_STATUS_HR           32
#define VDP_STATUS_BD           16
#define VDP_STATUS_MSC          4
#define VDP_STATUS_EO           2
#define VDP_STATUS_CE           1

/* Mode select defines for SetScreenMode */
#define VDP_MODE_P1		0	// Pattern mode 0 256 212
#define VDP_MODE_P2		1	// Pattern mode 1 512 212
#define VDP_MODE_B1		2	// Bitmap mode 1 256 212
#define VDP_MODE_B2		3	// Bitmap mode 2 384 240
#define VDP_MODE_B3		4	// Bitmap mode 3 512 212
#define VDP_MODE_B4		5	// Bitmap mode 4 768 240
#define VDP_MODE_B5		6	// Bitmap mode 5 640 400 (VGA)
#define VDP_MODE_B6		7	// Bitmap mode 6 640 480 (VGA)
#define VDP_MODE_B7		8	// Bitmap mode 7 1024 212 (Undocumented v9990 mode)

/* Fixed VRAM addresses */
#define VDP_SCRA_PAT_NAME_TABLE 0x07C000
#define VDP_SCRB_PAT_NAME_TABLE 0x07E000
#define VDP_P1_SPR_ATTRIB_TABLE 0x03FE00
#define VDP_P2_SPR_ATTRIB_TABLE 0x07BE00

#define VDP_CURSOR0_ATTRIB      0x07FE00
#define VDP_CURSOR1_ATTRIB      0x07FE08

#define VDP_CURSOR0_PAT_DATA    0x07FF00
#define VDP_CURSOR1_PAT_DATA    0x07FF80

#define VDP_RED                 32
#define VDP_GREEN               1024
#define VDP_BLUE                1

/* v9990 structures */
typedef struct VDP_POINT
{
        uint16_t x;
        uint16_t y;
} VDP_POINT;

typedef struct VDP_BOX
{
        uint16_t left;
        uint16_t top;
        uint16_t width;
        uint16_t height;
} VDP_BOX;

typedef struct VDP_COLOR
{
        uint8_t red;
        uint8_t green;
        uint8_t blue;
} VDP_COLOR;

typedef struct VDP_COPY_XY_XY
{
        int sourceX;
        int sourceY;
        int destX;
        int destY;
        int width;
        int height;
} VDP_COPY_XY_XY;

typedef struct VDP_COPY_VRAM_XY
{
        int sourceAddress;
        int destX;
        int destY;
        int width;
        int height;
} VDP_COPY_VRAM_XY;

typedef struct VDP_COPY_XY_VRAM
{
        int sourceX;
        int sourceY;
        long destAddress;
        int width;
        int height;
} VDP_COPY_XY_VRAM;

/* Library prototypes */
void v9990_Reset();
void v9990_SetScreenMode (char mode, char bpp, int XWidth, char interlace, char PaletteCntrlReg);
void v9990_SetVRAMWrite (uint32_t address);
void v9990_SetVRAMRead (uint32_t address);
void v9990_DisplayEnable();
void v9990_DisplayDisable();
void v9990_SpritesEnable();
void v9990_SpritesDisable();
void v9990_WritePalette (VDP_COLOR *palette, uint8_t palnum, int count);
void v9990_ReadPalette (VDP_COLOR *palette, uint8_t palnum, int count);
void v9990_SetAdjust (char x, char y);
void v9990_SetBackdropColour (uint8_t colour);
void v9990_SetScrollX (uint16_t x);
void v9990_SetScrollY (uint16_t y);
void v9990_SetScrollXB (uint16_t x);
void v9990_SetScrollYB (uint16_t y);
void v9990_SetScrollMode (uint8_t mode);

void v9990_DrawFilledBox (VDP_BOX *box, uint16_t colour);
void v9990_DrawLine (int x1, int y1, int x2, int y2, int colour);
void v9990_DrawBox (VDP_BOX *box, uint16_t colour);
int16_t v9990_SearchBordercolour (uint16_t x, uint16_t y, uint16_t colour, uint8_t direction);
void v9990_SetPoint (uint16_t x, uint16_t y);
uint8_t v9990_GetColour (uint16_t x, uint16_t y);
void v9990_SetupCopyRamCharToXY (VDP_BOX *box);
void v9990_SetupCopyRamToXY (VDP_BOX *box);
void v9990_CopyRamToXY (uint8_t *buffer, uint32_t count);
void v9990_SetupCopyXYToRam (VDP_BOX *box);
void v9990_CopyXYToRam (uint8_t *buffer, uint32_t count);
inline void v9990_CopyXYToXY (VDP_COPY_XY_XY *xybox);
void v9990_CopyVRamToXY (VDP_BOX *box, uint32_t vram);
void v9990_CopyVRamCharToXY (VDP_BOX *box, uint32_t vram);
void v9990_CopyXYToVram (VDP_COPY_XY_VRAM *vrambox);
/*
short v9990_CopyXYToRegisterXY();
short v9990_CopyXYToVram();
*/
void v9990_SetCmdWriteMask (uint16_t writemask);
void v9990_SetCmdColour (uint16_t colour);
void v9990_SetCmdBackColour (uint16_t colour);
void v9990_CopyRamToVram (uint8_t *buffer, uint16_t size);
void v9990_SetPatternData (uint8_t *buffer, uint16_t patnumber, uint8_t plane);
/*

short v9990_PatternData();
short v9990_SetPattern();
short v9990_GetPattern();
*/

