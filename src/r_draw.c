//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	The actual span/column drawing functions.
//	Here find the main potential for optimization,
//	 e.g. inline assembly, different algorithms.
//




#include "doomdef.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

// Needs access to LFB (guess what).
#include "v_video.h"

// State.
#include "doomstat.h"


// ?
#define MAXWIDTH			1120
#define MAXHEIGHT			832

// status bar height at bottom of screen
#define SBARHEIGHT		32

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//  not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//  and we need only the base address,
//  and the total size == width*height*depth/8.,
//


byte*		viewimage;
int		viewwidth;
int		scaledviewwidth;
int		viewheight;
int		viewwindowx;
int		viewwindowy;
pixel_t*		ylookup[MAXHEIGHT];
int		columnofs[MAXWIDTH];

// Color tables for different players,
//  translate a limited part to another
//  (color ramps used for  suit colors).
//
byte		translations[3][256];


//
// R_DrawColumn
// Source is the top of the column to scale.
//
lighttable_t*		dc_colormap;
int			dc_x;
int			dc_yl;
int			dc_yh;
fixed_t			dc_iscale;
fixed_t			dc_texturemid;

// first pixel in a column (possibly virtual)
byte*			dc_source;		

// just for profiling
int			dccount;

//
// A column is a vertical slice/span from a wall texture that,
//  given the DOOM style restrictions on the view orientation,
//  will always have constant z depth.
// Thus a special case loop for very fast rendering can
//  be used. It has also been used with Wolfenstein 3D.
//
void R_DrawColumn (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;	

    int xoff, yoff;
    uint8_t xmask;

    count = dc_yh - dc_yl;

    // Zero length, column does not exceed a pixel.
    if (count < 0)
	return;
				
#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
	I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
#endif

    // Framebuffer destination address.
    // Use ylookup LUT to avoid multiply with ScreenWidth.
    // Use columnofs LUT for subwindows?
    yoff = dc_yl;
    xoff = columnofs[dc_x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0x80 >> (xoff & 7);

    // Determine scaling,
    //  which is the only mapping to be done.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Inner loop that does the actual texture mapping,
    //  e.g. a DDA-lile scaling.
    // This is as fast as it gets.
    do
    {
	// Re-map color indices from wall texture column
	//  using a lighting/special effects LUT.
    if (shades[graymap[dc_colormap[dc_source[(frac>>FRACBITS)&127]]]][yoff & 3] & xmask) {
        *dest |= xmask;
    } else {
        *dest &= ~xmask;
    }
	
	dest += SCREENSTRIDE;
	frac += fracstep;
    ++yoff;
	
    } while (count--);
}

void R_DrawColumnLow (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;	
    int                 x;

    int xoff, yoff;
    uint8_t xmask;

    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
	return;
				
#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	
	I_Error ("R_DrawColumn: %i to %i at %i", dc_yl, dc_yh, dc_x);
    }
    //	dccount++;
#endif
    // Blocky mode, need to multiply by 2.
    x = dc_x << 1;

    yoff = dc_yl;
    xoff = columnofs[x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0xc0 >> (xoff & 7);

    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    do
    {
    *dest &= ~xmask;
    *dest |= (shades[graymap[dc_colormap[dc_source[(frac>>FRACBITS)&127]]]][yoff & 3] & xmask);
	dest += SCREENSTRIDE;
	frac += fracstep;
    ++yoff;

    } while (count--);
}


//
// Spectre/Invisibility.
//
#define FUZZTABLE		50


static uint8_t fuzzoffset[FUZZTABLE] = {
    0, 1, 0, 0, 2, 0, 1, 1, 0, 2,
    1, 0, 1, 1, 2, 0, 2, 1, 0, 1,
    1, 1, 0, 0, 1, 0, 0, 1, 0, 0,
    0, 1, 0, 0, 1, 1, 0, 0, 2, 2,
    2, 0, 0, 1, 2, 1, 1, 2, 2, 0,
};

static int fuzzpos = 0;


//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//  from adjacent ones to left and right.
// Used with an all black colormap, this
//  could create the SHADOW effect,
//  i.e. spectres and invisible players.
//
void R_DrawFuzzColumn (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;	

    int xoff, yoff;
    uint8_t xmask;

    // Adjust borders. Low...
    if (!dc_yl)
	dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1)
	dc_yh = viewheight - 2;
		
    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
	return;

#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);
    }
#endif

    yoff = dc_yl;
    xoff = columnofs[dc_x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0x80 >> (xoff & 7);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    int offset = fuzzoffset[fuzzpos];

    // instead of dither approach from vanilla, we take a different approach.
    // we'll apply black pixels every 3 pixels using fuzz offset.
    do
    {
    if (offset == 0) {
        *dest &= ~xmask;
    }
    if (++offset == 3)
        offset = 0;

    dest += SCREENSTRIDE;

    frac += fracstep;
    ++yoff;
    } while (count--);

	// Clamp table lookup index.
	if (++fuzzpos == FUZZTABLE)
	    fuzzpos = 0;
}

// low detail mode version

void R_DrawFuzzColumnLow (void)
{
    int			count;
    pixel_t*		dest;
    pixel_t*		dest2;
    fixed_t		frac;
    fixed_t		fracstep;	
    int x;

    int xoff, yoff;
    uint8_t xmask;

    // Adjust borders. Low...
    if (!dc_yl)
	dc_yl = 1;

    // .. and high.
    if (dc_yh == viewheight-1)
	dc_yh = viewheight - 2;
		
    count = dc_yh - dc_yl;

    // Zero length.
    if (count < 0)
	return;

    // low detail mode, need to multiply by 2

    x = dc_x << 1;

#ifdef RANGECHECK
    if ((unsigned)x >= SCREENWIDTH
	|| dc_yl < 0 || dc_yh >= SCREENHEIGHT)
    {
	I_Error ("R_DrawFuzzColumn: %i to %i at %i",
		 dc_yl, dc_yh, dc_x);
    }
#endif

    yoff = dc_yl;
    xoff = columnofs[x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0x80 >> (xoff & 7);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    int offset = fuzzoffset[fuzzpos];
    do
    {
    if (offset == 0) {
        *dest &= ~xmask;
    }
    if (++offset == 3)
        offset = 0;

    dest += SCREENSTRIDE;

    frac += fracstep;
    ++yoff;
    } while (count--);

	// Clamp table lookup index.
	if (++fuzzpos == FUZZTABLE)
	    fuzzpos = 0;
}





//
// R_DrawTranslatedColumn
// Used to draw player sprites
//  with the green colorramp mapped to others.
// Could be used with different translation
//  tables, e.g. the lighter colored version
//  of the BaronOfHell, the HellKnight, uses
//  identical sprites, kinda brightened up.
//
byte*	dc_translation;
byte*	translationtables;

void R_DrawTranslatedColumn (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;	

    int xoff, yoff;
    uint8_t xmask;

    count = dc_yh - dc_yl;
    if (count < 0)
	return;
				
#ifdef RANGECHECK
    if ((unsigned)dc_x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	I_Error ( "R_DrawColumn: %i to %i at %i",
		  dc_yl, dc_yh, dc_x);
    }

#endif


    yoff = dc_yl;
    xoff = columnofs[dc_x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0x80 >> (xoff & 7);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
	// Translation tables are used
	//  to map certain colorramps to other ones,
	//  used with PLAY sprites.
	// Thus the "green" ramp of the player 0 sprite
	//  is mapped to gray, red, black/indigo.

    // ...holy indirection batman!!!
    if (shades[graymap[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]]][yoff & 3] & xmask) {
        *dest |= xmask;
    } else {
        *dest &= ~xmask;
    }
	
	dest += SCREENSTRIDE;
	frac += fracstep;
    ++yoff;
    } while (count--);
}

void R_DrawTranslatedColumnLow (void)
{
    int			count;
    pixel_t*		dest;
    fixed_t		frac;
    fixed_t		fracstep;	
    int                 x;

    int xoff, yoff;
    uint8_t xmask;

    count = dc_yh - dc_yl;
    if (count < 0)
	return;

    // low detail, need to scale by 2
    x = dc_x << 1;
				
#ifdef RANGECHECK
    if ((unsigned)x >= SCREENWIDTH
	|| dc_yl < 0
	|| dc_yh >= SCREENHEIGHT)
    {
	I_Error ( "R_DrawColumn: %i to %i at %i",
		  dc_yl, dc_yh, x);
    }

#endif


    yoff = dc_yl;
    xoff = columnofs[x];
    dest = ylookup[dc_yl] + (xoff >> 3);
    xmask = 0xc0 >> (xoff & 7);

    // Looks familiar.
    fracstep = dc_iscale;
    frac = dc_texturemid + (dc_yl-centery)*fracstep;

    // Here we do an additional index re-mapping.
    do
    {
	// Translation tables are used
	//  to map certain colorramps to other ones,
	//  used with PLAY sprites.
	// Thus the "green" ramp of the player 0 sprite
	//  is mapped to gray, red, black/indigo.
    *dest &= ~xmask;
    *dest |= (shades[graymap[dc_colormap[dc_translation[dc_source[frac>>FRACBITS]]]]][yoff & 3] & xmask);
	dest += SCREENSTRIDE;
	frac += fracstep;
    ++yoff;
	
	frac += fracstep;
    } while (count--);
}




//
// R_InitTranslationTables
// Creates the translation tables to map
//  the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables (void)
{
    int		i;
	
    translationtables = Z_Malloc (256*3, PU_STATIC, 0);

    // translate just the 16 green colors
    for (i=0 ; i<256 ; i++)
    {
	if (i >= 0x70 && i<= 0x7f)
	{
	    // map green ramp to gray, brown, red
	    translationtables[i] = 0x60 + (i&0xf);
	    translationtables [i+256] = 0x40 + (i&0xf);
	    translationtables [i+512] = 0x20 + (i&0xf);
	}
	else
	{
	    // Keep all other colors as is.
	    translationtables[i] = translationtables[i+256]
		= translationtables[i+512] = i;
	}
    }
}




//
// R_DrawSpan
// With DOOM style restrictions on view orientation,
//  the floors and ceilings consist of horizontal slices
//  or spans with constant z depth.
// However, rotation around the world z axis is possible,
//  thus this mapping, while simpler and faster than
//  perspective correct texture mapping, has to traverse
//  the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//  and the inner loop has to step in texture space u and v.
//
int			ds_y;
int			ds_x1;
int			ds_x2;

lighttable_t*		ds_colormap;

fixed_t			ds_xfrac;
fixed_t			ds_yfrac;
fixed_t			ds_xstep;
fixed_t			ds_ystep;

// start of a 64*64 tile image
byte*			ds_source;	

// just for profiling
int			dscount;


//
// Draws the actual span.
void R_DrawSpan (void)
{
    unsigned int position, step;
    pixel_t *dest;
    int count;
    int spot;
    unsigned int xtemp, ytemp;

    int xoff;
    uint8_t xmask;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++;
#endif

    // Pack position and step variables into a single 32-bit integer,
    // with x in the top 16 bits and y in the bottom 16 bits.  For
    // each 16-bit part, the top 6 bits are the integer part and the
    // bottom 10 bits are the fractional part of the pixel position.

    position = ((ds_xfrac << 10) & 0xffff0000)
             | ((ds_yfrac >> 6)  & 0x0000ffff);
    step = ((ds_xstep << 10) & 0xffff0000)
         | ((ds_ystep >> 6)  & 0x0000ffff);

    xoff = columnofs[ds_x1];
    dest = ylookup[ds_y] + (xoff >> 3);
    xmask = 0x80 >> (xoff & 7);

    // We do not check for zero spans here?
    count = ds_x2 - ds_x1;

    do
    {
	// Calculate current texture index in u,v.
        ytemp = (position >> 4) & 0x0fc0;
        xtemp = (position >> 26);
        spot = xtemp | ytemp;

	// Lookup pixel from flat texture tile,
	//  re-index using light/colormap.
        if (shades[graymap[ds_colormap[ds_source[spot]]]][ds_y & 3] & xmask) {
	        *dest |= xmask;
        } else {
            *dest &= ~xmask;
        }

        xmask >>= 1;
        if (xmask == 0) {
            xmask = 0x80;
            ++dest;
        }

        position += step;

    } while (count--);
}

//
// Again..
//
void R_DrawSpanLow (void)
{
    unsigned int position, step;
    unsigned int xtemp, ytemp;
    pixel_t *dest;
    int count;
    int spot;

    int xoff;
    uint8_t xmask;

#ifdef RANGECHECK
    if (ds_x2 < ds_x1
	|| ds_x1<0
	|| ds_x2>=SCREENWIDTH
	|| (unsigned)ds_y>SCREENHEIGHT)
    {
	I_Error( "R_DrawSpan: %i to %i at %i",
		 ds_x1,ds_x2,ds_y);
    }
//	dscount++;
#endif

    position = ((ds_xfrac << 10) & 0xffff0000)
             | ((ds_yfrac >> 6)  & 0x0000ffff);
    step = ((ds_xstep << 10) & 0xffff0000)
         | ((ds_ystep >> 6)  & 0x0000ffff);

    count = (ds_x2 - ds_x1);

    // Blocky mode, need to multiply by 2.
    ds_x1 <<= 1;
    ds_x2 <<= 1;

    xoff = columnofs[ds_x1];
    dest = ylookup[ds_y] + (xoff >> 3);
    xmask = 0xc0 >> (xoff & 7);

    do
    {
	// Calculate current texture index in u,v.
        ytemp = (position >> 4) & 0x0fc0;
        xtemp = (position >> 26);
        spot = xtemp | ytemp;

        *dest &= ~xmask;
        *dest |= (shades[graymap[ds_colormap[ds_source[spot]]]][ds_y & 3] & xmask);

        xmask >>= 2;
        if (xmask == 0) {
            xmask = 0xc0;
            ++dest;
        }

	position += step;

    } while (count--);
}

//
// R_InitBuffer
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int		width,
  int		height )
{
    int		i;

    // Handle resize,
    //  e.g. smaller view windows
    //  with border and/or status bar.
    viewwindowx = (SCREENWIDTH-width) >> 1;

    // Column offset. For windows.
    for (i=0 ; i<width ; i++)
	columnofs[i] = viewwindowx + i;

    // Samw with base row offset.
    if (width == SCREENWIDTH)
	viewwindowy = 0;
    else
	viewwindowy = (SCREENHEIGHT-SBARHEIGHT-height) >> 1;

    // Preclaculate all row offsets.
    for (i=0 ; i<height ; i++) {
        ylookup[i] = I_VideoBuffer + (i+viewwindowy)*52;
    }
}

//
// Copy a screen buffer.
//
void
R_VideoErase
( unsigned	ofs,
  int		count )
{
  // LFB copy.
  // This might not be a good idea if memcpy
  //  is not optiomal, e.g. byte by byte on
  //  a 32bit CPU, as GNU GCC/Linux libc did
  //  at one point.

    memset(I_VideoBuffer + ofs, 0, count * sizeof(*I_VideoBuffer));
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void R_DrawViewBorder (void)
{
    int		top;
    int		side;
    int		ofs;
    int		i;

    if (scaledviewwidth == SCREENWIDTH)
	return;

    top = ((SCREENHEIGHT-SBARHEIGHT)-viewheight)/2;
    side = (SCREENWIDTH-scaledviewwidth)>>4;

    // copy top and one line of left size
    R_VideoErase (0, top*SCREENSTRIDE+side);

    // copy one line of right side and bottom
    ofs = (viewheight+top)*SCREENSTRIDE-side-2;
    R_VideoErase (ofs, top*SCREENSTRIDE+side+2);

    // copy sides using wraparound
    ofs = (top+1)*SCREENSTRIDE-side-2;
    side <<= 1;
    side += 2;

    for (i=1 ; i<viewheight ; i++)
    {
	R_VideoErase (ofs, side);
	ofs += SCREENSTRIDE;
    }
}


