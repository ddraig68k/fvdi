#ifndef DDRAIG_VGA_H
#define DDRAIG_VGA_H

typedef unsigned char UBYTE;
typedef unsigned short UWORD;
typedef unsigned long ULONG;
typedef int BOOL;
typedef void VOID;
typedef ULONG IPTR;

#define TRUE 1
#define FALSE 0

long CDECL c_write_pixel(Virtual *vwk, MFDB *mfdb, long x, long y, long colour);
long CDECL c_read_pixel(Virtual *vwk, MFDB *mfdb, long x, long y);

long CDECL c_get_colour(Virtual *vwk, long colour);
void CDECL c_set_colours(Virtual *vwk, long start, long entries, unsigned short *requested, Colour palette[]);


#endif
